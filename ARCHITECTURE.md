# NEUROJ 0.2 architecture

## Ownership and realtime boundary

`Parameters.h` defines stable IDs, valid ranges, defaults, grouping, and deterministic PRNG. `Generator.h` builds a fixed-size `Patch` and procedural `Tables` from constrained family recipes. Table generation runs on the non-audio UI/state side.

`Processor` owns APVTS host parameters, persistent metadata, a canonical wavetable bank, and a three-slot table mailbox. Non-audio producers are serialised by `stateMutex`; the callback never acquires it. The producer owns a back slot, the consumer owns a front slot, and an atomic exchange hands ownership through the middle slot. New generations can supersede unread generations without filling a queue. No table pointer is freed on the audio thread.

Continuous controls are atomically sampled at each block and smoothed per sample (15 ms one-pole); discrete controls switch without smoothing. A new bank crossfades from the previous bank over 15 ms. Multiple APVTS parameter notifications are not an atomic host-automation transaction; a concurrent host may observe a transient mixture during a Generate operation. Both parameter sets remain range-valid.

## Synthesis

Each of eight preallocated voices has A/B table oscillators, a sinusoidal C modulator, ADSR, two LFO phases, gun transient/burst state, one TPT filter, clean-sub high-pass and four fixed-buffer comb resonators. No audio samples are loaded. Table banks contain 2 oscillators × 4 frames × 6 harmonic-limited mip levels × 512 samples. Playback interpolates table samples and frames. Mip selection reduces source harmonic aliasing but does not remove aliasing introduced by nonlinear processing or deep FM.

The clean sine sub is summed on a separate mono path. Color feedback operates above a first-order crossover. Mid content passes through STFT processing when enabled, then a second first-order high-pass. Filtered echo provides stereo width. Width changes stereo echo timing rather than unison spread.

## STFT

Fixed 512-point radix-2 complex FFT, 128-sample hop, square-root Hann window for analysis and synthesis. Four overlapping windows use a 0.5 reconstruction factor. Positive-frequency complex bins are interpolated using shift and stretch, optionally blurred/tilted/gated, then mirrored conjugately for real output. Freeze holds the spectrum and advances bin phases each hop. This is deliberately a digital texture effect, not transparent pitch shifting.

Dry mid and sub both use a 512-sample ring delay. The host is always told 512 samples, even in bypassed spectral mode. This avoids changing the DAW's compensation during automation. A spectral enable may have a short warm-up as the overlap buffers fill; no latency-report changes occur.

## Generation calibration

Before publishing a generated patch, a temporary off-thread engine renders a fixed note for 8,192 samples at 48 kHz. Silent/nonfinite results are rejected. RMS after the 512-sample latency is used to choose a bounded 0.55–2x calibration gain targeting roughly 0.075 RMS at the reference output setting. That gain is saved and smoothed in playback; the Advanced calibration toggle can disable it. This is not perceptual LUFS normalisation, and later knob changes can change loudness.

## State storage

Version-2 XML-in-JUCE-binary contains APVTS values, generation seed, actual and selected family, WT seed, locks, and all actual table samples in base64 little-endian float32 format. Apple Silicon, Intel Mac and Windows targets are little-endian. Imported table lengths, finite values, bounds and schema version are checked before acceptance. Invalid presets leave the previous patch intact.

State recall reproduces subsequent deterministic retriggered notes within the same engine/build environment; it does not resume oscillators, delay tails or frozen spectral history from the middle of an already-held note. History uses 40 non-audio snapshots; it is not itself persisted.

## Validation layers

1. Portable C++ DSP tests run without JUCE.
2. JUCE processor tests compare all saved values and rendered post-restore audio.
3. Separate JUCE VST3 host scans/loads the compiled bundle, checks instrument/MIDI/latency, exercises all target rates and block sizes, and constructs/resizes the editor.
4. Manual FL Studio acceptance remains required on the user's Mac.

Only layers actually run locally are recorded as passed in `TEST-RESULTS.md`; layers 2 and 3 are wired into CI, not assumed to have succeeded.
