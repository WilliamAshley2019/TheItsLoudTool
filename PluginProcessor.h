#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

class ViaU2AudioProcessor final : public juce::AudioProcessor
{
public:
    ViaU2AudioProcessor();
    ~ViaU2AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Meter Getters
    float getCurrentMomentaryLUFS() const noexcept { return currentMomentaryLUFS.load(std::memory_order_relaxed); }
    float getCurrentShortTermLUFS() const noexcept { return currentShortTermLUFS.load(std::memory_order_relaxed); }
    float getCurrentIntegratedLUFS() const noexcept { return currentIntegratedLUFS.load(std::memory_order_relaxed); }
    float getCurrentTruePeak() const noexcept { return currentTruePeak.load(std::memory_order_relaxed); }
    float getHeldTruePeak() const noexcept { return heldTruePeak.load(std::memory_order_relaxed); }
    bool isPeakHit() const noexcept { return peakHit.load(std::memory_order_relaxed); }

    void resetIntegratedLoudness() noexcept;
    int getCurrentMode() const noexcept { return currentMode.load(std::memory_order_relaxed); }
    void cycleLoudnessMode() noexcept;
    void exportCSV() noexcept;

    // DSP Control Atomics (Thread-safe UI to Audio thread)
    std::atomic<float> targetLUFS{ -14.0f };
    std::atomic<float> driveAmount{ 2.0f };   // 1.0 = clean, >1.0 = saturation
    std::atomic<float> ceilingDB{ -1.0f };    // True Peak Ceiling
    std::atomic<bool> isBypassed{ false };

private:
    // Oversampling
    juce::dsp::Oversampling<float> oversampling{ 2, false, juce::dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple };

    // Metering State
    std::atomic<float> currentTruePeak{ -100.0f };
    std::atomic<float> heldTruePeak{ -100.0f };
    std::atomic<bool> peakHit{ false };
    int peakHoldSamples = 0;
    int peakHoldDuration = 0;

    juce::dsp::IIR::Filter<float> kFilterL1, kFilterR1;
    juce::dsp::IIR::Filter<float> kFilterL2, kFilterR2;

    std::vector<float> momentaryBufferL, momentaryBufferR;
    int momWritePos = 0, maxMomSamples = 0;
    double momentarySum = 0.0;
    std::atomic<float> currentMomentaryLUFS{ -100.0f };

    std::vector<float> shortTermBufferL, shortTermBufferR;
    int stWritePos = 0, maxStSamples = 0;
    double shortTermSum = 0.0;
    std::atomic<float> currentShortTermLUFS{ -100.0f };

    double integratedSum = 0.0, integratedCount = 0.0;
    std::atomic<float> currentIntegratedLUFS{ -100.0f };
    std::atomic<int> currentMode{ 0 };

    // Limiter State (Lookahead)
    std::vector<float> lookaheadBufferL, lookaheadBufferR;
    int lookaheadWritePos = 0;
    int lookaheadSamples = 0;
    float peakEnv = 0.0f;
    float currentGainReduction = 1.0f;
    float smoothedMakeupGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViaU2AudioProcessor)
};