#!/bin/bash
# Builds an unsigned macOS installer that puts VST3 + AU into the system plugin folders.
set -euo pipefail
ART=build/VISE8_artefacts/Release
OUT=dist; ROOT=$(mktemp -d)
mkdir -p "$OUT" "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Library/Audio/Plug-Ins/Components"
cp -R "$ART/VST3/VISE-8.vst3" "$ROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/VISE-8.component" "$ROOT/Library/Audio/Plug-Ins/Components/"
codesign --force --deep -s - "$ROOT/Library/Audio/Plug-Ins/VST3/VISE-8.vst3"
codesign --force --deep -s - "$ROOT/Library/Audio/Plug-Ins/Components/VISE-8.component"
pkgbuild --root "$ROOT" --identifier com.anvildynamics.vise8 --version 1.0.0 --install-location / "$OUT/VISE-8-component.pkg"
productbuild --package "$OUT/VISE-8-component.pkg" "$OUT/VISE-8-macOS-Installer.pkg"
rm "$OUT/VISE-8-component.pkg"
