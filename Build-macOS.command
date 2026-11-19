#!/bin/bash
# Rave Academy - Instant Pump: build on your own Mac (free, no GitHub minutes).
# Double-click in Finder. First time macOS may ask to install Command Line Tools - accept and run again.
set -e
cd "$(dirname "$0")"
if ! xcode-select -p >/dev/null 2>&1; then xcode-select --install; echo "Zainstaluj Command Line Tools i uruchom ponownie."; exit 1; fi
if ! command -v cmake >/dev/null 2>&1; then
  if command -v brew >/dev/null 2>&1; then brew install cmake; else echo "Brak CMake. Zainstaluj: https://cmake.org/download/ (albo Homebrew: brew install cmake)"; exit 1; fi
fi
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --target InstantPump_VST3 InstantPump_AU -j "$(sysctl -n hw.ncpu)"
A="build/InstantPump_artefacts/Release"
codesign --force --deep -s - "$A/VST3/Rave Academy Instant Pump.vst3" "$A/AU/Rave Academy Instant Pump.component"
OUT="dist/Instant Pump (Mac)"; rm -rf dist; mkdir -p "$OUT"
cp -R "$A/VST3/Rave Academy Instant Pump.vst3" "$A/AU/Rave Academy Instant Pump.component" Install/INSTALUJ-Mac.txt LICENSE "$OUT/"
git archive --format=zip -o "$OUT/Instant Pump - kod zrodlowy.zip" HEAD 2>/dev/null || zip -qr "$OUT/Instant Pump - kod zrodlowy.zip" Source Tests CMakeLists.txt LICENSE README.md
(cd dist && ditto -c -k --keepParent "Instant Pump (Mac)" InstantPump-Mac.zip)
# install for yourself too
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
rm -rf ~/Library/Audio/Plug-Ins/VST3/"Rave Academy Instant Pump.vst3" ~/Library/Audio/Plug-Ins/Components/"Rave Academy Instant Pump.component"
cp -R "$A/VST3/Rave Academy Instant Pump.vst3" ~/Library/Audio/Plug-Ins/VST3/
cp -R "$A/AU/Rave Academy Instant Pump.component" ~/Library/Audio/Plug-Ins/Components/
echo ""
echo "GOTOWE. Wtyczka zainstalowana u Ciebie, a paczka do rozdania jest w: $(pwd)/dist/InstantPump-Mac.zip"
open dist
