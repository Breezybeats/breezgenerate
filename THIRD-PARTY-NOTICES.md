# Dependency notices

The build downloads JUCE **8.0.6** from the juce-framework/JUCE repository. Its modules are dual-licensed under AGPLv3 and the commercial JUCE licence; they are not MIT-licensed by this project's LICENSE. The bundled VST3 SDK and other dependencies retain the terms listed in the JUCE license file and their individual notices.

The workflow copies JUCE's license and its bundled dependency license/notice files into the downloadable artifact. The exact downloaded source version and repository source URL are included so source can be obtained alongside binaries. A source ZIP is also included in the cloud output.

Do not assume the MIT license on the project-specific code gives you permission to redistribute proprietary JUCE-based binaries without meeting JUCE's terms. Choose the applicable JUCE licensing route before distributing outside personal development. This project supplies no commercial JUCE or Apple Developer ID license.

Primary dependency sources:

- [JUCE 8.0.6 source](https://github.com/juce-framework/JUCE/tree/8.0.6)
- [JUCE 8.0.6 licence](https://github.com/juce-framework/JUCE/blob/8.0.6/LICENSE.md)
- [JUCE embedded VST3 SDK licence](https://github.com/juce-framework/JUCE/blob/8.0.6/modules/juce_audio_processors/format_types/VST3_SDK/LICENSE.txt)

Original wavetable samples are computed from deterministic mathematical harmonic coefficients. Noise and gun transients are synthesized. No LOBOTIX, Vital, Serum or other proprietary presets, samples, interfaces or wavetables are distributed.
