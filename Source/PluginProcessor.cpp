#include "PluginProcessor.h"
#include "PluginEditor.h"

KickDuckProcessor::KickDuckProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      params (*this, nullptr, "KickDuck", createLayout())
{
    mixParam = params.getRawParameterValue ("mix");
    lengthParam = params.getRawParameterValue ("length");
    curveParam = params.getRawParameterValue ("curve");
    shapeParam = params.getRawParameterValue ("shape");
}

juce::AudioProcessorValueTreeState::ParameterLayout KickDuckProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + "%"; });
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.f, 100.f, 0.1f), 100.f, pct));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "length", 1 }, "Length", NormalisableRange<float> (0.f, 100.f, 0.1f), 50.f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
            return "x" + String (std::pow (2.0, (v - 50.0) / 25.0), 2); })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "curve", 1 }, "Curve", NormalisableRange<float> (-100.f, 100.f, 0.1f), -100.f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
            auto r = roundToInt (v); return r > 0 ? "+" + String (r) : String (r); })));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "shape", 1 }, "Shape", StringArray { "TIGHT", "PUMP", "SMOOTH", "CUT" }, 1));
    return l;
}

kickduck::Shape KickDuckProcessor::currentShape() const
{
    kickduck::Shape s;
    s.modeIndex = (int) shapeParam->load();
    s.length = lengthParam->load();
    s.curve = curveParam->load();
    return s;
}

bool KickDuckProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == layouts.getMainInputChannelSet();
}

void KickDuckProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    depth.reset (sampleRate, 0.02);
    depth.setCurrentAndTargetValue (0.0);
    chunkCount = 0; chunkIn = chunkOut = 0;
}

void KickDuckProcessor::pushMeter (float ph, float in, float out)
{
    const auto scope = meterFifo.write (1);
    if (scope.blockSize1 > 0) meterBuf[(size_t) scope.startIndex1] = { ph, in, out };
    else if (scope.blockSize2 > 0) meterBuf[(size_t) scope.startIndex2] = { ph, in, out };
}

int KickDuckProcessor::readMeter (MeterPoint* dest, int maxPoints)
{
    const auto scope = meterFifo.read (juce::jmin (maxPoints, meterFifo.getNumReady()));
    int n = 0;
    for (int i = 0; i < scope.blockSize1; ++i) dest[n++] = meterBuf[(size_t) (scope.startIndex1 + i)];
    for (int i = 0; i < scope.blockSize2; ++i) dest[n++] = meterBuf[(size_t) (scope.startIndex2 + i)];
    return n;
}

void KickDuckProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch) buffer.clear (ch, 0, numSamples);

    bool playing = false;
    double bpm = 120.0, ppq = 0.0, barStart = 0.0;
    int num = 4, den = 4;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            if (auto bs = pos->getPpqPositionOfLastBarStart()) barStart = *bs;
            if (auto ts = pos->getTimeSignature()) { num = ts->numerator; den = ts->denominator; }
        }
    if (bpm <= 0) bpm = 120.0;
    hostPlaying.store (playing);
    hostBpm.store (bpm);

    const double beatSec = 60.0 / bpm;
    const double beatsPerSample = bpm / (60.0 * sr);
    const double barLenQ = juce::jmax (0.25, num * 4.0 / den);
    beatsPerBar.store (juce::jmax (1, juce::roundToInt (barLenQ)));
    const auto shape = currentShape();
    depth.setTargetValue (playing ? currentMix01() : 0.0);

    for (int i = 0; i < numSamples; ++i)
    {
        const double beat = ppq + i * beatsPerSample;
        const double x = beat - std::floor (beat);
        const double env = kickduck::envelope (x, beatSec, shape);
        const float g = (float) (1.0 - depth.getNextValue() * (1.0 - env));

        float inPk = 0, outPk = 0;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            inPk = juce::jmax (inPk, std::abs (d[i]));
            d[i] *= g;
            outPk = juce::jmax (outPk, std::abs (d[i]));
        }

        if (playing)
        {
            chunkIn = juce::jmax (chunkIn, inPk);
            chunkOut = juce::jmax (chunkOut, outPk);
            if (chunkCount == 0)
            {
                double bp = (beat - barStart) / barLenQ;
                chunkBarPhase = bp - std::floor (bp);
            }
            if (++chunkCount >= 96)
            {
                pushMeter ((float) chunkBarPhase, chunkIn, chunkOut);
                chunkCount = 0; chunkIn = chunkOut = 0;
            }
        }
    }
    if (playing)
    {
        double bp = (ppq + numSamples * beatsPerSample - barStart) / barLenQ;
        barPhase.store ((float) (bp - std::floor (bp)));
    }
    else barPhase.store (-1.0f);
}

juce::AudioProcessorEditor* KickDuckProcessor::createEditor() { return new KickDuckEditor (*this); }

void KickDuckProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = params.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void KickDuckProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (params.state.getType())) params.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KickDuckProcessor(); }
