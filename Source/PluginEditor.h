#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace rave
{
const juce::Colour bg0 { 0xff09060f }, bg1 { 0xff150c24 }, panel { 0xff110b1c }, card { 0xff170f24 },
    cardOn { 0xff1a2008 }, border { 0xff2c2140 }, lime { 0xffc8ff1a }, purple { 0xff9545e7 },
    silver { 0xffc7c2d6 }, text { 0xffe6e2ee }, dim { 0xff8a8199 };
}

class RaveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
};

// waveform + envelope display (top)
class EnvelopeDisplay : public juce::Component
{
public:
    explicit EnvelopeDisplay (KickDuckProcessor& p);
    void paint (juce::Graphics&) override;
    void pullMeter();
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    KickDuckProcessor& proc;
    static constexpr int numCols = 720;
    std::array<float, numCols> inPk {}, outPk {};
    int lastCol = -1;
    float dragStartLen = 50.f;
    juce::RangedAudioParameter* lenParam = nullptr;
};

// one Kick-Extractor-style card holding a knob
class KnobCard : public juce::Component
{
public:
    KnobCard (KickDuckProcessor& p, const juce::String& paramId, const juce::String& title, bool stepped);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { repaint(); }
    juce::Slider slider;

private:
    KickDuckProcessor& proc;
    juce::String pid, title;
    bool stepped;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class KickDuckEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KickDuckEditor (KickDuckProcessor&);
    ~KickDuckEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    // same canvas as Kick Extractor: 960 x 420, scaled 60..200 %
    static constexpr int baseW = 960, baseH = 420;

private:
    struct Panel : public juce::Component
    {
        explicit Panel (KickDuckEditor& e) : owner (e) {}
        void paint (juce::Graphics& g) override { owner.paintPanel (g); }
        KickDuckEditor& owner;
    };

    void timerCallback() override;
    void paintPanel (juce::Graphics&);

    KickDuckProcessor& proc;
    RaveLookAndFeel lnf;
    Panel panel { *this };
    EnvelopeDisplay display;
    KnobCard mixCard, lengthCard, curveCard, shapeCard;
    juce::Image logo;
    bool restoring = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckEditor)
};
