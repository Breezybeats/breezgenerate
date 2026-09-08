#include <iostream>
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <stdexcept>
static void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
int main(int argc, char **argv)
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        check(argc == 2, "Provide the compiled NEUROJ.vst3 path");
        juce::AudioPluginFormatManager formats;
        formats.addDefaultFormats();
        juce::OwnedArray<juce::PluginDescription> descriptions;
        for (int i = 0; i < formats.getNumFormats(); ++i)
            formats.getFormat(i)->findAllTypesForFile(descriptions, argv[1]);
        check(descriptions.size() > 0, "VST3 scan returned no instruments");
        check(descriptions[0]->isInstrument, "Not reported as instrument");
        juce::String error;
        auto instance = formats.createPluginInstance(*descriptions[0], 48000, 128, error);
        if (!instance)
            throw std::runtime_error(error.toStdString());
        check(instance->acceptsMidi(), "No MIDI input");
        check(instance->getLatencySamples() == 512, "Host latency not reported");
        for (double rate : {44100., 48000., 88200., 96000.})
            for (int block : {32, 64, 128, 256, 512, 1024, 2048})
            {
                instance->setRateAndBufferSizeDetails(rate, block);
                instance->prepareToPlay(rate, block);
                juce::AudioBuffer<float> buffer(2, block);
                juce::MidiBuffer midi;
                double energy = 0;
                for (int b = 0; b < 40; ++b)
                {
                    buffer.clear();
                    midi.clear();
                    if (b == 0)
                        midi.addEvent(juce::MidiMessage::noteOn(1, 36, .8f), 0);
                    if (b == 30)
                        midi.addEvent(juce::MidiMessage::noteOff(1, 36), block / 2);
                    instance->processBlock(buffer, midi);
                    for (int c = 0; c < 2; ++c)
                        for (int s = 0; s < block; ++s)
                        {
                            float v = buffer.getSample(c, s);
                            check(std::isfinite(v), "Nonfinite host audio");
                            check(std::abs(v) < .914f, "Host safety exceeded");
                            energy += v * v;
                        }
                }
                check(energy > 1e-7, "Silent VST3");
                instance->releaseResources();
            }
        juce::MemoryBlock saved;
        instance->getStateInformation(saved);
        check(saved.getSize() > 100000, "Wavetable state absent");
        instance->setStateInformation(saved.getData(), int(saved.getSize()));
        auto *editor = instance->createEditorIfNeeded();
        check(editor != nullptr, "Editor creation failed");
        editor->setSize(1200, 850);
        instance->editorBeingDeleted(editor);
        delete editor;
        std::cout << "PASS: compiled VST3 scan/load, instrument flag, MIDI, latency, sample rates/block "
                     "sizes, finite audio, state roundtrip, editor construction/resizing\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
