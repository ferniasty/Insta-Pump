# Rave Academy – Instant Pump

Volume-ducking plugin in the style of a sidechain: the bass is turned down on every kick and
comes back along a smooth curve. VST3 (Mac + Windows) and AU (Mac), built with JUCE 8.

Controls: **MIX** (depth), **LENGTH** (stretch the envelope, also drag the display left/right),
**CURVE** (bend of the recovery), **SHAPE** (TIGHT / PUMP / SMOOTH / CUT).
Every edge of the envelope is a curve no steeper than a 12 ms S-fade, so no setting clicks.

## Build

Mac and Windows builds run automatically on GitHub Actions (`.github/workflows/build.yml`);
download them from the run's *Artifacts*.

Local build:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Tests (envelope sweep, stereo + click test on the real processor, UI snapshot):

```bash
cmake -B build -DKD_BUILD_TOOLS=ON && cmake --build build
./build/kd_envelope_test
./build/kd_snapshot_artefacts/Release/kd_snapshot preview.png
```

## Install (Mac)

Unzip and copy `Instant Pump.vst3` to `~/Library/Audio/Plug-Ins/VST3/` and
`Instant Pump.component` to `~/Library/Audio/Plug-Ins/Components/`.
The build is not notarized, so macOS may block it the first time. Remove the quarantine flag:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Instant Pump.vst3" ~/Library/Audio/Plug-Ins/Components/"Instant Pump.component"
```

## License note

JUCE is dual-licensed (AGPLv3 or a JUCE commercial licence, including the free Starter tier
for small revenue). Pick the right one before selling or distributing this plugin.
