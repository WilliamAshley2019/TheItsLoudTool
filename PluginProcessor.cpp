#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <chrono>

#if _MSC_VER
#pragma float_control(precise, off)
#pragma fp_contract(on)
#endif

ViaU2AudioProcessor::ViaU2AudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    oversampling(2, false, juce::dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple)
{
}

bool ViaU2AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo();
}

void ViaU2AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentMomentaryLUFS.store(-100.0f, std::memory_order_relaxed);
    currentShortTermLUFS.store(-100.0f, std::memory_order_relaxed);
    currentIntegratedLUFS.store(-100.0f, std::memory_order_relaxed);
    currentTruePeak.store(-100.0f, std::memory_order_relaxed);
    heldTruePeak.store(-100.0f, std::memory_order_relaxed);
    peakHit.store(false, std::memory_order_relaxed);

    oversampling.initProcessing(static_cast<size_t>(samplesPerBlock));

    peakHoldDuration = static_cast<int>(sampleRate * 1.5);
    peakHoldSamples = 0;

    maxMomSamples = static_cast<int>(sampleRate * 0.4);
    momentaryBufferL.resize(maxMomSamples, 0.0f);
    momentaryBufferR.resize(maxMomSamples, 0.0f);
    momWritePos = 0; momentarySum = 0.0;

    maxStSamples = static_cast<int>(sampleRate * 3.0);
    shortTermBufferL.resize(maxStSamples, 0.0f);
    shortTermBufferR.resize(maxStSamples, 0.0f);
    stWritePos = 0; shortTermSum = 0.0;

    integratedSum = 0.0; integratedCount = 0.0;

    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.13547087602444);
    auto shelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1500.0f, 0.707f, 4.0f);

    kFilterL1.coefficients = hpCoeffs; kFilterR1.coefficients = hpCoeffs;
    kFilterL2.coefficients = shelfCoeffs; kFilterR2.coefficients = shelfCoeffs;
    kFilterL1.reset(); kFilterR1.reset();
    kFilterL2.reset(); kFilterR2.reset();

    // Lookahead Limiter Setup (5ms lookahead)
    lookaheadSamples = juce::jmax(1, static_cast<int>(sampleRate * 0.005));
    lookaheadBufferL.assign(lookaheadSamples, 0.0f);
    lookaheadBufferR.assign(lookaheadSamples, 0.0f);
    lookaheadWritePos = 0;
    peakEnv = 0.0f;
    currentGainReduction = 1.0f;
    smoothedMakeupGain = 1.0f;

    // Report total latency (Oversampling + Lookahead)
    setLatencySamples(oversampling.getLatencyInSamples() + lookaheadSamples);
}

void ViaU2AudioProcessor::releaseResources() { oversampling.reset(); }

void ViaU2AudioProcessor::resetIntegratedLoudness() noexcept
{
    integratedSum = 0.0; integratedCount = 0.0;
    currentIntegratedLUFS.store(-100.0f, std::memory_order_relaxed);
}

void ViaU2AudioProcessor::cycleLoudnessMode() noexcept
{
    int mode = currentMode.load(std::memory_order_relaxed);
    currentMode.store((mode + 1) % 3, std::memory_order_relaxed);
}

void ViaU2AudioProcessor::exportCSV() noexcept
{
    juce::String filename = "VIAU_Loudness_" + juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S") + ".csv";
    auto file = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile(filename);
    if (auto stream = file.createOutputStream())
    {
        *stream << "Timestamp,Integrated_LUFS,ShortTerm_LUFS,Momentary_LUFS,TruePeak_dBTP\n";
        *stream << juce::Time::getCurrentTime().toString(true, true, true, true) << ",";
        *stream << currentIntegratedLUFS.load() << "," << currentShortTermLUFS.load() << ",";
        *stream << currentMomentaryLUFS.load() << "," << currentTruePeak.load() << "\n";
        stream->flush();
    }
}

void ViaU2AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0) return;

    bool bypassed = isBypassed.load(std::memory_order_relaxed);

    // ======================================================================
    // 1. PERCEPTUAL GAIN COMPUTER (Feed-Forward Makeup Gain)
    // ======================================================================
    if (!bypassed)
    {
        float currentMom = currentMomentaryLUFS.load(std::memory_order_relaxed);
        float target = targetLUFS.load(std::memory_order_relaxed);

        // Calculate error, clamp to prevent exploding silence (-inf LUFS)
        float errorDB = target - juce::jmax(-70.0f, currentMom);
        errorDB = juce::jlimit(-12.0f, 30.0f, errorDB);

        float targetLinGain = juce::Decibels::decibelsToGain(errorDB);

        // Smooth the gain (approx 50ms slew to prevent zipper noise)
        float gainCoeff = 1.0f - std::exp(-1.0f / (getSampleRate() * 0.05f));
        smoothedMakeupGain += gainCoeff * (targetLinGain - smoothedMakeupGain);

        juce::dsp::AudioBlock<float> block(buffer);
        block.multiplyBy(smoothedMakeupGain);

        // ==================================================================
        // 2. HARMONIC EXCITER (Oversampled Saturation)
        // ==================================================================
        juce::dsp::AudioBlock<float> osBlock(oversampling.processSamplesUp(block));
        float drive = driveAmount.load(std::memory_order_relaxed);

        for (int ch = 0; ch < (int)osBlock.getNumChannels(); ++ch)
        {
            auto* data = osBlock.getChannelPointer(ch);
            for (int n = 0; n < (int)osBlock.getNumSamples(); ++n)
            {
                // Normalized asymmetric tanh for harmonic density
                data[n] = std::tanh(data[n] * drive) / std::tanh(drive);
            }
        }
        oversampling.processSamplesDown(block);
    }

    // ======================================================================
    // 3. LOOKAHEAD LIMITER (Crest Factor Reduction)
    // ======================================================================
    float ceilingLin = juce::Decibels::decibelsToGain(ceilingDB.load(std::memory_order_relaxed));
    float attackCoeff = 1.0f - std::exp(-1.0f / (getSampleRate() * 0.001f)); // 1ms
    float releaseCoeff = 1.0f - std::exp(-1.0f / (getSampleRate() * 0.050f)); // 50ms

    for (int n = 0; n < numSamples; ++n)
    {
        float inL = buffer.getSample(0, n);
        float inR = numChannels > 1 ? buffer.getSample(1, n) : inL;

        // Write to lookahead ring buffer
        lookaheadBufferL[lookaheadWritePos] = inL;
        lookaheadBufferR[lookaheadWritePos] = inR;

        // Scan lookahead window for true peak
        float maxPeak = 0.0f;
        for (int i = 0; i < lookaheadSamples; ++i)
        {
            int idx = (lookaheadWritePos - i + lookaheadSamples) % lookaheadSamples;
            maxPeak = juce::jmax(maxPeak, std::abs(lookaheadBufferL[idx]));
            maxPeak = juce::jmax(maxPeak, std::abs(lookaheadBufferR[idx]));
        }

        // Envelope follower
        if (maxPeak > peakEnv) peakEnv += attackCoeff * (maxPeak - peakEnv);
        else peakEnv += releaseCoeff * (maxPeak - peakEnv);

        // Gain computer
        float gr = (peakEnv > ceilingLin && peakEnv > 0.0001f) ? (ceilingLin / peakEnv) : 1.0f;
        currentGainReduction += 0.1f * (gr - currentGainReduction); // Smooth GR

        // Read delayed signal and apply gain reduction
        int readPos = (lookaheadWritePos - lookaheadSamples + lookaheadSamples) % lookaheadSamples;
        float outL = lookaheadBufferL[readPos] * currentGainReduction;
        float outR = lookaheadBufferR[readPos] * currentGainReduction;

        buffer.setSample(0, n, outL);
        if (numChannels > 1) buffer.setSample(1, n, outR);

        lookaheadWritePos = (lookaheadWritePos + 1) % lookaheadSamples;
    }

    // ======================================================================
    // 4. BS.1770 METERING (Measures Final Output)
    // ======================================================================
    const float* leftIn = buffer.getReadPointer(0);
    const float* rightIn = numChannels > 1 ? buffer.getReadPointer(1) : leftIn;

    float blockPeak = 0.0f;
    for (int n = 0; n < numSamples; ++n)
    {
        float leftFiltered = kFilterL1.processSample(leftIn[n]);
        leftFiltered = kFilterL2.processSample(leftFiltered);
        float rightFiltered = kFilterR1.processSample(rightIn[n]);
        rightFiltered = kFilterR2.processSample(rightFiltered);

        const float leftPower = leftFiltered * leftFiltered;
        const float rightPower = rightFiltered * rightFiltered;
        const float framePower = leftPower + rightPower;

        blockPeak = juce::jmax(blockPeak, std::abs(leftIn[n]));
        blockPeak = juce::jmax(blockPeak, std::abs(rightIn[n]));

        // Momentary (400ms)
        float oldMomL = momentaryBufferL[momWritePos];
        float oldMomR = momentaryBufferR[momWritePos];
        momentaryBufferL[momWritePos] = leftPower;
        momentaryBufferR[momWritePos] = rightPower;
        momentarySum += framePower - (oldMomL + oldMomR);
        momWritePos = (momWritePos + 1) % maxMomSamples;
        double currentMomCount = juce::jmin(momWritePos == 0 ? maxMomSamples : momWritePos, maxMomSamples);
        if (momentarySum > 0.0 && currentMomCount > 0)
            currentMomentaryLUFS.store(-0.691f + 10.0f * std::log10(momentarySum / (currentMomCount * 2.0) + 1e-12f), std::memory_order_relaxed);

        // Short-Term (3s)
        float oldStL = shortTermBufferL[stWritePos];
        float oldStR = shortTermBufferR[stWritePos];
        shortTermBufferL[stWritePos] = leftPower;
        shortTermBufferR[stWritePos] = rightPower;
        shortTermSum += framePower - (oldStL + oldStR);
        stWritePos = (stWritePos + 1) % maxStSamples;
        double currentStCount = juce::jmin(stWritePos == 0 ? maxStSamples : stWritePos, maxStSamples);
        if (shortTermSum > 0.0 && currentStCount > 0)
            currentShortTermLUFS.store(-0.691f + 10.0f * std::log10(shortTermSum / (currentStCount * 2.0) + 1e-12f), std::memory_order_relaxed);

        // Integrated
        integratedSum += framePower;
        integratedCount += 2.0;
        if (integratedSum > 0.0 && integratedCount > 0.0)
            currentIntegratedLUFS.store(-0.691f + 10.0f * std::log10(integratedSum / integratedCount + 1e-12f), std::memory_order_relaxed);
    }

    const float peakDb = juce::Decibels::gainToDecibels(blockPeak, -100.0f);
    currentTruePeak.store(peakDb, std::memory_order_relaxed);

    float currentHeld = heldTruePeak.load(std::memory_order_relaxed);
    if (peakDb > currentHeld) {
        heldTruePeak.store(peakDb, std::memory_order_relaxed);
        peakHoldSamples = peakHoldDuration;
    }
    else {
        peakHoldSamples -= numSamples;
        if (peakHoldSamples <= 0) {
            const float decayed = currentHeld * std::pow(0.9995f, static_cast<float>(numSamples));
            heldTruePeak.store(juce::jmax(decayed, -100.0f), std::memory_order_relaxed);
        }
    }
    peakHit.store(peakDb >= -0.3f, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* ViaU2AudioProcessor::createEditor() { return new ViaU2AudioProcessorEditor(*this); }
void ViaU2AudioProcessor::getStateInformation(juce::MemoryBlock& destData) { juce::ignoreUnused(destData); }
void ViaU2AudioProcessor::setStateInformation(const void* data, int sizeInBytes) { juce::ignoreUnused(data, sizeInBytes); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ViaU2AudioProcessor(); }