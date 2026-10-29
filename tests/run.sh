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
c++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -IMeteoPlaneRadar \
  MeteoPlaneRadar/CalendarIcs.cpp tests/calendar_ics.cpp -o "$test_build_dir/calendar_ics"
"$test_build_dir/calendar_ics"
c++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Itests/stubs -IMeteoPlaneRadar -I"$ARDUINOJSON_SRC" \
  MeteoPlaneRadar/Lang.cpp MeteoPlaneRadar/Settings.cpp MeteoPlaneRadar/CalendarIcs.cpp \
  MeteoPlaneRadar/Calendar.cpp MeteoPlaneRadar/ScreenCalendar.cpp MeteoPlaneRadar/ScreenChrome.cpp MeteoPlaneRadar/Layout.cpp tests/calendar_runtime.cpp -o "$test_build_dir/calendar_runtime"
"$test_build_dir/calendar_runtime"
node tests/web.mjs
c++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Itests/stubs -IMeteoPlaneRadar -I"$ARDUINOJSON_SRC" \
  MeteoPlaneRadar/Lang.cpp MeteoPlaneRadar/Settings.cpp MeteoPlaneRadar/CalendarIcs.cpp \
  MeteoPlaneRadar/Notifications.cpp tests/notifications.cpp -o "$test_build_dir/notifications"
"$test_build_dir/notifications"
python3 tests/chat_bridge.py
c++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Itests/stubs -IMeteoPlaneRadar \
  MeteoPlaneRadar/Navigation.cpp tests/navigation.cpp -o "$test_build_dir/navigation"
"$test_build_dir/navigation"
c++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -IMeteoPlaneRadar \
  MeteoPlaneRadar/CalendarIcs.cpp tests/calendar_zones.cpp -o "$test_build_dir/calendar_zones"
"$test_build_dir/calendar_zones"
c++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Itests/stubs -IMeteoPlaneRadar -I"$ARDUINOJSON_SRC" \
  MeteoPlaneRadar/Settings.cpp MeteoPlaneRadar/Lang.cpp MeteoPlaneRadar/Layout.cpp MeteoPlaneRadar/ScreenChrome.cpp \
  MeteoPlaneRadar/Navigation.cpp MeteoPlaneRadar/ScreenPlanes.cpp MeteoPlaneRadar/ScreenSettings.cpp \
  MeteoPlaneRadar/ScreenClock.cpp MeteoPlaneRadar/ScreenForecast.cpp MeteoPlaneRadar/WxIcon.cpp \
  tests/screen_layout.cpp -o "$test_build_dir/screen_layout"
"$test_build_dir/screen_layout"
