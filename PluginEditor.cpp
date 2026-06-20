#include "PluginEditor.h"
#include "PluginProcessor.h"

// ==============================================================================
// VuMeter Implementation
// ==============================================================================
void VuMeter::drawBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::Path outerBezel; outerBezel.addRoundedRectangle(bounds, 8.f);
    juce::ColourGradient bezelGrad(juce::Colour(0xff1a1a1a), bounds.getX(), bounds.getY(), juce::Colour(0xff0a0a0a), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bezelGrad); g.fillPath(outerBezel);
    g.setColour(juce::Colours::black); g.strokePath(outerBezel, juce::PathStrokeType(2.0f));
    auto inner = bounds.reduced(6.f, 6.f);
    g.setColour(juce::Colour(0xff0d0d0d)); g.fillRoundedRectangle(inner, 4.f);
    auto face = inner.reduced(4.f, 4.f);
    g.setColour(juce::Colour(0xffe8e4d8)); g.fillRoundedRectangle(face, 3.f);
    g.setColour(juce::Colours::black.withAlpha(0.3f)); g.drawRoundedRectangle(face, 3.f, 1.5f);
    auto centreX = face.getCentreX(), centreY = face.getBottom() + face.getHeight() * 0.15f, radius = face.getWidth() * 0.65f;
    juce::Path arcPath; arcPath.addCentredArc(centreX, centreY, radius, radius, 0.0f, -juce::MathConstants<float>::pi * 0.45f, juce::MathConstants<float>::pi * 0.45f, true);
    g.setColour(juce::Colours::black.withAlpha(0.1f)); g.strokePath(arcPath, juce::PathStrokeType(radius * 0.15f, juce::PathStrokeType::curved));
    g.setColour(juce::Colours::black);
    juce::Font font(juce::FontOptions().withHeight(juce::jmax(9.0f, radius * 0.08f)).withStyle("Bold")); g.setFont(font);
    std::vector<float> vuTicks = { -60.0f, -40.0f, -20.0f, -10.0f, -7.0f, -5.0f, -3.0f, 0.0f, 3.0f };
    for (float dB : vuTicks) {
        float angle = dBToAngle(dB), innerRadius = radius * 0.80f, outerRadius = (dB >= 0.0f) ? radius * 0.95f : radius * 0.88f;
        float startX = centreX + innerRadius * std::sin(angle), startY = centreY - innerRadius * std::cos(angle);
        float endX = centreX + outerRadius * std::sin(angle), endY = centreY - outerRadius * std::cos(angle);
        g.setColour(dB >= 0.0f ? juce::Colours::red : juce::Colours::black); g.drawLine(startX, startY, endX, endY, dB >= 0.0f ? 2.0f : 1.5f);
        float textRadius = radius * 0.68f, textX = centreX + textRadius * std::sin(angle), textY = centreY - textRadius * std::cos(angle);
        juce::String text = dB == 0.0f ? "0" : dB > 0.0f ? "+" + juce::String(juce::roundToInt(dB)) : juce::String(juce::roundToInt(dB));
        g.drawText(text, juce::Rectangle<float>(textX - 15, textY - 10, 30, 20), juce::Justification::centred, false);
    }
    g.setColour(juce::Colours::black); g.setFont(juce::FontOptions().withHeight(juce::jmax(14.0f, radius * 0.12f)).withStyle("Bold"));
    g.drawText("VIAU-MAX", face.getCentreX() - 35, face.getBottom() - 35, 70, 25, juce::Justification::centred);
    juce::ColourGradient glass(juce::Colours::white.withAlpha(0.15f), face.getX(), face.getY(), juce::Colours::white.withAlpha(0.0f), face.getX(), face.getCentreY(), false);
    g.setGradientFill(glass); g.fillRoundedRectangle(face, 3.f);
}

void VuMeter::drawDynamicElements(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat(), face = bounds.reduced(10.f, 10.f);
    auto centreX = face.getCentreX(), centreY = face.getBottom() + face.getHeight() * 0.15f, radius = face.getWidth() * 0.65f;
    float angle = dBToAngle(currentLevel), pointerLength = radius * 0.92f;
    float endX = centreX + pointerLength * std::sin(angle), endY = centreY - pointerLength * std::cos(angle);
    g.setColour(juce::Colours::black.withAlpha(0.3f)); g.drawLine(centreX + 2, centreY + 2, endX + 2, endY + 2, 2.5f);
    g.setColour(juce::Colour(0xffb01010)); g.drawLine(centreX, centreY, endX, endY, 2.0f);
    g.setColour(juce::Colours::black); g.fillEllipse(centreX - 8, centreY - 8, 16, 16);
    juce::ColourGradient pivotGrad(juce::Colour(0xff3a3a3a), centreX - 5, centreY - 5, juce::Colours::black, centreX + 5, centreY + 5, true);
    g.setGradientFill(pivotGrad); g.fillEllipse(centreX - 6, centreY - 6, 12, 12);
    float ledSize = 14.0f, ledX = bounds.getRight() - ledSize - 12.0f, ledY = 12.0f;
    if (peakLED) { g.setColour(juce::Colours::red.withAlpha(0.3f)); g.fillEllipse(ledX - 4, ledY - 4, ledSize + 8, ledSize + 8); g.setColour(juce::Colours::red); }
    else { g.setColour(juce::Colour(0xff440000)); }
    g.fillEllipse(ledX, ledY, ledSize, ledSize);
    g.setColour(juce::Colours::white.withAlpha(0.4f)); g.fillEllipse(ledX + 2, ledY + 2, ledSize * 0.4f, ledSize * 0.4f);
    g.setColour(juce::Colours::black); g.setFont(juce::FontOptions().withHeight(10.0f).withStyle("Bold"));
    g.drawText("PEAK", ledX - 38, ledY + 2, 32, 12, juce::Justification::centredRight);
}

// ==============================================================================
// Main Editor Implementation
// ==============================================================================
ViaU2AudioProcessorEditor::ViaU2AudioProcessorEditor(ViaU2AudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    setSize(750, 340);
    setResizable(true, true);
    setResizeLimits(600, 250, 1200, 500);

    for (auto* s : { &screwTL, &screwTR, &screwBL, &screwBR }) addAndMakeVisible(s);
    for (auto* btn : { &resetButton, &modeButton, &exportButton, &bypassButton }) {
        btn->setLookAndFeel(&buttonLAF); addAndMakeVisible(btn);
    }

    resetButton.onClick = [this]() { processor.resetIntegratedLoudness(); };
    modeButton.onClick = [this]() { processor.cycleLoudnessMode(); };
    exportButton.onClick = [this]() { processor.exportCSV(); };
    bypassButton.onClick = [this]() {
        bool state = !processor.isBypassed.load();
        processor.isBypassed.store(state);
        bypassButton.setButtonText(state ? "ENGAGED" : "BYPASS");
        };

    auto setupKnob = [&](juce::Slider& knob, juce::Label& label, juce::String name, float min, float max, float def, juce::String suffix) {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        knob.setRange(min, max, 0.1);
        knob.setValue(def);
        knob.setLookAndFeel(&knobLAF);
        addAndMakeVisible(knob);
        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions().withName("Courier New").withStyle("Bold").withHeight(10.0f));
        label.setColour(juce::Label::textColourId, BroadcastColour::labelText);
        addAndMakeVisible(label);
        };

    setupKnob(targetKnob, targetLabel, "TARGET LUFS", -30.0, -8.0, -14.0, "");
    setupKnob(driveKnob, driveLabel, "TEXTURE", 1.0, 5.0, 2.0, "x");
    setupKnob(ceilingKnob, ceilingLabel, "CEILING", -10.0, 0.0, -1.0, "dBTP");

    targetKnob.onValueChange = [this]() { processor.targetLUFS.store((float)targetKnob.getValue()); };
    driveKnob.onValueChange = [this]() { processor.driveAmount.store((float)driveKnob.getValue()); };
    ceilingKnob.onValueChange = [this]() { processor.ceilingDB.store((float)ceilingKnob.getValue()); };

    addAndMakeVisible(vuMeter);
    startTimerHz(30);
}

ViaU2AudioProcessorEditor::~ViaU2AudioProcessorEditor()
{
    for (auto* btn : { &resetButton, &modeButton, &exportButton, &bypassButton }) btn->setLookAndFeel(nullptr);
    targetKnob.setLookAndFeel(nullptr); driveKnob.setLookAndFeel(nullptr); ceilingKnob.setLookAndFeel(nullptr);
}

void ViaU2AudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    const int earW = 34, railH = 16;
    screwTL.setBounds(earW - 24, railH + 10, 16, 16);
    screwTR.setBounds(bounds.getWidth() - earW + 8, railH + 10, 16, 16);
    screwBL.setBounds(earW - 24, bounds.getHeight() - railH - 26, 16, 16);
    screwBR.setBounds(bounds.getWidth() - earW + 8, bounds.getHeight() - railH - 26, 16, 16);

    auto panel = bounds.reduced(earW, railH + 8);
    auto lcdArea = panel.removeFromTop(panel.getHeight() * 0.55f).reduced(10);
    auto bottomArea = panel.reduced(10);
    auto controlsArea = bottomArea.removeFromLeft(bottomArea.getWidth() * 0.55f);

    auto btnArea = controlsArea.removeFromTop(35);
    int btnW = btnArea.getWidth() / 4;
    resetButton.setBounds(btnArea.removeFromLeft(btnW).reduced(4));
    modeButton.setBounds(btnArea.removeFromLeft(btnW).reduced(4));
    exportButton.setBounds(btnArea.removeFromLeft(btnW).reduced(4));
    bypassButton.setBounds(btnArea.removeFromLeft(btnW).reduced(4));

    auto knobArea = controlsArea;
    int knobSize = 65;
    int totalKnobWidth = knobSize * 3 + 30;
    int startX = knobArea.getX() + (knobArea.getWidth() - totalKnobWidth) / 2;
    int knobY = knobArea.getY() + 15;

    targetKnob.setBounds(startX, knobY, knobSize, knobSize + 20);
    driveKnob.setBounds(startX + knobSize + 15, knobY, knobSize, knobSize + 20);
    ceilingKnob.setBounds(startX + (knobSize + 15) * 2, knobY, knobSize, knobSize + 20);

    targetLabel.setBounds(targetKnob.getX(), targetKnob.getBottom(), knobSize, 16);
    driveLabel.setBounds(driveKnob.getX(), driveKnob.getBottom(), knobSize, 16);
    ceilingLabel.setBounds(ceilingKnob.getX(), ceilingKnob.getBottom(), knobSize, 16);

    auto vuPanel = bottomArea;
    int vuWidth = vuPanel.getWidth(), vuHeight = vuWidth / 2;
    if (vuHeight > vuPanel.getHeight()) { vuHeight = vuPanel.getHeight(); vuWidth = vuHeight * 2; }
    vuMeter.setBounds(vuPanel.withSizeKeepingCentre(vuWidth, vuHeight).toNearestInt());
}

void ViaU2AudioProcessorEditor::timerCallback()
{
    auto smooth = [](float curr, float target, float coeff) { return curr + coeff * (target - curr); };
    smoothMom = smooth(smoothMom, processor.getCurrentMomentaryLUFS(), 0.4f);
    smoothShort = smooth(smoothShort, processor.getCurrentShortTermLUFS(), 0.2f);
    smoothInt = smooth(smoothInt, processor.getCurrentIntegratedLUFS(), 0.05f);
    smoothPeak = smooth(smoothPeak, processor.getCurrentTruePeak(), 0.6f);
    smoothNeedleVU = smooth(smoothNeedleVU, processor.getCurrentShortTermLUFS(), 0.3f);
    vuMeter.setLevel(smoothNeedleVU);
    if (smoothPeak > 0.0f) vuMeter.triggerPeak();
    repaint();
}

void ViaU2AudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawChassisAndRack(g, bounds);
    auto panel = bounds.reduced(34.f, 24.f);
    auto lcdArea = panel.removeFromTop(panel.getHeight() * 0.55f).reduced(10.f);
    drawLCDScreen(g, lcdArea);
}

void ViaU2AudioProcessorEditor::drawChassisAndRack(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    const float W = bounds.getWidth(), H = bounds.getHeight(), earW = 34.f, railH = 16.f;
    g.setColour(BroadcastColour::chassis); g.fillRoundedRectangle(bounds, 4.f);
    juce::ColourGradient leftEar(BroadcastColour::rackEarLight, 0.f, 0.f, BroadcastColour::rackEar, earW, 0.f, false);
    g.setGradientFill(leftEar); g.fillRoundedRectangle(0.f, 0.f, earW, H, 4.f);
    g.setColour(BroadcastColour::rackEar); g.fillRect(W - earW, 0.f, earW, H);
    g.setColour(BroadcastColour::rackRail); g.fillRect(earW, 0.f, W - 2.f * earW, railH); g.fillRect(earW, H - railH, W - 2.f * earW, railH);
    for (float yy = 2.f; yy < railH - 1.f; yy += 3.f) {
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.fillRect(earW, yy, W - 2.f * earW, 1.f); g.fillRect(earW, H - railH + yy, W - 2.f * earW, 1.f);
    }
}

void ViaU2AudioProcessorEditor::drawLCDScreen(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Path bezelPath; bezelPath.addRoundedRectangle(bounds, 12.f);
    juce::ColourGradient bezelGrad(juce::Colour(0xff2a2a2a), bounds.getX(), bounds.getY(), juce::Colour(0xff050505), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bezelGrad); g.fillPath(bezelPath);
    g.setColour(juce::Colours::black); g.strokePath(bezelPath, juce::PathStrokeType(2.0f));
    auto lcd = bounds.reduced(12.f, 8.f);
    g.setColour(BroadcastColour::screenBg); g.fillRect(lcd);
    g.setColour(juce::Colours::black.withAlpha(0.8f)); g.drawRect(lcd, 3);
    auto content = lcd.reduced(10.f, 8.f);
    auto fontDigital = juce::FontOptions().withName("Courier New").withStyle("Bold");
    auto drawTealText = [&](const juce::String& text, juce::Rectangle<float> area, float height, juce::Justification just, bool active = true) {
        g.setFont(fontDigital.withHeight(height));
        g.setColour(BroadcastColour::tealGlow.withAlpha(0.2f)); g.drawFittedText(text, area.expanded(2).toNearestInt(), just, 1);
        g.setColour(active ? BroadcastColour::tealGlow : BroadcastColour::tealDim); g.drawFittedText(text, area.toNearestInt(), just, 1);
        };
    auto topRow = content.removeFromTop(20.f);
    drawTealText("VIAU-MAX DSP", topRow.removeFromLeft(140.f), 14.f, juce::Justification::centredLeft);
    auto mode = processor.getCurrentMode();
    juce::String modeText = mode == 0 ? "PERCEPTUAL" : mode == 1 ? "MODE: SHORT" : "MODE: INF";
    drawTealText(modeText, topRow.removeFromRight(170.f), 14.f, juce::Justification::centredRight);
    content.removeFromTop(10.f);
    auto readoutArea = content.removeFromTop(content.getHeight() * 0.6f);
    float colW = readoutArea.getWidth() / 3.f;
    auto drawReadout = [&](juce::Rectangle<float> area, const juce::String& label, float val) {
        drawTealText(label, area.removeFromTop(16.f), 12.f, juce::Justification::centred);
        juce::String str = (val <= -90.f) ? "-inf" : juce::String(val, 1);
        drawTealText(str, area, 38.f, juce::Justification::centred);
        };
    drawReadout(readoutArea.removeFromLeft(colW), "INTEGRATED", smoothInt);
    drawReadout(readoutArea.removeFromLeft(colW), "SHORT-TERM", smoothShort);
    drawReadout(readoutArea, "MOMENTARY", smoothMom);
    content.removeFromTop(5.f);
    auto bottomRow = content;
    juce::String peakStr = (smoothPeak <= -90.f) ? "-inf" : juce::String(smoothPeak, 1) + " dBTP";
    drawTealText("TRUE PEAK: " + peakStr, bottomRow, 16.f, juce::Justification::centredLeft);
    for (float yy = lcd.getY(); yy < lcd.getBottom(); yy += 2.f) {
        g.setColour(juce::Colours::black.withAlpha(0.15f)); g.fillRect(lcd.getX(), yy, lcd.getWidth(), 1.f);
    }
    juce::ColourGradient bloom(BroadcastColour::tealGlow.withAlpha(0.05f), lcd.getCentreX(), lcd.getCentreY(), BroadcastColour::tealGlow.withAlpha(0.f), lcd.getRight(), lcd.getBottom(), true);
    g.setGradientFill(bloom); g.fillRect(lcd);
}