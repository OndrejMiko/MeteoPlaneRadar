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

Optional browser layout and navigation checks (requires Playwright and Chrome):

```sh
NODE_PATH=/absolute/path/to/node_modules node tests/web_layout.cjs
```

Checks all eight tabs in three languages at 320/390/768/1280 px, page overflow,
fixed save placement after scrolling, keyboard tab navigation, preservation of
unsaved fields and calendar activation payloads. All requests use local fixtures;
no device or external service is contacted. `WEB_SCREENSHOTS=/absolute/directory`
records desktop/mobile snapshots with synthetic device data.

Aircraft detail tests cover the shared 32px close icon and invisible 64px hit
area, capture of body/outside taps, exact target edges, and route/type rendering
in all languages and both unit systems.

The native Mac bridge has separate offline Swift checks. With Apple Command Line
Tools, run `swift run --build-system native --package-path tools/google-chat-macos
ChatCoreChecks` as a single command. They cover actual local OAuth callbacks,
cancellation, redirect refusal and the native delivery/checkpoint implementation.
No Google account, Keychain item or real display is used by these checks.

Code pairing checks cover single use, exact expiry, five guesses, cancellation,
replacement and millis rollover. Web checks cover the admin-password payload for code generation,
local expiry and cancellation; native checks cover code claim formatting,
address validation, unsupported firmware and invalid response keys.

`tests/macos_oauth_build.sh` validates native OAuth packaging offline: strict
missing/placeholder refusal, normalized registration with no token fields, and
removal of a stale bundled client when reusing an output directory. It uses only
synthetic registration in a temporary folder; no real credentials or Keychain.

Map overlays now render the real precipitation legend and bottom playback track
with its first/last-frame thumb in the shared chrome fixture. Tests verify that
the old wide top panel and playback dots are absent, the map beneath the clock
is available for labels, and the notification badge avoids the new controls.
Browser checks also enforce a separate, spaced test-notification action row.

Energy coverage checks Czech-only screen visibility across language changes, backup and NVS migration, preventing removal of the last visible screen, zero HTTP requests while hidden or disabled, regional limits, quarter-hour price parsing, tariff calculation and generation type grouping. Real-browser checks verify that energy controls and remote buttons are hidden in English/Slovak while their saved selections are preserved.

Regenerate the repository's screen gallery using the real drawing fixtures and
synthetic data (requires the same ArduinoJson setup plus Playwright and Chrome):

```sh
preview_dir=$(mktemp -d)
SCREEN_LAYOUT_SCREENSHOTS="$preview_dir" CALENDAR_SCREENSHOTS="$preview_dir" \
NOTIFICATION_SCREENSHOTS="$preview_dir" tests/run.sh
NODE_PATH=/absolute/path/to/node_modules node tools/render_screenshots.cjs "$preview_dir"
```

This updates `obrazovky.png` and `docs/screens/`. Calendar and notification
fixtures contain synthetic data. The weather image shows real controls over a
synthetic grid rather than live radar; the banner preview composes the real
notification layer over the clock fixture; expanded details replace the screen. Smooth fonts use actual firmware
bitmaps; the host approximates the built-in pixel font. These are previews,
not physical-device photographs or proof of real-account/network behaviour.

Windows bridge core and cross-build checks (requires .NET 10 SDK):

```sh
dotnet run --project tools/google-chat-windows/Checks/Checks.csproj -c Release
dotnet build tools/google-chat-windows/MeteoChat/MeteoChat.csproj -c Release
```

Windows-only DPAPI/persistence/UI checks are available as `MeteoChat.exe --self-test`;
the Windows packaging script runs them. Core checks use synthetic messages and local
loopback HTTP servers only, never real Google accounts or displays. See the
[Windows guide](../tools/google-chat-windows/README.md) for packaging and limitations.
