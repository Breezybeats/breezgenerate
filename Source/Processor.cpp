#include "Processor.h"
#include "Calibration.h"
#include "Editor.h"
#include <cstring>
using namespace nj;
juce::AudioProcessorValueTreeState::ParameterLayout NeuroProcessor::layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    for (int i = 0; i < Count; ++i)
    {
        const auto &s = specs[i];
        juce::NormalisableRange<float> range(s.lo, s.hi, s.integer ? 1.f : 0.f);
        if (i == Cutoff)
            range.setSkewForCentre(2000);
        if (i == Attack || i == Decay || i == Release || i == PitchDecay)
            range.setSkewForCentre(s.lo + (s.hi - s.lo) * .2f);
        if (s.integer && s.hi == 1)
            result.add(
                std::make_unique<juce::AudioParameterBool>(juce::ParameterID{s.id, 1}, s.name, s.def > .5f));
        else
            result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{s.id, 1}, s.name, range,
                                                                   s.def));
    }
    return result;
}
NeuroProcessor::NeuroProcessor()
    : AudioProcessor(BusesProperties().withOutput("Stereo", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "PARAMETERS", layout()), engine(std::make_unique<Engine>())
{
    for (int i = 0; i < Count; ++i)
        raw[i] = params.getRawParameterValue(specs[i].id);
    stored = generate(stored, 839472, 24);
    storedTables.build(stored.waveSeed);
    calibrate(stored, storedTables);
    calibrationGain.store(stored.calibration);
    commitValues(stored.v);
    engine->tables = storedTables;
    for (auto &s : scope)
        s.store(0);
    checkpointLocked();
    setLatencySamples(Engine::latency);
}
float NeuroProcessor::value(int i) const
{
    return raw[size_t(i)]->load(std::memory_order_relaxed);
}
void NeuroProcessor::readValues(Values &v) const
{
    for (int i = 0; i < Count; ++i)
        v[i] = value(i);
    sanitise(v);
}
void NeuroProcessor::setValue(int i, float v)
{
    auto *p = params.getParameter(specs[i].id);
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1(clamp(v, specs[i].lo, specs[i].hi)));
    p->endChangeGesture();
}
void NeuroProcessor::commitValues(const Values &v)
{
    for (int i = 0; i < Count; ++i)
        setValue(i, v[i]);
}
void NeuroProcessor::prepareToPlay(double sr, int)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    engine->prepare(float(sr));
    engine->tables = storedTables;
    Values v;
    readValues(v);
    engine->set(v, true);
    auditionActive = false;
    setLatencySamples(Engine::latency);
}
bool NeuroProcessor::isBusesLayoutSupported(const BusesLayout &l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
           l.getMainInputChannelSet().isDisabled();
}
void NeuroProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto start = juce::Time::getHighResolutionTicks();
    buffer.clear();
    if (tableMiddle.load(std::memory_order_acquire) & 4u)
    {
        tableFront = tableMiddle.exchange(tableFront, std::memory_order_acq_rel) & 3u;
        engine->replaceTables(tableQueue[tableFront]);
    }
    Values v;
    readValues(v);
    engine->set(v);
    engine->calibration = calibrationGain.load(std::memory_order_relaxed);
    if (auto *head = getPlayHead())
        if (auto pos = head->getPosition())
            if (auto tempo = pos->getBpm())
                engine->bpm = clamp(float(*tempo), 20, 400);
    bool want = auditionRequested.load();
    if (want != auditionActive)
    {
        if (want)
            engine->noteOn(36, .85f);
        else
            engine->noteOff(36);
        auditionActive = want;
    }
    auto event = midi.cbegin();
    float peak = 0;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        while (event != midi.cend() && (*event).samplePosition <= i)
        {
            if ((*event).numBytes > 3)
            {
                ++event;
                continue;
            }
            const auto msg = (*event).getMessage();
            if (msg.isNoteOn())
                engine->noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            else if (msg.isNoteOff())
                engine->noteOff(msg.getNoteNumber());
            else if (msg.isPitchWheel())
                engine->bend = (msg.getPitchWheelValue() - 8192) / 8192.f;
            else if (msg.isChannelPressure())
                engine->pressure = msg.getChannelPressureValue() / 127.f;
            else if (msg.isAftertouch())
                engine->pressure = msg.getAfterTouchValue() / 127.f;
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                engine->panic();
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1)
                    engine->wheel = msg.getControllerValue() / 127.f;
                else if (msg.getControllerNumber() == 64)
                    engine->pedal(msg.getControllerValue() >= 64);
            }
            ++event;
        }
        auto sample = engine->tick();
        for (int c = 0; c < std::min(2, buffer.getNumChannels()); ++c)
        {
            buffer.setSample(c, i, sample[c]);
            peak = std::max(peak, std::abs(sample[c]));
        }
        if (++scopeDecimate >= 64)
        {
            scopeDecimate = 0;
            scope[size_t(scopePosition)].store(sample[0], std::memory_order_relaxed);
            scopePosition = (scopePosition + 1) % 256;
        }
    }
    midi.clear();
    meter.store(peak);
    const double elapsed =
        juce::Time::highResolutionTicksToSeconds(juce::Time::getHighResolutionTicks() - start);
    if (buffer.getNumSamples() > 0)
        cpu.store(float(elapsed * getSampleRate() / buffer.getNumSamples() * 100));
}
bool NeuroProcessor::enqueueTables(const Tables &t)
{
    tableQueue[tableBack] = t;
    tableBack = tableMiddle.exchange(tableBack | 4u, std::memory_order_acq_rel) & 3u;
    return true;
}
Patch NeuroProcessor::patchInfo()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    auto p = stored;
    readValues(p.v);
    return p;
}
void NeuroProcessor::setFamily(int f)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    stored.selection = std::clamp(f, 0, 29);
}
void NeuroProcessor::lockGroup(int g, bool on)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (g >= 0 && g < GroupCount)
        stored.locks[g] = on;
}
void NeuroProcessor::lockWave(bool on)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    stored.lockWave = on;
}
void NeuroProcessor::lockSeed(bool on)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    stored.lockSeed = on;
}
bool NeuroProcessor::commitPatch(const Patch &p)
{
    auto next = std::make_unique<Tables>();
    if (p.waveSeed == stored.waveSeed)
        *next = storedTables;
    else
        next->build(p.waveSeed);
    auto validated = p;
    if (!calibrate(validated, *next))
        return false;
    if (historyIndex >= 0)
        history[size_t(historyIndex)] = snapshotLocked();
    if (!enqueueTables(*next))
        return false;
    stored = validated;
    calibrationGain.store(stored.calibration);
    storedTables = *next;
    commitValues(p.v);
    checkpointLocked();
    return true;
}
bool NeuroProcessor::makePatch(bool mutate)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    auto base = stored;
    readValues(base.v);
    const uint32_t seed =
        base.lockSeed ? base.seed
                      : uint32_t(juce::Time::getHighResolutionTicks() ^ (++randomSerial * 2654435761u));
    return commitPatch(generate(base, seed, base.selection, mutate));
}
bool NeuroProcessor::recallSeed(uint32_t seed)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    auto base = stored;
    readValues(base.v);
    return commitPatch(generate(base, seed, base.selection));
}
bool NeuroProcessor::newWavetable()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (stored.lockWave)
        return false;
    auto p = stored;
    readValues(p.v);
    p.waveSeed = RNG(uint32_t(++randomSerial) ^ uint32_t(juce::Time::getHighResolutionTicks())).next();
    return commitPatch(p);
}
bool NeuroProcessor::randomRhythm()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (stored.locks[Mod])
        return false;
    auto p = stored;
    readValues(p.v);
    RNG r(uint32_t(++randomSerial) ^ uint32_t(juce::Time::getHighResolutionTicks()));
    for (int i = Step0; i <= Step15; ++i)
        p.v[i] = (i - Step0) % 4 == 0 ? 1 : r.range(.1f, 1);
    p.v[RhythmOn] = 1;
    return commitPatch(p);
}
bool NeuroProcessor::factory(int i)
{
    if (i < 0 || i >= 90)
        return false;
    std::lock_guard<std::mutex> lock(stateMutex);
    Patch p;
    p.v[Brightness] = .35f + .25f * (i / 30);
    p.v[Destruction] = .4f + .2f * (i / 30);
    return commitPatch(generate(p, 100000u + uint32_t(i) * 7919u, 1 + i % 29));
}
juce::ValueTree NeuroProcessor::snapshotLocked()
{
    auto root = juce::ValueTree("NEUROJ");
    root.setProperty("version", 2, nullptr);
    root.setProperty("seed", juce::String(stored.seed), nullptr);
    root.setProperty("waveSeed", juce::String(stored.waveSeed), nullptr);
    root.setProperty("family", stored.family, nullptr);
    root.setProperty("calibration", stored.calibration, nullptr);
    root.setProperty("selection", stored.selection, nullptr);
    root.setProperty("lockWave", stored.lockWave, nullptr);
    root.setProperty("lockSeed", stored.lockSeed, nullptr);
    for (int i = 0; i < GroupCount; ++i)
        root.setProperty("lock" + juce::String(i), stored.locks[i], nullptr);
    root.addChild(params.copyState(), -1, nullptr);
    juce::MemoryBlock bytes(storedTables.data.data(), storedTables.data.size() * sizeof(float));
    root.setProperty("wavetableLE32", bytes.toBase64Encoding(), nullptr);
    return root;
}
bool NeuroProcessor::restoreLocked(const juce::ValueTree &root)
{
    if (!root.hasType("NEUROJ") || int(root.getProperty("version", 0)) != 2)
        return false;
    auto tree = root.getChildWithName("PARAMETERS");
    if (!tree.isValid())
        return false;
    Patch p;
    p.seed = uint32_t(root.getProperty("seed").toString().getLargeIntValue());
    p.waveSeed = uint32_t(root.getProperty("waveSeed").toString().getLargeIntValue());
    p.family = std::clamp(int(root.getProperty("family", 1)), 0, 29);
    p.calibration = clamp(float(root.getProperty("calibration", 1)), .55f, 2);
    p.selection = std::clamp(int(root.getProperty("selection", p.family)), 0, 29);
    p.lockWave = bool(root.getProperty("lockWave", false));
    p.lockSeed = bool(root.getProperty("lockSeed", false));
    for (int i = 0; i < GroupCount; ++i)
        p.locks[i] = bool(root.getProperty("lock" + juce::String(i), false));
    for (int i = 0; i < Count; ++i)
    {
        auto child = tree.getChildWithProperty("id", specs[i].id);
        if (child.isValid())
            p.v[i] = float(child.getProperty("value", specs[i].def));
    }
    sanitise(p.v);
    auto table = std::make_unique<Tables>();
    juce::MemoryBlock bytes;
    if (!bytes.fromBase64Encoding(root.getProperty("wavetableLE32").toString()) ||
        bytes.getSize() != table->data.size() * sizeof(float))
        return false;
    std::memcpy(table->data.data(), bytes.getData(), bytes.getSize());
    for (auto x : table->data)
        if (!std::isfinite(x) || std::abs(x) > 1.01f)
            return false;
    if (!enqueueTables(*table))
        return false;
    stored = p;
    calibrationGain.store(p.calibration);
    storedTables = *table;
    commitValues(p.v);
    return true;
}
void NeuroProcessor::checkpointLocked()
{
    if (historyIndex >= 0)
        history.resize(size_t(historyIndex + 1));
    history.push_back(snapshotLocked());
    if (history.size() > 40)
        history.erase(history.begin());
    historyIndex = int(history.size()) - 1;
}
bool NeuroProcessor::undo()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (historyIndex <= 0)
        return false;
    history[size_t(historyIndex)] = snapshotLocked();
    if (!restoreLocked(history[size_t(historyIndex - 1)]))
        return false;
    --historyIndex;
    return true;
}
bool NeuroProcessor::redo()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (historyIndex + 1 >= int(history.size()))
        return false;
    if (!restoreLocked(history[size_t(historyIndex + 1)]))
        return false;
    ++historyIndex;
    return true;
}
void NeuroProcessor::getStateInformation(juce::MemoryBlock &dest)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    auto xml = snapshotLocked().createXml();
    copyXmlToBinary(*xml, dest);
}
void NeuroProcessor::setStateInformation(const void *data, int size)
{
    if (size <= 0 || size > 4 * 1024 * 1024)
        return;
    auto xml = getXmlFromBinary(data, size);
    if (!xml)
        return;
    std::lock_guard<std::mutex> lock(stateMutex);
    if (restoreLocked(juce::ValueTree::fromXml(*xml)))
        checkpointLocked();
}
bool NeuroProcessor::savePreset(const juce::File &f)
{
    juce::MemoryBlock data;
    getStateInformation(data);
    return f.replaceWithData(data.getData(), data.getSize());
}
bool NeuroProcessor::loadPreset(const juce::File &f)
{
    if (f.getSize() > 4 * 1024 * 1024)
        return false;
    juce::MemoryBlock data;
    if (!f.loadFileAsData(data))
        return false;
    auto xml = getXmlFromBinary(data.getData(), int(data.getSize()));
    if (!xml)
        return false;
    std::lock_guard<std::mutex> lock(stateMutex);
    if (!restoreLocked(juce::ValueTree::fromXml(*xml)))
        return false;
    checkpointLocked();
    return true;
}
juce::AudioProcessorEditor *NeuroProcessor::createEditor()
{
    return new NeuroEditor(*this);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new NeuroProcessor;
}
