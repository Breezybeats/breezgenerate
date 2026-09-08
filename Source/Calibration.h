#pragma once
#include "Engine.h"
#include <memory>
namespace nj
{
// Non-realtime only: render a standard MIDI note and estimate RMS.
// This is a bounded gain estimate, not LUFS measurement or quality judgement.
inline bool calibrate(Patch &p, const Tables &t)
{
    auto test = std::make_unique<Engine>();
    test->prepare(48000);
    test->tables = t;
    auto values = p.v;
    values[Output] = -7;
    values[Normalise] = 0;
    values[SafeOutput] = 0;
    test->set(values, true);
    test->noteOn(36, .85f);
    double sum = 0;
    for (int i = 0; i < 8192; ++i)
    {
        auto y = test->tick();
        if (!std::isfinite(y[0]) || !std::isfinite(y[1]))
            return false;
        if (i >= 512)
            sum += double(y[0]) * y[0];
    }
    const double rms = std::sqrt(sum / (8192 - 512));
    if (rms < 1e-7)
        return false;
    p.calibration = clamp(float(.075 / rms), .55f, 2.f);
    return true;
}
} // namespace nj
