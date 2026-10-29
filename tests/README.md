Run the host regression checks with a C++17 compiler, Node.js and ArduinoJson 7.x:

```sh
ARDUINOJSON_SRC=/absolute/path/to/ArduinoJson/src tests/run.sh
```

The C++ tests use in-memory Preferences to verify language persistence, legacy
screen masks, importing older backups and the current export format. They also
check that every display translation is ASCII. The web tests execute the embedded
JavaScript with a minimal DOM and API fixture to verify config loading, all three
languages, screen controls and the save payload.

Build the complete ESP32 firmware separately from the repository root:

```sh
arduino-cli compile --profile default MeteoPlaneRadar
```

The profile is in `MeteoPlaneRadar/sketch.yaml`. Host tests do not verify display
rendering, touch input, network services or flash persistence on physical hardware.

Calendar coverage includes folded/escaped ICS text, all-day and multi-day
boundaries, UTC and Central European DST, daily/weekly/monthly/yearly recurrence,
exceptions and moved/cancelled instances, unsupported rules/zones and capacity
limits. AddressSanitizer and UndefinedBehaviorSanitizer run on the parser and
scheduler tests. The scheduler tests verify PSRAM-only allocation, offline/clock
startup, failed downloads retaining the cache, source removal, and single-source
versus multiple-source drawing. `CALENDAR_SCREENSHOTS=/absolute/directory` also
records the real screen's drawing calls as SVG for layout inspection.

Navigation tests cover transient screen/zoom indicators, exact expiry, refreshing
the zoom timer, redraw requests surviving throttling, right-side geometry and
32-bit millis rollover. Physical gesture recognition still needs device testing.

Timezone tests cover namespaced aliases, VTIMEZONE order, yearly transitions,
DST gaps/folds, southern hemisphere, fractional offsets, bounded definitions,
source isolation and reporting an unsupported TZID while preserving cached data.

Screen layout coverage exercises actual aircraft/detail, forecast, clock and
settings draw calls with a busy map and long SSID, plus calendar day/month views.
It checks shared header/range reservations, whole-icon exclusion and hidden touch
targets, circular chord fit, and opaque backing before text. Notification tests
also verify that badges leave Today and settings controls usable.
`SCREEN_LAYOUT_SCREENSHOTS=/absolute/directory` writes SVG snapshots; weather
coverage renders only the shared chrome, without fetching or decoding radar PNGs.
These snapshots use approximate host fonts, not physical panel pixels.
