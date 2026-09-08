#pragma once
#include "Engine.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <mutex>
#include <vector>
class NeuroProcessor final : public juce::AudioProcessor
{
  public:
    NeuroProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override
    {
        return true;
    }
    const juce::String getName() const override
    {
        return "NEUROJ";
    }
    bool acceptsMidi() const override
    {
        return true;
    }
    bool producesMidi() const override
    {
        return false;
    }
    bool isMidiEffect() const override
    {
        return false;
    }
    double getTailLengthSeconds() const override
    {
        return 8;
    }
    int getNumPrograms() override
    {
        return 1;
    }
    int getCurrentProgram() override
    {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override
    {
        return "Current Patch";
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    bool makePatch(bool mutate = false);
    bool newWavetable();
    bool randomRhythm();
    bool recallSeed(uint32_t);
    bool factory(int index);
    bool undo();
    bool redo();
    void setFamily(int);
    void lockGroup(int, bool);
    void lockWave(bool);
    void lockSeed(bool);
    nj::Patch patchInfo();
    void setValue(int, float);
    float value(int) const;
    bool savePreset(const juce::File &);
    bool loadPreset(const juce::File &);
    void audition(bool on)
    {
        auditionRequested.store(on);
    }
    juce::AudioProcessorValueTreeState params;
    std::atomic<float> meter{0}, cpu{0};
    std::array<std::atomic<float>, 256> scope{};

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    std::array<std::atomic<float> *, nj::Count> raw{};
    std::unique_ptr<nj::Engine> engine;
    // Triple-buffer mailbox. UI/state producers are serialised by stateMutex.
    // The callback does not acquire that mutex, allocate, or release shared pointers.
    std::array<nj::Tables, 3> tableQueue;
    unsigned tableBack = 0, tableFront = 1;
    std::atomic<unsigned> tableMiddle{2}; // bit 2 marks new data; low bits select slot
    nj::Tables storedTables;
    std::atomic<float> calibrationGain{1};
    nj::Patch stored;
    std::mutex stateMutex;
    std::vector<juce::ValueTree> history;
    int historyIndex = -1;
    std::atomic<bool> auditionRequested{false};
    bool auditionActive = false;
    int scopePosition = 0, scopeDecimate = 0;
    uint64_t randomSerial = 1;
    bool enqueueTables(const nj::Tables &);
    void readValues(nj::Values &) const;
    void commitValues(const nj::Values &);
    bool commitPatch(const nj::Patch &);
    juce::ValueTree snapshotLocked();
    bool restoreLocked(const juce::ValueTree &);
    void checkpointLocked();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuroProcessor)
};
