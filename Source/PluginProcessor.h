#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Envelope.h"

class KickDuckProcessor : public juce::AudioProcessor
{
public:
    KickDuckProcessor();
    ~KickDuckProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState params;

    kickduck::Shape currentShape() const;

    // editor size is remembered with the session (same as Kick Extractor)
    int getSavedWidth() const { return (int) params.state.getProperty ("uiWidth", 0); }
    void setSavedWidth (int w) { params.state.setProperty ("uiWidth", w, nullptr); }
    double currentMix01() const { return mixParam->load() / 100.0; }

    // ---- data for the waveform display (audio thread -> UI thread, lock free)
    struct MeterPoint { float barPhase, inPeak, outPeak; };
    int readMeter (MeterPoint* dest, int maxPoints);
    std::atomic<bool> hostPlaying { false };
    std::atomic<double> hostBpm { 120.0 };
    std::atomic<float> barPhase { -1.0f };
    std::atomic<int> beatsPerBar { 4 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* lengthParam = nullptr;
    std::atomic<float>* curveParam = nullptr;
    std::atomic<float>* shapeParam = nullptr;

    double sr = 44100.0;
    juce::SmoothedValue<double> depth;

    static constexpr int meterSize = 8192;
    juce::AbstractFifo meterFifo { meterSize };
    std::array<MeterPoint, meterSize> meterBuf {};
    int chunkCount = 0;
    float chunkIn = 0, chunkOut = 0;
    double chunkBarPhase = 0;
    void pushMeter (float ph, float in, float out);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickDuckProcessor)
};
