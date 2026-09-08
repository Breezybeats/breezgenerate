# NEURO//J 0.2 — original bass generator

**Start with [START-HERE.md](START-HERE.md) to build the Mac M1 VST3 using GitHub.**

A real C++/JUCE instrument project with MIDI synthesis, constrained generation, mutation, locks, original procedural wavetables, dedicated clean sub, color chord resonators, real FFT spectral processing, gun bursts and full implemented-parameter automation.

This is a substantially rebuilt development release, not the earlier source ZIP. It is **not the full 80-section commercial-product specification**. See [FEATURE-COVERAGE.md](FEATURE-COVERAGE.md) for precisely what is implemented, partial, or absent. Do not equate a successful compiler run with FL Studio certification or professionally curated sound quality.

## Implemented signal path

Two four-frame, six-mip-level original wavetable banks → C-to-B PM and B-to-A FM → noise transient → drive/fold → TPT multimode filter → clean-sub crossover → MIDI-tuned color comb resonators → real FFT shift/stretch/blur/freeze/tilt/gate → filtered stereo delay → independent mono sub merge → DC block and safety saturation.

Eight macros, four generator biases, all 30 family labels (Auto + 29 families), 90 factory recipes, 16-step rhythm, two LFOs, sharp pitch/gun transients, mono last-note fallback or eight voices, glide, MIDI bend/mod wheel/pressure/sustain, preset save/load, 40 generation-history states, seeds and section locks.

## Build locally (optional)

Use a C++17 compiler and CMake 3.22+. JUCE 8.0.6 is downloaded automatically at configure time. No third-party presets, samples or wavetables are required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

Mac artifact: `build/NeuroJ_artefacts/Release/VST3/NEUROJ.vst3`.

Windows developer build: use Visual Studio 2022 C++ tools, run `cmake -S . -B build -A x64`, then `cmake --build build --config Release`. This path is not locally or CI tested in this delivery. The supplied automatic workflow targets **Mac M1 only**.

Independent DSP tests (no JUCE/developer GUI libraries required):

```sh
cmake -S . -B build-core -DNEUROJ_CORE_ONLY=ON
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

## Sound preview

`Audio-preview.wav` is rendered directly by this project's DSP engine. Each two-second segment is: Japanese Neuro, Tearout Gun, Machine Gun, Color, Spectral, Reese, Color + Tearout, Hybrid. It proves sound generation, not FL Studio compatibility or subjective quality acceptance.

## Source and dependencies

Project-specific source is provided under the MIT license in `LICENSE`. JUCE and its embedded dependencies retain their separate licenses. The workflow includes their license files in the artifact. See `THIRD-PARTY-NOTICES.md` before distributing a compiled product.
