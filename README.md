# VISE-8

A behavioural model of a digitally-controlled analogue knee compressor with switchable
harmonic distortion (the Distressor family of behaviour), as a VST3. Original name, panel and
code: built from published specifications and listening-style behaviour, not from any
vendor's code, circuit data or artwork.

## Controls
INPUT / ATTACK / RELEASE / OUTPUT (0-10) · RATIO 1:1, 2:1, 3:1, 4:1, 6:1, 10:1 OPTO, 20:1, NUKE ·
AUDIO CLEAN / DIST 2 (2nd harmonic) / DIST 3 (3rd harmonic), HP · DETECTOR: DET HP, LINK, BRIT ·
IN (bypass) · MIX (parallel). Double-click a knob to reset. Window is resizable (host-dependent).

## Build
    git clone --depth 1 --branch 8.0.8 https://github.com/juce-framework/JUCE.git JUCE
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target VISE8_VST3 --config Release
Output: build/VISE8_artefacts/Release/VST3/VISE-8.vst3 (and AU/VISE-8.component on macOS, built with target VISE8_AU). The GitHub Actions workflow builds Windows/macOS/Linux.
Offline checks: `cmake --build build --target VISE8Test` then run `VISE8Test <outdir>` (needs a display or xvfb for UI snapshots).
