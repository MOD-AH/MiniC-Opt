#!/usr/bin/env bash
# Cppify — build the compiler, build the app, wrap both into Cppify.app.
#
# SwiftPM produces a bare executable; macOS needs a bundle with an
# Info.plist before it will treat it as a real app (dock icon, menu bar,
# window focus). This assembles that bundle and puts the compiler binary
# inside it, so the app is self-contained and can be moved anywhere.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

CONFIG="${1:-release}"
APP="$ROOT/Cppify.app"

echo "==> Building minic-trace (the vendored, instrumented compiler)"
cd "$ROOT/MiniCCore"
mkdir -p build
c++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
    $(find src -name '*.cpp' | sort) -o build/minic-trace
echo "    build/minic-trace ready"

echo "==> Staging the compiler as an app resource"
cp "$ROOT/MiniCCore/build/minic-trace" "$ROOT/Cppify/Resources/minic-trace"

echo "==> Building the Swift app ($CONFIG)"
cd "$ROOT/Cppify"
swift build -c "$CONFIG"
BIN="$(swift build -c "$CONFIG" --show-bin-path)"

echo "==> Assembling Cppify.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$BIN/Cppify" "$APP/Contents/MacOS/Cppify"
# SwiftPM puts resources in a .bundle next to the binary; carry it across so
# Bundle.module keeps resolving once the executable has moved.
if [ -d "$BIN/Cppify_Cppify.bundle" ]; then
  cp -R "$BIN/Cppify_Cppify.bundle" "$APP/Contents/Resources/"
fi
# Dock / Finder icon: the compiled Icon Composer document (Assets.car, read via
# CFBundleIconName) plus actool's .icns fallback. Regenerate with
# branding/make-assets.swift then branding/package-assets.sh.
cp "$ROOT/Cppify/Resources/AppIcon/Assets.car"   "$APP/Contents/Resources/Assets.car"
cp "$ROOT/Cppify/Resources/AppIcon/AppIcon.icns" "$APP/Contents/Resources/AppIcon.icns"

cat > "$APP/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>              <string>Cppify</string>
    <key>CFBundleDisplayName</key>       <string>Cppify</string>
    <key>CFBundleIdentifier</key>        <string>local.minicopt.cppify</string>
    <key>CFBundleVersion</key>           <string>1.0</string>
    <key>CFBundleShortVersionString</key><string>1.0</string>
    <key>CFBundlePackageType</key>       <string>APPL</string>
    <key>CFBundleExecutable</key>        <string>Cppify</string>
    <key>CFBundleIconFile</key>          <string>AppIcon</string>
    <key>CFBundleIconName</key>          <string>AppIcon</string>
    <key>LSMinimumSystemVersion</key>    <string>26.0</string>
    <key>NSHighResolutionCapable</key>   <true/>
    <!-- Not sandboxed: the app spawns a bundled compiler binary, and this is
         a local development tool rather than something bound for the App
         Store. Sandboxing would add entitlement work for no benefit here. -->
    <key>NSSupportsAutomaticTermination</key> <false/>
</dict>
</plist>
PLIST

# Ad-hoc signature so Gatekeeper lets a locally built app run without the
# "damaged" dialog that unsigned bundles trigger.
codesign --force --deep --sign - "$APP" 2>/dev/null || \
  echo "    (codesign unavailable — the app still runs locally)"

echo "==> Done: $APP"
