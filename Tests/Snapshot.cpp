// Runs audio through the real processor with a fake host transport, checks stereo + clicks,
// then renders the editor to a PNG (for previews without a DAW).
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"

struct FakeHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 130;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setIsPlaying (true); p.setBpm (bpm); p.setPpqPosition (ppq);
        p.setPpqPositionOfLastBarStart (std::floor (ppq / 4.0) * 4.0);
        p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String out = argc > 1 ? argv[1] : "instantpump.png";
    const double sr = 48000; const int bs = 512;

    KickDuckProcessor proc;
    FakeHead head;
    proc.setPlayHead (&head);
    proc.setPlayConfigDetails (2, 2, sr, bs);
    proc.prepareToPlay (sr, bs);
    if (argc > 2) proc.params.getParameter ("shape")->setValueNotifyingHost ((float) juce::String (argv[2]).getIntValue() / 3.f);

    // --- 1) click + stereo test: L = constant 1.0 (output == gain), R = constant 0.5
    double maxStep = 0, prevL = -1; bool stereoOk = true;
    juce::AudioBuffer<float> buf (2, bs); juce::MidiBuffer midi;
    for (int b = 0; b < 400; ++b)
    {
        for (int i = 0; i < bs; ++i) { buf.setSample (0, i, 1.0f); buf.setSample (1, i, 0.5f); }
        proc.processBlock (buf, midi);
        for (int i = 0; i < bs; ++i)
        {
            const double l = buf.getSample (0, i), r = buf.getSample (1, i);
            if (std::abs (r - 0.5 * l) > 1e-5) stereoOk = false;
            if (prevL >= 0 && b > 10) maxStep = std::max (maxStep, std::abs (l - prevL));
            prevL = l;
        }
        head.ppq += bs * head.bpm / (60.0 * sr);
    }
    const double perMs = maxStep * sr / 1000.0;
    std::printf ("stereo channels kept separate: %s\nlargest gain change: %.1f%% per ms\n", stereoOk ? "yes" : "NO", perMs * 100);

    // --- 2) musical signal for the waveform picture: wobbling saw bass
    double ph = 0;
    for (int b = 0; b < 260; ++b)
    {
        for (int i = 0; i < bs; ++i)
        {
            const double t = (b * bs + i) / sr;
            const double amp = 0.72 * (0.82 + 0.18 * std::sin (t * 2 * juce::MathConstants<double>::pi * 3.1));
            ph += 55.0 / sr; ph -= std::floor (ph);
            const float s = (float) (amp * (2 * ph - 1));
            buf.setSample (0, i, s); buf.setSample (1, i, s);
        }
        proc.processBlock (buf, midi);
        head.ppq += bs * head.bpm / (60.0 * sr);
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    ed->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
    ed.reset();
    return (stereoOk && perMs <= 0.2) ? 0 : 1;
}
