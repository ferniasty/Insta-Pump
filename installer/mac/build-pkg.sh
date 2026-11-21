#!/bin/bash
# Builds a double-click macOS installer (.pkg) for a Rave Academy plugin.
# usage: build-pkg.sh "<Plugin Name>" <id> <version> <path/to/X.vst3> <path/to/X.component|-> <out.pkg>
# Installs into /Library/Audio/Plug-Ins (VST3 + optional AU). Files installed by Installer.app are not
# quarantined, so no Terminal command is needed afterwards.
set -euo pipefail
NAME="$1"; ID="$2"; VER="$3"; VST3="$4"; AU="$5"; OUT="$6"
HERE="$(cd "$(dirname "$0")" && pwd)"
W="$(mktemp -d)"
mkpart () {   # mkpart <part> <bundle> <install dir>
  local part="$1" src="$2" dest="$3"
  mkdir -p "$W/root-$part$dest"
  ditto "$src" "$W/root-$part$dest/$(basename "$src")"
  chmod -R 755 "$W/root-$part"
  pkgbuild --analyze --root "$W/root-$part" "$W/$part.plist"
  # never "relocate" to a copy the user moved elsewhere: always install into the plug-in folder
  /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$W/$part.plist"
  pkgbuild --root "$W/root-$part" --component-plist "$W/$part.plist" --identifier "com.raveacademy.$ID.$part" \
           --version "$VER" --install-location / "$W/$part.pkg"
}
mkpart vst3 "$VST3" "/Library/Audio/Plug-Ins/VST3"
HAS_AU=0
if [ "$AU" != "-" ] && [ -d "$AU" ]; then mkpart au "$AU" "/Library/Audio/Plug-Ins/Components"; HAS_AU=1; fi

mkdir -p "$W/res"
sed "s/__NAME__/$NAME/g; s/__VERSION__/$VER/g" "$HERE/welcome.html" > "$W/res/welcome.html"
sed "s/__NAME__/$NAME/g" "$HERE/conclusion.html" > "$W/res/conclusion.html"

AU_LINE=""; AU_CHOICE=""; AU_REF=""
if [ $HAS_AU = 1 ]; then
  AU_LINE="<line choice=\"au\"/>"
  AU_CHOICE="<choice id=\"au\" title=\"Audio Unit (Logic, GarageBand)\" description=\"Optional. Not needed for Ableton.\"><pkg-ref id=\"com.raveacademy.$ID.au\"/></choice>"
  AU_REF="<pkg-ref id=\"com.raveacademy.$ID.au\" version=\"$VER\">au.pkg</pkg-ref>"
fi
cat > "$W/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>$NAME</title>
  <welcome file="welcome.html" mime-type="text/html"/>
  <conclusion file="conclusion.html" mime-type="text/html"/>
  <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <choices-outline>
    <line choice="vst3"/>
    $AU_LINE
  </choices-outline>
  <choice id="vst3" title="VST3 (Ableton Live)" description="The plug-in for Ableton Live and other VST3 hosts."><pkg-ref id="com.raveacademy.$ID.vst3"/></choice>
  $AU_CHOICE
  <pkg-ref id="com.raveacademy.$ID.vst3" version="$VER">vst3.pkg</pkg-ref>
  $AU_REF
</installer-gui-script>
XML
productbuild --distribution "$W/distribution.xml" --resources "$W/res" --package-path "$W" "$OUT"
pkgutil --check-signature "$OUT" >/dev/null 2>&1 || true
echo "Built $OUT"
