#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
: "${ARDUINOJSON_SRC:?Set ARDUINOJSON_SRC to ArduinoJson 7.x src directory}"
test_build_dir=$(mktemp -d)
trap 'rm -rf "$test_build_dir"' EXIT
c++ -std=c++17 -Wall -Wextra -Itests/stubs -IMeteoPlaneRadar -I"$ARDUINOJSON_SRC" \
  MeteoPlaneRadar/Lang.cpp MeteoPlaneRadar/Settings.cpp tests/settings_language.cpp \
  -o "$test_build_dir/settings_language"
"$test_build_dir/settings_language"
node tests/web.mjs
