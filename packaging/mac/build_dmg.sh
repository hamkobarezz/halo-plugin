#!/bin/bash
# Builds HALO for macOS (Logic Pro AU + VST3 + standalone app, Apple Silicon + Intel)
# and packages it as dist/HALO-<version>-macOS.dmg containing a one-click installer.
#
# Run on a Mac with Xcode installed:   ./packaging/mac/build_dmg.sh
#
# Optional signing / notarisation (needs a paid Apple Developer account):
#   export HALO_APP_SIGN_ID="Developer ID Application: Your Name (TEAMID)"
#   export HALO_PKG_SIGN_ID="Developer ID Installer: Your Name (TEAMID)"
#   export HALO_NOTARY_PROFILE="halo-notary"   # created with: xcrun notarytool store-credentials
# Without them everything is ad-hoc signed: Logic loads it fine, but macOS will warn on first
# open of the installer (right-click > Open, or System Settings > Privacy & Security > Open Anyway).
set -euo pipefail
cd "$(dirname "$0")/../.."
ROOT="$(pwd)"
VERSION="1.1.0"
BUILD="$ROOT/build-mac"
ART="$BUILD/HBHalo_artefacts/Release"
WORK="$BUILD/package"
APP_ID="${HALO_APP_SIGN_ID:--}"

if [ ! -d "$ROOT/JUCE" ]; then
  git clone --depth 1 --branch 9.0.5 https://github.com/juce-framework/JUCE.git "$ROOT/JUCE"
fi

echo "==> Building (universal arm64 + x86_64)"
cmake -S "$ROOT" -B "$BUILD" -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "$BUILD" --config Release --target HBHalo_AU HBHalo_VST3 HBHalo_Standalone -- -quiet

echo "==> Running DSP tests"
cmake --build "$BUILD" --config Release --target HaloTest -- -quiet
"$BUILD/HaloTest_artefacts/Release/HaloTest"

echo "==> Signing ($APP_ID)"
SIGN_ARGS=(--force --deep --sign "$APP_ID")
if [ "$APP_ID" != "-" ]; then SIGN_ARGS+=(--options runtime --timestamp); fi
for b in "$ART/AU/HALO.component" "$ART/VST3/HALO.vst3" "$ART/Standalone/HALO.app"; do
  codesign "${SIGN_ARGS[@]}" "$b"
done

echo "==> Building installer package"
rm -rf "$WORK" && mkdir -p "$WORK/root/Library/Audio/Plug-Ins/Components" "$WORK/root/Library/Audio/Plug-Ins/VST3" "$WORK/root/Applications" "$WORK/scripts" "$WORK/dmg"
cp -R "$ART/AU/HALO.component"   "$WORK/root/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/VST3/HALO.vst3"      "$WORK/root/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/Standalone/HALO.app" "$WORK/root/Applications/"

cat > "$WORK/scripts/postinstall" <<'EOS'
#!/bin/bash
# refresh the Audio Unit cache so Logic sees HALO immediately
killall -9 AudioComponentRegistrar 2>/dev/null || true
exit 0
EOS
chmod +x "$WORK/scripts/postinstall"

# keep bundles at their install paths (don't let Installer "relocate" them)
pkgbuild --analyze --root "$WORK/root" "$WORK/components.plist"
plutil -replace BundleIsRelocatable -bool NO "$WORK/components.plist" 2>/dev/null || true
for i in $(seq 0 10); do plutil -replace "$i.BundleIsRelocatable" -bool NO "$WORK/components.plist" 2>/dev/null || true; done

pkgbuild --root "$WORK/root" --component-plist "$WORK/components.plist" --scripts "$WORK/scripts" \
  --identifier com.halostudios.halo.pkg --version "$VERSION" --install-location / "$WORK/HALO-core.pkg"

cat > "$WORK/distribution.xml" <<EOS
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>HALO</title>
  <organization>com.halostudios</organization>
  <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <welcome mime-type="text/plain"><![CDATA[HALO by HALO Studios — vocal, mix & mastering chain.

This installs:
  • Audio Unit for Logic Pro / GarageBand  (/Library/Audio/Plug-Ins/Components)
  • VST3 for Ableton, FL Studio, Cubase…    (/Library/Audio/Plug-Ins/VST3)
  • HALO standalone app                     (/Applications)

After installing, open Logic Pro. If HALO isn't listed: Logic Pro > Settings > Plug-in Manager > Reset & Rescan Selection.]]></welcome>
  <choices-outline><line choice="default"/></choices-outline>
  <choice id="default" title="HALO"><pkg-ref id="com.halostudios.halo.pkg"/></choice>
  <pkg-ref id="com.halostudios.halo.pkg" version="$VERSION">HALO-core.pkg</pkg-ref>
</installer-gui-script>
EOS

PKG="$WORK/dmg/Install HALO.pkg"
if [ -n "${HALO_PKG_SIGN_ID:-}" ]; then
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" --sign "$HALO_PKG_SIGN_ID" "$PKG"
else
  productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" "$PKG"
fi

cat > "$WORK/dmg/Read Me.txt" <<'EOS'
HALO by HALO Studios

1. Double-click "Install HALO.pkg" and follow the steps.
   (If macOS says it can't verify the developer: right-click the pkg > Open > Open.)
2. Open Logic Pro, add HALO from Audio Units > HALO Studios > HALO on any track or bus.
3. Not showing? Logic Pro > Settings > Plug-in Manager > Reset & Rescan Selection.

User presets are saved to Documents/HALO Studios/HALO/Presets.
EOS

echo "==> Creating DMG"
mkdir -p "$ROOT/dist"
DMG="$ROOT/dist/HALO-$VERSION-macOS.dmg"
rm -f "$DMG"
hdiutil create -volname "HALO $VERSION" -srcfolder "$WORK/dmg" -ov -format UDZO "$DMG"

if [ -n "${HALO_NOTARY_PROFILE:-}" ]; then
  echo "==> Notarising"
  [ "$APP_ID" != "-" ] && codesign --force --sign "$APP_ID" --timestamp "$DMG"
  xcrun notarytool submit "$DMG" --keychain-profile "$HALO_NOTARY_PROFILE" --wait
  xcrun stapler staple "$DMG"
fi

echo "Done: $DMG"
