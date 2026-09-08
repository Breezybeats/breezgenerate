#pragma once
#include "Parameters.h"
#include <complex>
namespace nj
{
// Fixed-size radix-2 FFT and 75% overlap STFT; no allocation or locks.
class SpectralProcessor
{
  public:
    static constexpr int N = 512, H = 128, Ring = 2048;
    using C = std::complex<float>;
    std::array<float, N> input{}, window{};
    std::array<float, Ring> output{};
    std::array<C, N> bins{}, mapped{}, frozen{}, twiddle{};
    int write = 0;
    uint64_t samples = 0;
    bool wasFrozen = false;
    SpectralProcessor()
    {
        for (int i = 0; i < N; ++i)
        {
            window[i] = std::sqrt(.5f - .5f * std::cos(tau * i / N));
            twiddle[i] = std::polar(1.f, -tau * i / N);
        }
    }
    void reset()
    {
        input.fill(0);
        output.fill(0);
        frozen.fill({});
        write = 0;
        samples = 0;
        wasFrozen = false;
    }
    void fft(std::array<C, N> &x, bool inverse)
    {
        for (int i = 1, j = 0; i < N; ++i)
        {
            int bit = N >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(x[i], x[j]);
        }
        for (int len = 2; len <= N; len <<= 1)
            for (int i = 0; i < N; i += len)
                for (int j = 0; j < len / 2; ++j)
                {
                    C w = twiddle[j * N / len];
                    if (inverse)
                        w = std::conj(w);
                    C u = x[i + j], v = x[i + j + len / 2] * w;
                    x[i + j] = u + v;
                    x[i + j + len / 2] = u - v;
                }
        if (inverse)
            for (auto &v : x)
                v /= float(N);
    }
    float tick(float x, const Values &p, bool enabled)
    {
        float result = output[samples % Ring];
        output[samples % Ring] = 0;
        input[write] = x;
        write = (write + 1) % N;
        ++samples;
        if (samples % H == 0 && enabled)
        {
            for (int i = 0; i < N; ++i)
            {
                bins[i] = input[(write + i) % N] * window[i];
            }
            fft(bins, false);
            const bool freeze = p[Freeze] > .5f;
            if (freeze)
            {
                if (!wasFrozen)
                    frozen = bins;
                else
                    for (int k = 0; k <= N / 2; ++k)
                        frozen[k] *= std::polar(1.f, tau * k * H / N);
                bins = frozen;
            }
            wasFrozen = freeze;
            mapped.fill({});
            const float shift = p[Shift], stretch = p[Stretch], blur = p[Blur];
            for (int k = 1; k < N / 2; ++k)
            {
                float src = (k - shift) / stretch;
                if (src < 1 || src >= N / 2 - 1)
                    continue;
                int a = int(src);
                float f = src - a;
                C v = bins[a] * (1 - f) + bins[a + 1] * f;
                v = (1 - blur) * v + blur * (bins[a - 1] + bins[a] + bins[a + 1]) / 3.f;
                if (std::abs(v) < p[Gate] * N * .1f)
                    v = {};
                v *= std::pow(float(k) / 16.f, p[Tilt] * .5f);
                mapped[k] = v;
                mapped[N - k] = std::conj(v);
            }
            fft(mapped, true);
            for (int i = 0; i < N; ++i)
                output[(samples + i) % Ring] += mapped[i].real() * window[i] * .5f;
        }
        return std::isfinite(result) ? result : 0;
    }
};
} // namespace nj
