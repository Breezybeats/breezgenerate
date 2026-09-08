#include "Processor.h"
#include <iostream>
#include <stdexcept>
static void check(bool b, const char *m)
{
    if (!b)
        throw std::runtime_error(m);
}
int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        auto a = std::make_unique<NeuroProcessor>(), b = std::make_unique<NeuroProcessor>();
        a->setFamily(10);
        a->recallSeed(32761);
        a->setValue(nj::ColorOn, 0);
        a->setValue(nj::SpectralOn, 1);
        a->setValue(nj::Shift, 7);
        a->setValue(nj::Step3, .124f);
        a->lockGroup(nj::Mod, true);
        a->lockWave(true);
        juce::MemoryBlock data;
        a->getStateInformation(data);
        b->setStateInformation(data.getData(), int(data.getSize()));
        for (int i = 0; i < nj::Count; ++i)
            check(std::abs(a->value(i) - b->value(i)) < 1e-4f, "State parameter mismatch");
        auto pa = a->patchInfo(), pb = b->patchInfo();
        check(pa.seed == pb.seed && pa.waveSeed == pb.waveSeed && pa.locks == pb.locks &&
                  pa.lockWave == pb.lockWave,
              "Metadata mismatch");
        a->prepareToPlay(48000, 128);
        b->prepareToPlay(48000, 128);
        juce::AudioBuffer<float> ba(2, 128), bb(2, 128);
        juce::MidiBuffer ma, mb;
        for (int n = 0; n < 100; ++n)
        {
            ma.clear();
            mb.clear();
            if (n == 0)
            {
                ma.addEvent(juce::MidiMessage::noteOn(1, 36, .8f), 0);
                mb = ma;
            }
            if (n == 80)
            {
                ma.addEvent(juce::MidiMessage::noteOff(1, 36), 17);
                mb = ma;
            }
            a->processBlock(ba, ma);
            b->processBlock(bb, mb);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 128; ++i)
                    check(std::abs(ba.getSample(c, i) - bb.getSample(c, i)) < 1e-5f, "Audio recall mismatch");
        }
        check(a->getLatencySamples() == 512, "Latency mismatch");
        a->setFamily(6);
        a->recallSeed(111);
        auto first = a->patchInfo();
        a->recallSeed(222);
        check(a->undo(), "Undo failed");
        check(a->patchInfo().seed == first.seed, "Undo seed mismatch");
        check(a->redo(), "Redo failed");
        check(a->patchInfo().seed == 222, "Redo seed mismatch");
        for (int i = 0; i < 90; ++i)
            check(a->factory(i), "Factory recall failed");
        auto state = a->patchInfo();
        char junk[8] = {};
        a->setStateInformation(junk, 8);
        check(a->patchInfo().seed == state.seed, "Invalid state modified patch");
        std::cout << "PASS: full state and rendered audio recall, toggles, metadata, latency, undo/redo, 90 "
                     "factory entries, invalid state rejection\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
