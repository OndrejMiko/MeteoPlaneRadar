#!/bin/bash
# Produces a standalone universal .app; no Python or third-party dependencies.
set -euo pipefail
project_dir=$(cd "$(dirname "$0")" && pwd)
repo_dir=$(cd "$project_dir/../.." && pwd)
version=$(sed -n 's/^#define FW_VERSION "\([^"]*\)"/\1/p' "$repo_dir/MeteoPlaneRadar/Version.h")
[[ -n "$version" ]] || { echo 'Missing firmware version' >&2; exit 1; }
output_dir=${1:-"$project_dir/dist"}
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd)
oauth_config=${METEO_CHAT_OAUTH_CONFIG:-"$project_dir/config/google-oauth.json"}
if [[ "$oauth_config" == none ]]; then oauth_config=""; fi
if [[ -n "$oauth_config" && -n "${METEO_CHAT_OAUTH_CONFIG:-}" && ! -f "$oauth_config" ]]; then
  echo 'METEO_CHAT_OAUTH_CONFIG does not point to a file.' >&2; exit 1
fi
if [[ "${METEO_CHAT_REQUIRE_OAUTH:-0}" == 1 && ! -f "$oauth_config" ]]; then
  echo 'Missing Google OAuth registration. Run configure-oauth.sh with the downloaded Desktop app JSON first.' >&2; exit 1
fi
if [[ -f "$oauth_config" ]]; then
  swift run --build-system native --package-path "$project_dir" OAuthConfigTool validate "$oauth_config"
else
  echo 'Building without default Google sign-in; manual OAuth import remains available.'
fi
app_dir="$output_dir/Meteo Chat.app"
mkdir -p "$app_dir/Contents/MacOS" "$app_dir/Contents/Resources"
for architecture in arm64 x86_64; do
  swift build --build-system native --package-path "$project_dir" -c release --triple "$architecture-apple-macosx13.0" --product MeteoChat
  swift build --build-system native --package-path "$project_dir" -c release --triple "$architecture-apple-macosx13.0" --product ClaudeUsageRelay
 done
/usr/bin/lipo -create "$project_dir/.build/arm64-apple-macosx/release/MeteoChat" "$project_dir/.build/x86_64-apple-macosx/release/MeteoChat" -output "$app_dir/Contents/MacOS/MeteoChat"
/usr/bin/lipo -create "$project_dir/.build/arm64-apple-macosx/release/ClaudeUsageRelay" "$project_dir/.build/x86_64-apple-macosx/release/ClaudeUsageRelay" -output "$app_dir/Contents/MacOS/ClaudeUsageRelay"
/usr/bin/codesign --force --sign - "$app_dir/Contents/MacOS/ClaudeUsageRelay"
cat > "$app_dir/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>MeteoChat</string>
<key>CFBundleIdentifier</key><string>sk.meteoplaneradar.chat</string>
<key>CFBundleName</key><string>Meteo Chat</string>
<key>CFBundleDisplayName</key><string>Meteo Chat</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>$version</string>
<key>CFBundleVersion</key><string>$version</string>
<key>LSMinimumSystemVersion</key><string>13.0</string>
<key>LSUIElement</key><true/>
<key>NSHighResolutionCapable</key><true/>
<key>NSLocalNetworkUsageDescription</key><string>Meteo Chat posiela notifikácie na váš displej v domácej sieti.</string>
<key>NSAppTransportSecurity</key><dict><key>NSAllowsLocalNetworking</key><true/><key>NSAllowsArbitraryLoads</key><true/></dict>
</dict></plist>
PLIST
# Delete only our generated resource on rebuild so a no-config build never
# accidentally retains a previous project's OAuth identity.
rm -f "$app_dir/Contents/Resources/GoogleOAuth.json"
if [[ -f "$oauth_config" ]]; then
  swift run --build-system native --package-path "$project_dir" OAuthConfigTool export "$oauth_config" "$app_dir/Contents/Resources/GoogleOAuth.json"
fi
# Google URLs are fixed HTTPS in source; HTTP is needed for the LAN pairing URL.
/usr/bin/codesign --force --sign - --identifier sk.meteoplaneradar.chat "$app_dir"
/usr/bin/codesign --verify --strict "$app_dir"
/usr/bin/ditto -c -k --sequesterRsrc --keepParent "$app_dir" "$output_dir/Meteo-Chat-$version-macOS.zip"
echo "Built: $app_dir"
echo "Archive: $output_dir/Meteo-Chat-$version-macOS.zip"
