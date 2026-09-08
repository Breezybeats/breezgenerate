#include "Calibration.h"
#include "Engine.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
static void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
static void u16(std::ofstream &f, uint16_t v)
{
    char b[2] = {char(v), char(v >> 8)};
    f.write(b, 2);
}
static void u32(std::ofstream &f, uint32_t v)
{
    u16(f, uint16_t(v));
    u16(f, uint16_t(v >> 16));
}
static void renderDemo(const char *path)
{
    std::vector<int16_t> samples;
    auto e = std::make_unique<nj::Engine>();
    e->prepare(48000);
    for (int family : {24, 3, 4, 9, 8, 6, 10, 29})
    {
        auto p = nj::generate(nj::Patch{}, 839472 + family * 17, family);
        e->reset();
        e->tables.build(p.waveSeed);
        nj::calibrate(p, e->tables);
        e->calibration = p.calibration;
        e->set(p.v, true);
        e->noteOn(36, .85f);
        for (int i = 0; i < 96000; ++i)
        {
            if (i == 72000)
                e->noteOff(36);
            auto y = e->tick();
            samples.push_back(int16_t(nj::clamp(y[0], -1, 1) * 32767));
            samples.push_back(int16_t(nj::clamp(y[1], -1, 1) * 32767));
        }
    }
    std::ofstream f(path, std::ios::binary);
    f.write("RIFF", 4);
    u32(f, uint32_t(36 + samples.size() * 2));
    f.write("WAVEfmt ", 8);
    u32(f, 16);
    u16(f, 1);
    u16(f, 2);
    u32(f, 48000);
    u32(f, 192000);
    u16(f, 4);
    u16(f, 16);
    f.write("data", 4);
    u32(f, uint32_t(samples.size() * 2));
    for (auto x : samples)
        u16(f, uint16_t(x));
}
int main(int argc, char **argv)
{
    try
    {
        using namespace nj;
        if (argc == 3 && std::string(argv[1]) == "--render")
        {
            renderDemo(argv[2]);
            return 0;
        }
        for (int family = 0; family < 30; ++family)
        {
            auto a = generate(Patch{}, 91231, family), b = generate(Patch{}, 91231, family);
            require(a.v == b.v && a.waveSeed == b.waveSeed, "Deterministic generation failed");
            for (int g = 0; g < GroupCount; ++g)
            {
                auto locked = a;
                locked.locks[g] = true;
                auto c = generate(locked, 77, 3);
                for (int i = 0; i < Count; ++i)
                    if (specs[i].group == g)
                        require(c.v[i] == a.v[i], "Section lock changed");
            }
            auto locked = a;
            locked.lockWave = true;
            require(generate(locked, 88, 4).waveSeed == locked.waveSeed, "WT lock changed");
        }
        auto tables = std::make_unique<Tables>();
        tables->build(123);
        auto tables2 = std::make_unique<Tables>();
        tables2->build(123);
        require(tables->data == tables2->data, "Tables not deterministic");
        for (float x : tables->data)
            require(std::isfinite(x) && std::abs(x) <= 1.001f, "Invalid wavetable");
        auto fft = std::make_unique<SpectralProcessor>();
        std::array<SpectralProcessor::C, SpectralProcessor::N> data{};
        for (int i = 0; i < 512; ++i)
            data[i] = std::sin(tau * i / 37);
        auto original = data;
        fft->fft(data, false);
        fft->fft(data, true);
        for (int i = 0; i < 512; ++i)
            require(std::abs(original[i] - data[i]) < 1e-4f, "FFT inverse mismatch");
        auto calibrationPatch = generate(Patch{}, 91231, 9);
        require(calibrate(calibrationPatch, *tables), "Calibration rejected valid patch");
        require(calibrationPatch.calibration >= .55f && calibrationPatch.calibration <= 2,
                "Calibration out of bounds");
        auto e = std::make_unique<Engine>();
        double lowest = 100, highest = 0;
        for (float rate : {44100.f, 48000.f, 88200.f, 96000.f})
        {
            e->prepare(rate);
            for (int f = 1; f < 30; ++f)
            {
                auto p = generate(Patch{}, 7391 + f, f);
                e->reset();
                e->tables.build(p.waveSeed);
                e->set(p.v, true);
                e->noteOn(36, .8f);
                double energy = 0;
                for (int i = 0; i < 8192; ++i)
                {
                    auto y = e->tick();
                    require(std::isfinite(y[0]) && std::isfinite(y[1]), "Nonfinite output");
                    require(std::abs(y[0]) <= .913f && std::abs(y[1]) <= .913f, "Unsafe level");
                    energy += y[0] * y[0];
                }
                require(energy > 1e-5, "Silent generated patch");
                double rms = std::sqrt(energy / 8192);
                lowest = std::min(lowest, rms);
                highest = std::max(highest, rms);
                e->noteOff(36);
                for (int i = 0; i < int(rate * 1.1f); ++i)
                    e->tick();
                require(!e->voices[0].active, "Stuck note");
            }
        }
        e->prepare(48000);
        auto p = generate(Patch{}, 99, 1);
        p.v[Glide] = .1f;
        e->set(p.v, true);
        e->noteOn(36, 1);
        e->noteOn(40, 1);
        e->noteOff(40);
        require(e->voices[0].note == 36, "Mono last-note fallback failed");
        e->pedal(true);
        e->noteOff(36);
        require(e->voices[0].pedal, "Sustain not held");
        e->pedal(false);
        require(e->voices[0].stage == 4, "Sustain not released");
        auto a = std::make_unique<Engine>(), b = std::make_unique<Engine>();
        a->prepare(48000);
        b->prepare(48000);
        a->set(p.v, true);
        b->set(p.v, true);
        a->noteOn(36, .8f);
        b->noteOn(36, .8f);
        for (int block : {32, 64, 128, 256, 512, 1024, 2048})
            for (int i = 0; i < block; ++i)
            {
                auto x = a->tick(), y = b->tick();
                require(x == y, "Buffer invariance mismatch");
            }
        auto bad = defaults();
        bad[Cutoff] = std::numeric_limits<float>::quiet_NaN();
        bad[FMAmount] = std::numeric_limits<float>::infinity();
        e->set(bad);
        for (int i = 0; i < 4096; ++i)
        {
            auto y = e->tick();
            require(std::isfinite(y[0]), "Parameter sanitisation failed");
        }
        e->prepare(48000);
        for (uint32_t seed = 1; seed <= 200; ++seed)
        {
            auto p = generate(Patch{}, seed * 7369, 0);
            e->reset();
            e->tables.build(p.waveSeed);
            e->set(p.v, true);
            e->noteOn(24 + int(seed % 36), .85f);
            double energy = 0;
            for (int i = 0; i < 4096; ++i)
            {
                auto y = e->tick();
                require(std::isfinite(y[0]) && std::abs(y[0]) <= .913f, "Random seed unsafe");
                energy += y[0] * y[0];
            }
            require(energy > 1e-6, "Random seed silent");
        }
        auto extreme = defaults();
        for (int i = 0; i < Count; ++i)
            extreme[i] = specs[i].hi;
        extreme[Mono] = 0;
        extreme[BurstRate] = 0;
        extreme[Freeze] = 0;
        extreme[Output] = 0;
        e->reset();
        e->set(extreme, true);
        for (int n : {12, 24, 36, 48, 60, 72, 96, 127})
            e->noteOn(n, 1);
        for (int i = 0; i < 8192; ++i)
        {
            auto y = e->tick();
            require(std::isfinite(y[0]) && std::isfinite(y[1]) && std::abs(y[0]) <= .913f,
                    "Extreme polyphonic patch unsafe");
        }
        auto identity = defaults();
        identity[Shift] = 0;
        identity[Stretch] = 1;
        identity[Blur] = 0;
        identity[Gate] = 0;
        identity[Tilt] = 0;
        fft->reset();
        double err = 0;
        for (int i = 0; i < 4096; ++i)
        {
            float x = std::sin(tau * 8 * i / 512), y = fft->tick(x, identity, true);
            if (i > 1024)
                err += std::abs(y - std::sin(tau * 8 * (i - 512) / 512));
        }
        require(err / 3071 < .001, "STFT latency/reconstruction failed");
        std::cout << "PASS: deterministic patches/tables; every section lock; FFT roundtrip and STFT "
                     "latency/reconstruction; 29 families x 4 rates; 200 extra seeds; extreme 8-voice patch; "
                     "RMS calibration; safe output; note-off; sustain; mono fallback; block partition "
                     "invariance; invalid parameter repair.\n";
        std::cout << "Generated-patch RMS range (test window, not perceptual normalisation): " << lowest
                  << " to " << highest << "\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
