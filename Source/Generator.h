#pragma once
#include "Parameters.h"
namespace nj
{
struct Patch
{
    Values v = defaults();
    uint32_t seed = 839472, waveSeed = 1234;
    int family = 1, selection = 1;
    float calibration = 1;
    std::array<bool, GroupCount> locks{};
    bool lockWave = false, lockSeed = false;
};
inline Patch generate(Patch base, uint32_t seed, int type, bool mutate = false)
{
    RNG r(seed);
    Patch p = base;
    p.seed = seed;
    p.selection = std::clamp(type, 0, 29);
    p.family = type == 0 ? 1 + r.pick(29) : std::clamp(type, 1, 29);
    auto &v = p.v;
    auto set = [&](Param id, float x)
    {
        if (!base.locks[specs[id].group])
            v[id] = x;
    };
    if (mutate)
    {
        for (int i = WtA; i < Count; ++i)
            if (specs[i].group != Global && !base.locks[specs[i].group] && !specs[i].integer)
                v[i] += r.range(-1, 1) * base.v[Mutation] * (specs[i].hi - specs[i].lo);
    }
    else
    {
        const float bright = base.v[Brightness], destroy = base.v[Destruction], complex = base.v[Complexity],
                    atonal = base.v[Atonality];
        for (int i = WtA; i < Count; ++i)
            if (specs[i].group != Global && !base.locks[specs[i].group])
                v[i] = specs[i].def;
        set(WtA, r.unit());
        set(WtB, r.unit());
        set(Blend, r.range(.15f, .55f));
        set(Detune, r.range(1, 15));
        set(FMAmount, r.range(.3f, 1.6f) + destroy * 1.6f);
        set(Ratio, atonal > r.unit() ? r.range(.5f, 10.f) : float(1 + r.pick(5)));
        set(PMAmount, r.range(.1f, 1.f) * complex);
        set(Noise, r.range(.01f, .14f));
        set(SubLevel, r.range(.5f, .8f));
        set(Cutoff, 500 + bright * bright * 10500 + r.range(0, 1300));
        set(Resonance, r.range(.6f, 1.5f));
        set(Drive, 1.5f + destroy * 8);
        set(Fold, r.unit() * destroy * .7f);
        set(LFOSpeed, float(1 << r.pick(3)));
        set(LFO2Speed, r.pick(2) ? .5f : 1.5f);
        set(LFOShape, float(r.pick(5)));
        set(ModFM, r.range(.2f, .85f) * complex);
        set(ModFilter, r.range(.6f, 3.f) * complex);
        set(ModWT, r.range(.1f, .8f) * complex);
        set(DelayMix, r.range(.03f, .2f));
        set(Release, r.range(.05f, .24f));
        const int f = p.family;
        const bool gun = f == 3 || f == 4 || f == 10 || f == 11 || f == 13;
        const bool color = f == 9 || f == 10 || f == 21 || f == 22 || f == 25 || f == 26;
        const bool spectral = f == 8 || f == 11 || f == 22 || f == 28;
        set(ColorOn, color ? 1.f : 0.f);
        set(SpectralOn, spectral ? 1.f : 0.f);
        if (gun)
        {
            set(Attack, .001f);
            set(Decay, r.range(.04f, .14f));
            set(Sustain, .12f);
            set(PitchDrop, r.range(18, 42));
            set(PitchDecay, r.range(.015f, .08f));
            set(FMAmount, r.range(2.5f, 4.5f));
            set(Noise, .25f);
            set(Drive, 10);
            set(BurstRate, f == 4 ? 8 : 4);
            set(DelayMix, .025f);
            set(Release, .06f);
        }
        if (f == 2)
        {
            set(Cutoff, 12000);
            set(Ratio, 8);
            set(FMAmount, 3);
            set(Resonance, 2);
            set(Fold, .7f);
        }
        if (f == 5 || f == 14 || f == 19)
        {
            set(FilterMode, 1);
            set(Cutoff, 650);
            set(Resonance, 3);
            set(ModFilter, 3.4f);
            set(FMAmount, 1.8f);
        }
        if (f == 6)
        {
            set(Detune, r.range(15, 32));
            set(FMAmount, .15f);
            set(Blend, .5f);
            set(FilterMode, 2);
            set(Cutoff, 900);
            set(ModFilter, 1.8f);
            set(Attack, .025f);
        }
        if (f == 7 || f == 12 || f == 25)
        {
            set(ModFilter, 4);
            set(Cutoff, 600);
            set(LFOShape, f == 12 ? 3 : 0);
            set(LFOSpeed, 2);
        }
        if (f == 15 || f == 16 || f == 20)
        {
            set(Ratio, r.range(3.1f, 8.9f));
            set(FMAmount, 4.4f);
            set(PMAmount, 1.5f);
            set(Fold, .7f);
        }
        if (f == 17 || f == 23 || f == 24 || f == 27 || f == 28)
        {
            set(RhythmOn, 1);
            set(LFOShape, 4);
            set(LFOSpeed, f == 27 ? 4 : 2);
            set(PMAmount, 1);
            set(ModWT, .7f);
            set(FMAmount, 2.8f);
        }
        if (f == 18)
        {
            set(Sustain, 1);
            set(Attack, .025f);
            set(Release, .3f);
            set(LFOSpeed, .5f);
        }
        if (f == 13)
        {
            set(PitchDrop, 48);
            set(PMAmount, 0);
            set(Noise, .03f);
        }
        if (color)
        {
            set(Chord, float(r.pick(6)));
            set(ColorAmount, .8f);
            set(ColorDecay, r.range(.08f, .25f));
            set(Ratio, 2);
            set(Drive, 3.5f);
            set(PitchDrop, gun ? 18 : 0);
        }
        if (spectral)
        {
            set(Shift, r.range(-5, 12));
            set(Stretch, r.range(.7f, 1.65f));
            set(Blur, r.range(.05f, .4f));
            set(SpectralMix, .75f);
        }
        if (f == 29)
        {
            set(ColorOn, 1);
            set(SpectralOn, 1);
            set(ColorAmount, .4f);
            set(SpectralMix, .4f);
            set(RhythmOn, 1);
        }
        for (int i = Step0; i <= Step15; ++i)
            set(Param(i), (i - Step0) % 4 == 0 ? 1.f : r.range(.15f, 1));
        if (!p.lockWave)
            p.waveSeed = r.next();
    }
    sanitise(v);
    return p;
}
// Bank data is reconstructed only on non-audio threads. Six mip levels cap harmonics.
struct Tables
{
    static constexpr int size = 512, frames = 4, levels = 6;
    std::array<float, 2 * frames * levels * size> data{};
    void build(uint32_t seed)
    {
        RNG r(seed);
        for (int osc = 0; osc < 2; ++osc)
            for (int frame = 0; frame < frames; ++frame)
            {
                std::array<float, 32> amp{}, phase{};
                for (int h = 1; h <= 32; ++h)
                {
                    amp[h - 1] = (h == 1 ? 1.f : r.range(-1, 1)) / std::pow(float(h), .65f + .25f * frame);
                    phase[h - 1] = r.unit() * tau;
                }
                for (int level = 0; level < levels; ++level)
                {
                    float peak = .01f;
                    auto offset = ((osc * frames + frame) * levels + level) * size;
                    for (int i = 0; i < size; ++i)
                    {
                        float sum = 0;
                        for (int h = 1; h <= (32 >> level); ++h)
                            sum += amp[h - 1] * std::sin(tau * h * i / size + phase[h - 1]);
                        data[offset + i] = sum;
                        peak = std::max(peak, std::abs(sum));
                    }
                    for (int i = 0; i < size; ++i)
                        data[offset + i] /= peak;
                }
            }
    }
    float read(int osc, float phase, float pos, float frequency, float sr) const
    {
        int level = std::clamp(int(std::ceil(std::log2(std::max(1.f, 64 * frequency / sr)))), 0, levels - 1);
        float frame = clamp(pos) * (frames - 1);
        int a = int(frame), b = std::min(a + 1, frames - 1);
        phase -= std::floor(phase);
        float idx = phase * size;
        int i = int(idx) % size, j = (i + 1) % size;
        float frac = idx - int(idx);
        auto at = [&](int f)
        {
            int o = ((osc * frames + f) * levels + level) * size;
            return data[o + i] + frac * (data[o + j] - data[o + i]);
        };
        return at(a) + (frame - a) * (at(b) - at(a));
    }
};
} // namespace nj
