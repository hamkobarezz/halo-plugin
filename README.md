# HALO by HALO Studios — vocal, mix & mastering chain

VST3 (FL Studio, Ableton, Cubase, Reaper, Studio One …) · AU (Logic Pro, build on a Mac) · Standalone

## Signal chain
`Input → [Enhance pre] → [EQ pre] → COMPRESS → ENHANCE → EQ → [Drive pre-FX] → MULTIPLY → SPACE (delay → reverb) → DRIVE → MASTER → Output → Dry/Wet`

| Module | What it does |
|---|---|
| **Compress** | One-knob compressor with 4 models: Clean (VCA), Opto (program-dependent release), FET (fast + bite), Vari-Mu (thick glue, 2nd harmonic). Input, Speed, Output, parallel Mix, sidechain filter, auto gain, live GR meter |
| **Enhance** | Relative split-band de-esser (only reacts when highs dominate) with frequency, 2-band de-noise expander, Body (low-mid weight), Air (shelf + harmonic exciter). Pre-comp option |
| **EQ** | 6 bands: 24 dB/oct low cut, low shelf, 2 bells, high shelf, 24 dB/oct high cut. Drag nodes on the graph, mouse wheel = Q, double-click = reset, live spectrum behind the curve. Pre-comp option |
| **Multiply** | 2–8 drifting, detuned, time-offset voices spread across the stereo field; low end of the doubles is cut so bass stays mono |
| **Space** | 8-channel FDN reverb (Room, Studio, Plate, Hall, Cathedral, Ambient) with pre-delay, damping, low cut, width, ducking · Delay (Mono, Ping-Pong, Wide, Tape with wow/flutter, Slap), tempo sync or ms, feedback tone, ducking |
| **Drive** | Tape, Tube, Broken Tube, Transistor, Diode, Fold — oversampled, auto level-matched, dry/wet, clip, pre-FX option |
| **Master** | Tilt EQ, stereo width, bass mono, glue bus compressor, loudness drive into an oversampled look-ahead brickwall limiter (ceiling, release), LUFS gauge |

Global: Input / Output / Dry-Wet, Bypass (click-free), **Quality** Eco 1x / High 2x / Ultra 4x / Insane 8x oversampling,
70 full-chain presets + 40 department presets (Vocals / Phone / Radio / Space / Singers / Artists / Mix / Master) + user presets (`Documents\HALO Studios\HALO\Presets`), A/B compare, undo/redo,
spectrum analyser (in vs out), level meters, gain reduction (comp/glue/limit), EBU R128 loudness (momentary, short-term, integrated, peak).
Resizable window (drag the corner), size is remembered.

Knobs: drag, **Shift-drag = fine**, **double-click = default**, hover shows the value.

## Measured (offline test, 48 kHz)
- All factory presets: no NaN/Inf, never exceed the master ceiling
- Limiter: +36 dB into a −1 dB ceiling → peak −1.000 dBFS
- Dry/Wet & bypass are latency aligned at every quality (null test 56–146 dB)
- CPU, one instance: Eco 4% · High 5.6% · Ultra 7.3% · Insane 9.8% of one core
- Latency: 72 / 170 / 192 / 202 samples (reported to the host, auto compensated)

## Download
**[Latest release](https://github.com/hamkobarezz/halo-plugin/releases/latest)** — one-click installers:

| Platform | Download | Then |
|---|---|---|
| **Windows** (FL Studio, Ableton, Cubase, Reaper…) | [HALO-1.1.0-Windows-Setup.exe](https://github.com/hamkobarezz/halo-plugin/releases/latest/download/HALO-1.1.0-Windows-Setup.exe) | Run it (asks for admin). In FL: *Options → Manage plugins → Find installed plugins*. Uninstall from *Settings → Apps*. |
| **macOS** (Logic Pro AU + VST3 + app) | [HALO-1.1.0-macOS.dmg](https://github.com/hamkobarezz/halo-plugin/releases/latest/download/HALO-1.1.0-macOS.dmg) | Open the DMG, double-click *Install HALO.pkg* (unsigned: right-click > Open). In Logic: *Audio Units → HALO Studios → HALO*. |
| **iPad** (Logic Pro for iPad, AUv3) | no installer file exists on iOS | `packaging/ios/build_ipad.sh` on a Mac, then Run from Xcode / TestFlight |

New releases are built automatically: push a tag like `v1.0.1` and GitHub builds both installers and publishes the release.

## Build
Windows (VS 2026 Build Tools):
```
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target HBHalo_VST3 HBHalo_Standalone
build\HaloTest_artefacts\Release\HaloTest.exe      # DSP test suite
```
macOS (Logic needs the **AU**, which can only be built on a Mac with Xcode):
```
cmake -S . -B build -G Xcode
cmake --build build --config Release --target HBHalo_AU HBHalo_VST3
cp -R "build/HBHalo_artefacts/Release/AU/HALO.component" ~/Library/Audio/Plug-Ins/Components/
```
Then in Logic: *Plug-in Manager → Reset & Rescan*. For distribution to other Macs, sign + notarize the component.

`-DHALO_DEMO=ON` builds a silent self-test version that feeds an internal test voice (UI checks only — never ship it).
