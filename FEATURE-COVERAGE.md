# Honest coverage against the supplied 80-section brief

This is a functional source implementation with an automated Mac build route, not a claim that every requested module or the FL Studio acceptance test is complete. All exposed controls drive implemented code; there are no inactive controls standing in for future effects.

| Area | Implemented in 0.2 | Limits / remaining work |
| --- | --- | --- |
| VST3 instrument | JUCE instrument target, stereo output, MIDI input; standalone target | Mac binary must be compiled/tested by supplied GitHub workflow; no local Mac run |
| Generate / Mutate | Deterministic xorshift generator, dependent family ranges, four generation biases, bounded mutation; short offline signal check | Rejects silent/nonfinite calibration renders; no perceptual quality scoring or repeated regeneration loop |
| Families | Auto + all 29 named families, archetype-specific settings and hybrids | Related families share engine mechanisms; these are not 29 different synthesis algorithms |
| Procedural tables | Two original 4-frame x 512-sample banks, 6 harmonic-limited mip levels; NEW WT; actual table samples stored | 32 source harmonics maximum; no wavetable drawing/import editor |
| FM/PM | B→A FM, C→B PM, variable C ratio, audio-rate modulation | Not the requested arbitrary six-route FM/AM/ring matrix |
| Sub | Independent mono sine, phase reset, adjustable crossover, clean-sub bypass around color/spectral/delay | One-pole protection slopes; not a linear-phase or brick-wall multiband crossover; low-mid stereo can remain |
| Color | Four MIDI-tuned feedback comb resonators; six chord voicings, decay, motion, mix | No custom intervals, root/scale editor, vocoder or harmonic-remapping bank |
| Spectral | Actual radix-2 FFT/STFT, 512-point square-root-Hann analysis/synthesis, 128-sample hop, shift/stretch/blur/freeze/tilt/gate | One fixed quality mode; deliberately rough complex-bin mapping, not a pitch-transparent phase vocoder; no shuffle/harmonic snap/feedback controls |
| Latency | Constant 512 samples reported to host; dry mid/sub delayed to align | About 11.6 ms at 44.1 kHz, including when spectral is bypassed |
| Gun engine | Sharp ADSR, exponential pitch transient, transient FM/noise, tempo-synced shots, count/repeat | No seven-mode pitch-shape chooser or mechanical sampled transients |
| Modulation | Two LFOs with five LFO1 shapes, pitch envelope, amp ADSR, editable 16-step amplitude rhythm | Not four drawable LFOs, three envelopes, arbitrary modulation matrix or spectral step destinations |
| Tempo | Host BPM controls LFO sync, bursts and delay | Note-retriggered phase; no absolute host transport/PPQ phase alignment |
| Filters | TPT lowpass/bandpass/notch, resonance and moving cutoff | One main filter; “formant/talking” families use bandpass movement rather than a true vowel bank |
| Distortion / FX | Tanh saturation, sine fold, filtered feedback delay and stereo offset | No oversampling, full FX rack, reverb, phaser, frequency shifter, OTT-style compressor, multiband distortion or anti-alias guarantee under extreme FM/folding |
| MIDI | Notes/velocity, pitch bend, mod wheel, channel/poly pressure, sustain, mono last-held-note fallback, glide, eight voices | MIDI channels combined into one instrument; no MPE; no separate always/fingered glide switch; voice stealing may click |
| Safety / loudness | Bounded feedback, finite parameter/output checks, DC high-pass, default ~−0.8 dBFS sample-peak saturation; optional offline RMS calibration | Not a look-ahead/true-peak limiter or LUFS normaliser. Calibration uses one short reference note, bounded 0.55–2x gain, and does not track later knob automation. Start monitoring quietly |
| Main UI | Generate, Mutate, eight macros, four biases, color/spectral/sub/safety toggles, audition, waveform, CPU/peak displays | Minimal custom JUCE UI, not the full polished commercial design |
| Advanced | Scrollable complete implemented-parameter editor, eight section locks, WT lock | No ten-tab module editor, routing diagrams or editable LFO drawing |
| Presets | Save/load .neuroj, FL project state, actual wavetable data, seeds, locks, all parameter values | No search/favorites/tags/browser database; no A/B morph |
| Factory bank | 90 original deterministic parameter/table recipes in the selector | Not hand-curated or subjectively certified; related recipes may sound similar |
| History | 40 generation/WT/rhythm/preset states, Undo/Redo restores seed/tables/values | Not a universal undo manager for every individual knob drag; history itself not saved in FL projects |
| Recall | Parameter/table/seed/lock recall; voice phases reset on retrigger | Live held-note position, delay tails, STFT freeze history and GUI layout are not restored across closing a project |
| Seed sharing | Seed + same generator version, family selection, biases and unlocked starting context reproduces generation | For guaranteed cross-project recall share the .neuroj preset, not just a seed; locks/mutations depend on starting state |
| Realtime | No intentional allocation, file access, GUI access or mutex in custom sample processing; atomics/triple-buffer table delivery | JUCE host/MIDI wrapper behaviour still needs validation; no measured worst-case Mac CPU guarantee |
| Export / reference | Standalone DSP test executable renders preview WAV | No in-plugin WAV export or reference-audio analyzer |
| Release engineering | Pinned JUCE tarball hash, Mac arm64 CI, core/state/host smoke tests, ad-hoc signature check, source/notice packaging | Not notarised or Developer ID signed; no installer; Windows/universal Mac builds and FL Studio manual tests pending |

Priority for further development: get a successful Mac workflow and FL Studio acceptance run; collect listening feedback; improve oversampling/normalisation; then expand modulation/routing and remaining requested modules. No claim is made that 70–90% of generated patches are professionally usable without listening tests.
