#!/usr/bin/env bash
# Packs the generated branding into the files the app ships:
#   Cppify/Resources/AppIcon/Assets.car  -- the macOS 26 icon (Icon Composer doc compiled by actool):
#                                           system mask + glass, Default/Dark/Clear/Tinted styles
#   Cppify/Resources/AppIcon/AppIcon.icns -- actool's fallback for anything reading CFBundleIconFile
#   Cppify/Resources/wordmark.tiff       -- title-bar logo, 1x and 2x in one file; macOS picks per display
set -euo pipefail
cd "$(dirname "$0")"
OUT=../Cppify/Resources/AppIcon
rm -rf "$OUT"; mkdir -p "$OUT"
xcrun actool AppIcon.icon --compile "$OUT" --platform macosx --minimum-deployment-target 26.0 \
  --app-icon AppIcon --output-partial-info-plist "$OUT/partial-Info.plist" \
  --warnings --errors --notices --output-format human-readable-text
rm -f "$OUT/partial-Info.plist"
tiffutil -cathidpicheck out/wordmark.png out/wordmark@2x.png -out ../Cppify/Resources/wordmark.tiff
rm -f ../Cppify/Resources/AppIcon.icns          # superseded by the compiled icon above
echo "packed AppIcon/ and wordmark.tiff"
