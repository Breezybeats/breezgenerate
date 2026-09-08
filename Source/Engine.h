#pragma once
#include "Generator.h"
#include "Spectral.h"
namespace nj
{
struct SVF
{
    float ic1 = 0, ic2 = 0;
    float tick(float x, float g, float q, int mode)
    {
        float k = 1 / q, a = 1 / (1 + g * (g + k));
        float v1 = a * (ic1 + g * (x - ic2)), v2 = ic2 + g * v1;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        if (!std::isfinite(ic1) || !std::isfinite(ic2))
        {
            ic1 = ic2 = 0;
            return 0;
        }
        float hp = x - k * v1 - v2;
        return mode == 1 ? v1 : mode == 2 ? hp + v2 : v2;
    }
};
struct OnePole
{
    float z = 0;
    float low(float x, float a)
    {
        z += a * (x - z);
        if (!std::isfinite(z))
            z = 0;
        return z;
    }
    float high(float x, float a)
    {
        return x - low(x, a);
    }
};
struct Resonator
{
    std::array<float, 8192> buf{};
    int head = 0;
    float tick(float x, float delay, float feedback)
    {
        float pos = head - clamp(delay, 2, 8190);
        if (pos < 0)
            pos += 8192;
        int i = int(pos);
        float frac = pos - i;
        float y = buf[i] * (1 - frac) + buf[(i + 1) % 8192] * frac;
        buf[head] = std::tanh(x * .18f + y * clamp(feedback, 0, .995f));
        head = (head + 1) % 8192;
        return y;
    }
    void reset()
    {
        buf.fill(0);
        head = 0;
    }
};
struct Voice
{
    bool active = false, down = false, pedal = false;
    int note = 60, stage = 0, shot = 0;
    uint64_t age = 0;
    float velocity = 0, env = 0, releaseStart = 0, phaseA = 0, phaseB = 0, phaseC = 0, phaseSub = 0,
          hz = 130.81f;
    float lfo1 = 0, lfo2 = 0, transient = 0, burst = 0, beat = 0, previous = 0;
    RNG rng{1};
    SVF filter;
    OnePole highpass;
    std::array<Resonator, 4> colors;
    void start(int n, float vel, uint64_t serial, bool legato)
    {
        note = n;
        velocity = vel;
        down = true;
        pedal = false;
        age = serial;
        burst = 0;
        shot = 0;
        transient = 0;
        if (!legato || !active)
        {
            stage = 1;
            env = 0;
            phaseA = phaseB = phaseC = phaseSub = lfo1 = lfo2 = beat = 0;
            hz = 440 * std::pow(2.f, (n - 69) / 12.f);
            rng = RNG(uint32_t(n * 787 + 123));
            filter = {};
            highpass = {};
            for (auto &c : colors)
                c.reset();
        }
        active = true;
    }
    void stop()
    {
        down = false;
        pedal = false;
        releaseStart = env;
        stage = 4;
    }
    void tickEnvelope(const Values &p, float sr)
    {
        switch (stage)
        {
        case 1:
            env += 1 / (p[Attack] * sr);
            if (env >= 1)
            {
                env = 1;
                stage = 2;
            }
            break;
        case 2:
            env -= (1 - p[Sustain]) / (p[Decay] * sr);
            if (env <= p[Sustain])
            {
                env = p[Sustain];
                stage = 3;
            }
            break;
        case 3:
            env = p[Sustain];
            break;
        case 4:
            env -= releaseStart / (p[Release] * sr);
            if (env <= 0)
            {
                env = 0;
                active = false;
                stage = 0;
            }
            break;
        default:
            break;
        }
    }
    float lfo(float phase, int shape) const
    {
        switch (shape)
        {
        case 0:
            return std::sin(tau * phase);
        case 1:
            return 1 - 4 * std::abs(phase - .5f);
        case 2:
            return 2 * phase - 1;
        case 3:
            return phase < .5f ? 1.f : -1.f;
        default:
            return float(int(phase * 8)) / 3.5f - 1;
        }
    }
    void tick(const Values &p, const Tables &t, const Tables &old, float tableFade, float sr, float bpm,
              float bend, float wheel, float pressure, float &mid, float &sub)
    {
        if (!active)
            return;
        tickEnvelope(p, sr);
        const float beatInc = bpm / (60 * sr);
        if (p[BurstRate] > .01f && stage != 4)
        {
            burst += beatInc * p[BurstRate];
            if (burst >= 1)
            {
                burst -= std::floor(burst);
                ++shot;
                if (p[Shots] < .5f || shot < int(p[Shots]))
                {
                    transient = 0;
                    env = 0;
                    stage = 1;
                }
                else
                    stop();
            }
        }
        const float sync = p[Sync] > .5f ? bpm / 60 : 1;
        lfo1 += p[LFOSpeed] * sync / sr;
        lfo1 -= std::floor(lfo1);
        lfo2 += p[LFO2Speed] * sync / sr;
        lfo2 -= std::floor(lfo2);
        beat += beatInc;
        beat -= std::floor(beat / 4) * 4;
        float movement = lfo(lfo1, int(p[LFOShape])) * clamp(p[Motion] + wheel * .3f + pressure * .2f);
        float movement2 = std::sin(tau * lfo2), pitchEnv = std::exp(-transient / p[PitchDecay]);
        transient += 1 / sr;
        float target = 440 * std::pow(2.f, (note - 69 + bend * p[PitchBendRange]) / 12.f);
        hz += (target - hz) * (p[Glide] < .0001f ? 1 : 1 - std::exp(-1 / (sr * p[Glide])));
        float freq = std::min(hz * std::pow(2.f, p[PitchDrop] * pitchEnv / 12), sr * .2f);
        const float fB = freq * std::pow(2.f, p[Detune] / 1200), fC = std::min(freq * p[Ratio], sr * .4f);
        auto read = [&](int o, float phase, float pos, float freq)
        {
            float y = t.read(o, phase, pos, freq, sr);
            if (tableFade < 1)
                y = old.read(o, phase, pos, freq, sr) * (1 - tableFade) + y * tableFade;
            return y;
        };
        float c = std::sin(tau * phaseC);
        float b = read(1, phaseB + c * p[PMAmount] * .13f, clamp(p[WtB] + movement2 * p[ModWT] * .2f), fB);
        float fm = p[FMAmount] * (.2f + p[Aggression]) * (1 + movement * p[ModFM]) * (1 + pitchEnv * .3f);
        float a = read(0, phaseA + b * fm * .15f,
                       clamp(p[WtA] + movement * p[ModWT] * .35f + (p[Character] - .5f) * .4f), freq);
        float x = a * (1 - p[Blend]) + b * p[Blend];
        x += c * p[Metal] * .12f;
        x += (rng.unit() * 2 - 1) * p[Noise] * (.15f + .85f * pitchEnv) * (1 + p[Chaos] * movement2 * .3f);
        x = std::tanh(x * p[Drive] * (.4f + p[Aggression]));
        x = x * (1 - p[Fold]) + std::sin(x * 4) * p[Fold];
        float cutoff = clamp(p[Cutoff] * std::pow(2.f, movement * p[ModFilter]), 50, sr * .42f);
        x = filter.tick(x, std::tan(pi * cutoff / sr), p[Resonance], int(p[FilterMode]));
        float hi = highpass.high(x, 1 - std::exp(-tau * p[Crossover] / sr));
        if (p[CleanSub] > .5f)
            x = hi;
        if (p[ColorOn] > .5f)
        {
            static constexpr int chords[6][4] = {{0, 7, 12, 19}, {0, 3, 7, 12}, {0, 4, 7, 12},
                                                 {0, 3, 7, 10},  {0, 4, 7, 11}, {0, 2, 7, 12}};
            float sum = 0;
            for (int k = 0; k < 4; ++k)
            {
                float tone = std::max(120.f, hz * std::pow(2.f, (12 + chords[int(p[Chord])][k]) / 12.f));
                float delay = sr / tone * (1 + movement2 * p[ColorMotion] * .001f * (k - 1.5f));
                float fb = std::pow(.001f, 1 / (tone * p[ColorDecay]));
                sum += colors[k].tick(hi, delay, fb);
            }
            x = x * (1 - p[ColorAmount] * .5f) + sum * p[ColorAmount] * 1.2f;
        }
        float rhythm = p[RhythmOn] > .5f ? p[Step0 + std::min(15, int(beat * 4))] : 1;
        float gain = env * velocity * rhythm;
        mid += clamp(x, -8, 8) * gain * .4f;
        sub += std::sin(tau * phaseSub) * p[SubLevel] * p[Weight] * gain * .6f;
        phaseA += freq / sr;
        phaseB += fB / sr;
        phaseC += fC / sr;
        phaseSub += hz / sr;
        phaseA -= std::floor(phaseA);
        phaseB -= std::floor(phaseB);
        phaseC -= std::floor(phaseC);
        phaseSub -= std::floor(phaseSub);
    }
};
class Engine
{
  public:
    static constexpr int latency = SpectralProcessor::N;
    Values current = defaults(), target = defaults();
    Tables tables, oldTables;
    float tableFade = 1, calibration = 1, currentCalibration = 1;
    std::array<Voice, 8> voices;
    SpectralProcessor spectral;
    std::array<float, latency> dry{}, low{};
    std::array<float, 262144> delay{};
    OnePole delayHP, delayLP, postHP, leftDC, rightDC;
    int latencyPos = 0, delayPos = 0;
    float sr = 48000, bpm = 150, bend = 0, wheel = 0, pressure = 0, smooth = .002f;
    uint64_t serial = 0;
    bool sustain = false;
    std::array<float, 128> held{};
    std::array<uint64_t, 128> order{};
    void prepare(float rate)
    {
        sr = rate;
        smooth = 1 - std::exp(-1 / (sr * .015f));
        tables.build(1234);
        reset();
    }
    void reset()
    {
        for (auto &v : voices)
        {
            v.active = false;
            v.env = 0;
            v.stage = 0;
        }
        held.fill(0);
        order.fill(0);
        sustain = false;
        bend = wheel = pressure = 0;
        serial = 0;
        dry.fill(0);
        low.fill(0);
        delay.fill(0);
        latencyPos = delayPos = 0;
        spectral.reset();
        delayHP = {};
        delayLP = {};
        postHP = {};
        leftDC = {};
        rightDC = {};
    }
    void set(const Values &p, bool snap = false)
    {
        target = p;
        sanitise(target);
        if (snap)
            current = target;
    }
    void replaceTables(const Tables &t)
    {
        oldTables = tables;
        tables = t;
        tableFade = 0;
    }
    void noteOn(int note, float velocity)
    {
        if (note < 0 || note > 127)
            return;
        if (velocity <= 0)
        {
            noteOff(note);
            return;
        }
        held[note] = velocity;
        order[note] = ++serial;
        Voice *v = &voices[0];
        if (target[Mono] > .5f)
        {
            for (int i = 1; i < 8; ++i)
                voices[i].stop();
        }
        else
        {
            for (auto &candidate : voices)
                if (!candidate.active)
                {
                    v = &candidate;
                    break;
                }
                else if (candidate.age < v->age)
                    v = &candidate;
        }
        v->start(note, clamp(velocity), serial, target[Mono] > .5f && target[Glide] > .001f && v->down);
    }
    void noteOff(int note)
    {
        if (note < 0 || note > 127)
            return;
        held[note] = 0;
        if (target[Mono] > .5f && voices[0].note == note)
        {
            int last = -1;
            uint64_t newest = 0;
            for (int i = 0; i < 128; ++i)
                if (held[i] > 0 && order[i] > newest)
                {
                    last = i;
                    newest = order[i];
                }
            if (last >= 0)
            {
                voices[0].start(last, held[last], ++serial, true);
                return;
            }
        }
        for (auto &v : voices)
            if (v.active && v.note == note)
            {
                v.down = false;
                if (sustain)
                    v.pedal = true;
                else
                    v.stop();
            }
    }
    void pedal(bool on)
    {
        sustain = on;
        if (!on)
            for (auto &v : voices)
                if (v.pedal)
                    v.stop();
    }
    void panic()
    {
        held.fill(0);
        for (auto &v : voices)
            v.stop();
    }
    std::array<float, 2> tick()
    {
        for (int i = 0; i < Count; ++i)
            current[i] = specs[i].integer ? target[i] : current[i] + smooth * (target[i] - current[i]);
        tableFade = std::min(1.f, tableFade + 1 / (sr * .015f));
        float mid = 0, sub = 0;
        for (auto &v : voices)
            v.tick(current, tables, oldTables, tableFade, sr, bpm, bend, wheel, pressure, mid, sub);
        const auto &p = current;
        float delayed = dry[latencyPos], bass = low[latencyPos];
        dry[latencyPos] = mid;
        low[latencyPos] = sub;
        latencyPos = (latencyPos + 1) % latency;
        float spec = spectral.tick(mid, p, p[SpectralOn] > .5f);
        // Frozen timbre still follows MIDI amplitude; never leave a stuck drone after note-off.
        if (p[Freeze] > .5f)
        {
            float envelope = 0;
            for (const auto &voice : voices)
                envelope = std::max(envelope, voice.env);
            spec *= envelope;
        }
        float mix = p[SpectralOn] > .5f ? p[SpectralMix] : 0;
        float x = delayed * (1 - mix) + spec * mix;
        if (p[CleanSub] > .5f)
            x = postHP.high(x, 1 - std::exp(-tau * p[Crossover] / sr));
        float delaySamples = clamp(sr * 60 / bpm * p[DelayTime], 1, 262140);
        int offset = int(delaySamples);
        auto tap = [&](int n) { return delay[(delayPos - n + 262144) % 262144]; };
        float echoL = tap(offset), echoR = tap(std::max(1, offset - int(p[Width] * sr * .013f)));
        float feedback =
            delayLP.low(delayHP.high(x + echoL * p[DelayFeedback], 1 - std::exp(-tau * 200 / sr)),
                        1 - std::exp(-tau * 6500 / sr));
        delay[delayPos] = std::tanh(feedback);
        delayPos = (delayPos + 1) % 262144;
        currentCalibration += smooth * ((p[Normalise] > .5f ? calibration : 1.f) - currentCalibration);
        float wet = p[DelayMix] * (.15f + p[Space] * 1.5f),
              gain = std::pow(10.f, p[Output] / 20) * currentCalibration;
        float l = (x + echoL * wet + bass) * gain, r = (x + echoR * wet + bass) * gain;
        const float dc = 1 - std::exp(-tau * 12 / sr);
        l = leftDC.high(l, dc);
        r = rightDC.high(r, dc);
        if (!std::isfinite(l))
            l = 0;
        if (!std::isfinite(r))
            r = 0;
        l = clamp(l, -8, 8);
        r = clamp(r, -8, 8);
        if (p[SafeOutput] > .5f)
        {
            constexpr float ceiling = .9120108f;
            l = ceiling * std::tanh(l / ceiling);
            r = ceiling * std::tanh(r / ceiling);
        }
        return {l, r};
    }
};
} // namespace nj
