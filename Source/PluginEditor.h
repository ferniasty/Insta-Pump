#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Rave Academy look — identical palette and fonts to Kick Extractor
namespace look
{
inline const juce::Colour night     { 0xff07030c };   // background
inline const juce::Colour plum      { 0xff160826 };   // panels
inline const juce::Colour line      { 0xff2e1a45 };
inline const juce::Colour violet    { 0xff8a2be2 };
inline const juce::Colour lavender  { 0xffd3a8ff };   // small caps labels
inline const juce::Colour lime      { 0xffc6ff1a };
inline const juce::Colour limeDeep  { 0xff17200a };
inline const juce::Colour white     { 0xfff4f1f8 };
inline const juce::Colour grey      { 0xff9a90a8 };

juce::Font heading (float height);            // Oswald Bold
juce::Font mono (float height, bool bold);    // Space Mono
void spaced (juce::Graphics&, const juce::String&, juce::Rectangle<float>, float height, juce::Colour,
             juce::Justification = juce::Justification::centredLeft, float tracking = 0.18f, bool bold = true);
}

class RaveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
};

// waveform + envelope display
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

// one card with a knob, its name and value
class KnobCard : public juce::Component
{
public:
    KnobCard (KickDuckProcessor& p, const juce::String& paramId, const juce::String& title, bool stepped);
    void paint (juce::Graphics&) override;
    void resized() override;
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
