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
