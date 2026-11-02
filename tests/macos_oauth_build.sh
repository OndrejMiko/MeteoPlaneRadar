#!/bin/bash
# Offline packaging checks; uses synthetic registration only, never Keychain/API.
set -euo pipefail
repo_dir=$(cd "$(dirname "$0")/.." && pwd)
project_dir="$repo_dir/tools/google-chat-macos"
fixture_dir=$(mktemp -d)
trap 'rm -rf "$fixture_dir"' EXIT
cat > "$fixture_dir/client.json" <<'JSON'
{"installed":{"client_id":"123456789012-packagingtest.apps.googleusercontent.com","client_secret":"test-client-secret","access_token":"SHOULD_NOT_SHIP","project_id":"test-only"},"tokens":"SHOULD_NOT_SHIP"}
JSON
if METEO_CHAT_REQUIRE_OAUTH=1 METEO_CHAT_OAUTH_CONFIG=none "$project_dir/build.sh" "$fixture_dir/missing" > "$fixture_dir/missing.log" 2>&1; then
  echo 'Strict build unexpectedly accepted missing registration' >&2; exit 1
fi
if METEO_CHAT_OAUTH_CONFIG="$project_dir/config/google-oauth.example.json" "$project_dir/build.sh" "$fixture_dir/invalid" > "$fixture_dir/invalid.log" 2>&1; then
  echo 'Build unexpectedly accepted placeholder registration' >&2; exit 1
fi
METEO_CHAT_REQUIRE_OAUTH=1 METEO_CHAT_OAUTH_CONFIG="$fixture_dir/client.json" "$project_dir/build.sh" "$fixture_dir/output"
resource="$fixture_dir/output/Meteo Chat.app/Contents/Resources/GoogleOAuth.json"
[[ -f "$resource" ]] || { echo 'Bundled registration missing' >&2; exit 1; }
if rg -q 'SHOULD_NOT_SHIP|project_id|access_token' "$resource"; then
  echo 'Unneeded fields leaked into bundled registration' >&2; exit 1
fi
# Reusing the same output directory without registration must remove old identity.
METEO_CHAT_OAUTH_CONFIG=none "$project_dir/build.sh" "$fixture_dir/output"
[[ ! -f "$resource" ]] || { echo 'Stale registration remained in bundle' >&2; exit 1; }
echo 'OAuth packaging checks passed: strict missing/placeholder rejection, normalized embedding and stale resource removal'
