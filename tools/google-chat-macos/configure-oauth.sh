#!/bin/bash
# One-time author setup. INPUT is Google's downloaded Desktop app JSON.
set -euo pipefail
project_dir=$(cd "$(dirname "$0")" && pwd)
if [[ $# -ne 1 ]]; then
  echo 'Usage: tools/google-chat-macos/configure-oauth.sh "/absolute/path/to/client_secret.json"' >&2
  exit 1
fi
swift run --build-system native --package-path "$project_dir" OAuthConfigTool export "$1" "$project_dir/config/google-oauth.json"
echo 'Saved to ignored config/google-oauth.json. User tokens were not imported.'
echo 'Build: METEO_CHAT_REQUIRE_OAUTH=1 tools/google-chat-macos/build.sh'
