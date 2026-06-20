#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <cmath>
#include <vector>

class ViaU2AudioProcessor;

namespace BroadcastColour
{
    static const juce::Colour rackEar{ 0xff2a2d30 };
    static const juce::Colour rackEarLight{ 0xff4a5055 };
    static const juce::Colour rackRail{ 0xff15171a };
    static const juce::Colour chassis{ 0xff1c1e22 };
    static const juce::Colour screenBg{ 0xff050a0f };
    static const juce::Colour tealGlow{ 0xff00e6ff };
    static const juce::Colour tealDim{ 0xff005566 };
    static const juce::Colour orangeFlash{ 0xffff8c00 };
    static const juce::Colour labelText{ 0xff8899aa };
}

class RackScrew : public juce::Component
{
public:
    explicit RackScrew(float angleDeg = 0.f) : angle(juce::degreesToRadians(angleDeg)) {}
    void paint(juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced(1.f);
        const float cx = b.getCentreX(), cy = b.getCentreY(), r = b.getWidth() * 0.5f;
        juce::ColourGradient rim(juce::Colour(0xff5a5a5a), cx - r * 0.3f, cy - r * 0.3f, juce::Colour(0xff1a1a1a), cx + r, cy + r, true);
        g.setGradientFill(rim); g.fillEllipse(b);
        juce::ColourGradient disc(juce::Colour(0xff3c3c3c), cx - r * 0.2f, cy - r * 0.2f, juce::Colour(0xff232323), cx + r * 0.6f, cy + r * 0.6f, true);
        g.setGradientFill(disc); g.fillEllipse(b.reduced(2.f));
        g.setColour(juce::Colour(0xff111111));
        const float slotLen = r * 0.7f, slotW = r * 0.15f, cosA = std::cos(angle), sinA = std::sin(angle);
        auto drawSlot = [&](float dx, float dy) {
            g.drawLine(cx + (-dx * slotLen) * cosA - (-dy * slotLen) * sinA, cy + (-dx * slotLen) * sinA + (-dy * slotLen) * cosA,
                cx + (dx * slotLen) * cosA - (dy * slotLen) * sinA, cy + (dx * slotLen) * sinA + (dy * slotLen) * cosA, slotW);
            };
        drawSlot(1.f, 0.f); drawSlot(0.f, 1.f);
        g.setColour(juce::Colours::white.withAlpha(0.15f)); g.drawEllipse(b.reduced(1.f), 0.7f);
    }
private:
    float angle;
};

class BroadcastKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BroadcastKnobLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, BroadcastColour::tealGlow);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0a0a0a));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::Label::textColourId, BroadcastColour::labelText);
    }
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float startAngle, float endAngle, juce::Slider&) override
    {
        const float cx = x + width * 0.5f, cy = y + height * 0.5f;
        const float outer = juce::jmin(width, height) * 0.5f - 2.f, capR = outer * 0.62f;
        juce::Path arcBg; arcBg.addCentredArc(cx, cy, outer - 2.f, outer - 2.f, 0.f, startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff1e282e)); g.strokePath(arcBg, juce::PathStrokeType(3.f));
        juce::Path arcVal;
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        arcVal.addCentredArc(cx, cy, outer - 2.f, outer - 2.f, 0.f, startAngle, angle, true);
        g.setColour(BroadcastColour::tealGlow.withAlpha(0.3f)); g.strokePath(arcVal, juce::PathStrokeType(7.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(BroadcastColour::tealGlow); g.strokePath(arcVal, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(juce::Colour(0xff3a3a3a)); g.fillEllipse(cx - outer, cy - outer, outer * 2.f, outer * 2.f);
        const int numRidges = juce::roundToInt(outer * 1.6f);
        const float ridgeLen = outer * 0.038f;
        for (int i = 0; i < numRidges; ++i) {
            const float a = (float)i / (float)numRidges * juce::MathConstants<float>::twoPi;
            juce::Path ridge; ridge.addRectangle(-0.45f, -outer, 0.9f, ridgeLen);
            ridge.applyTransform(juce::AffineTransform::rotation(a).translated(cx, cy));
            g.setColour(i % 2 == 0 ? juce::Colour(0xffd4d4d4) : juce::Colour(0xff606060)); g.fillPath(ridge);
        }
        juce::ColourGradient capBlend(juce::Colours::white, cx - capR * 0.35f, cy - capR * 0.4f, juce::Colour(0xffb8bcc0), cx + capR * 0.7f, cy + capR * 0.75f, true);
        capBlend.addColour(0.55, juce::Colour(0xffe2e4e6));
        capBlend.addColour(0.8, BroadcastColour::tealGlow.withAlpha(0.12f).overlaidWith(juce::Colour(0xffd6dadc)));
        g.setGradientFill(capBlend); g.fillEllipse(cx - capR, cy - capR, capR * 2.f, capR * 2.f);
        juce::ColourGradient sweep(juce::Colour(0x28ffffff), cx - capR * 0.6f, cy - capR * 0.7f, juce::Colour(0x00ffffff), cx + capR * 0.4f, cy + capR * 0.5f, false);
        g.setGradientFill(sweep); g.fillEllipse(cx - capR, cy - capR, capR * 2.f, capR * 2.f);
    }
};

class WhiteButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&, bool, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        juce::Colour baseColour = shouldDrawButtonAsDown ? BroadcastColour::orangeFlash : juce::Colour::fromRGB(240, 240, 242);
        juce::ColourGradient grad(baseColour.brighter(0.1f), bounds.getX(), bounds.getY(), baseColour.darker(0.15f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad); g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(juce::Colour(0xff888888).withAlpha(0.6f)); g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setFont(juce::FontOptions().withHeight((float)button.getHeight() / 3.5f).withStyle("Bold"));
        g.setColour(button.isDown() ? juce::Colours::black : juce::Colour(0xff444444));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 2), juce::Justification::centred, 2);
    }
};

class VuMeter : public juce::Component, private juce::Timer
{
public:
    VuMeter() { startTimerHz(10); }
    void setLevel(float newLevel) { currentLevel = juce::jlimit(-60.0f, 6.0f, newLevel); repaint(); }
    void triggerPeak() { peakLED = true; peakLEDCounter = 10; repaint(); }
    void paint(juce::Graphics& g) override { g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality); drawBackground(g); drawDynamicElements(g); }
private:
    float dBToAngle(float dB) const {
        float normalized = juce::jlimit(0.0f, 1.0f, (dB - -60.0f) / (6.0f - -60.0f));
        return juce::MathConstants<float>::pi * -0.75f + normalized * (juce::MathConstants<float>::pi * 0.5f);
    }
    void drawBackground(juce::Graphics& g);
    void drawDynamicElements(juce::Graphics& g);
    void timerCallback() override { if (peakLED && --peakLEDCounter <= 0) { peakLED = false; repaint(); } }
    float currentLevel = -100.0f; bool peakLED = false; int peakLEDCounter = 0;
};

class ViaU2AudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ViaU2AudioProcessorEditor(ViaU2AudioProcessor&);
    ~ViaU2AudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void drawChassisAndRack(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawLCDScreen(juce::Graphics& g, juce::Rectangle<float> bounds);

    ViaU2AudioProcessor& processor;
    BroadcastKnobLookAndFeel knobLAF;
    WhiteButtonLookAndFeel buttonLAF;

    RackScrew screwTL{ 15.f }, screwTR{ -20.f }, screwBL{ -10.f }, screwBR{ 25.f };
    juce::TextButton resetButton{ "RESET INT" }, modeButton{ "MODE" }, exportButton{ "EXPORT CSV" }, bypassButton{ "BYPASS" };

    juce::Slider targetKnob, driveKnob, ceilingKnob;
    juce::Label targetLabel, driveLabel, ceilingLabel;
    VuMeter vuMeter;

    float smoothMom = -100.0f, smoothShort = -100.0f, smoothInt = -100.0f;
    float smoothPeak = -100.0f, smoothNeedleVU = -100.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViaU2AudioProcessorEditor)
};