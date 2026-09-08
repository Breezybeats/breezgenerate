# Install NEUROJ on your M1 Mac

Requires macOS 11 or later and FL Studio running natively on Apple Silicon. This build is arm64, not Intel/Rosetta. Start with your monitor/headphone volume low.

1. Quit FL Studio.
2. In Finder press **Command + Shift + G** and paste `~/Library/Audio/Plug-Ins/`.
3. Open the `VST3` folder (create it if missing).
4. Copy the **NEUROJ.vst3 bundle** there. Do not open it or copy only its Contents folder.
5. Start FL Studio. Open **Options → Manage plugins**, then **Find installed plugins**. Enable rescanning previously failed plugins if an earlier NEUROJ scan failed.
6. Add NEUROJ as an **instrument/generator** in the Channel Rack, not as a mixer effect. Play a low MIDI note or click **AUDITION C**. Toggle audition off before sequencing.

Expected user-level path:

`~/Library/Audio/Plug-Ins/VST3/NEUROJ.vst3`

If FL Studio does not search this folder, add that exact directory to its plugin search paths and scan again.

## Mac security

This personal development build is ad-hoc signed, not Apple Developer ID signed or notarised. macOS may block it after download. If the system offers approval, inspect **System Settings → Privacy & Security** after the first load attempt and approve only the NEUROJ build you intentionally created. Exact wording varies by macOS release. Then reopen FL Studio and rescan.

Do not disable Gatekeeper globally. If macOS reports a damaged signature, an unknown architecture, or provides no approval option, send the exact error back rather than changing system security settings. A successful CI signature check is not notarisation and does not guarantee acceptance on your Mac.

## First session

- Select **JAPANESE NEURO**, hold a MIDI note, and click **GENERATE**.
- Use **MUTATE** for smaller changes. **UNDO** restores previous generated states and seeds.
- **COLOR** adds chord-tuned feedback resonators. **SPECTRAL** enables real FFT manipulation.
- **ADVANCED** exposes every implemented synthesis parameter and eight section locks.
- **Save** writes a `.neuroj` preset. Your FL project also stores parameter values and the actual generated wavetable samples.
- The factory selector contains 90 reproducible original generated recipes, not 90 hand-auditioned commercial presets.
- There is always a 512-sample reported latency so toggling spectral processing does not change host latency.

MIDI note 36 is the internal audition note. DAWs disagree on octave labels; use your ears and Piano Roll notes rather than assuming every application's C1 is the same frequency.

## Mandatory FL Studio checklist

- Scan and load as instrument without errors.
- Piano Roll note-on/note-off, pitch bend, sustain and mod wheel work.
- Generate while holding a note; compare Neuro, Gun, Color, Spectral and Reese.
- Disable COLOR and SPECTRAL after enabling them; the processing must turn off.
- Save project, close it, reopen; parameters, tables, seed and sound return.
- Automate a macro and verify playback and offline export.
- Resize the editor and open/scroll Advanced.

Official reference: [FL Studio installing plugins](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm).
