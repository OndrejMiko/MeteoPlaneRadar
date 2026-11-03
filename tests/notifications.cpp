#include "Notifications.h"

#include <esp_heap_caps.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "Lang.h"
#include "Settings.h"
#include "UI.h"

Arduino_GFX display;
Arduino_GFX* gfx = &display;
void UI_TextCenteredIn(const char* s, int x, int w, int y, uint16_t c, uint8_t size) {
  gfx->setTextSize(size);
  gfx->setTextColor(c);
  gfx->setCursor(x + (w - (int)strlen(s) * 6 * size) / 2, y);
  gfx->print(s);
}
void UI_TextCentered(const char* s, int y, uint16_t c, uint8_t size) {
  UI_TextCenteredIn(s, 0, 480, y, c, size);
}
void settings(bool enabled, unsigned seconds = 15) {
  JsonDocument doc;
  doc["notifications"]["enabled"] = enabled;
  doc["notifications"]["seconds"] = seconds;
  assert(Settings_FromJson(doc.as<JsonObjectConst>()));
}
JsonDocument message(unsigned id) {
  JsonDocument doc;
  doc["service"] = "google-chat";
  doc["id"] = std::to_string(id);
  doc["sender"] = "Matúš";
  doc["room"] = "Projekt";
  doc["text"] = "Dôležitá správa. C:\\new\nDruhy riadok.";
  return doc;
}
int main() {
  Settings_ClearAll();
  Settings_SetLanguage(LANG_SK);
  auto doc = message(0);
  assert(!Notifications_Push(doc.as<JsonObjectConst>()));
  settings(true);
  testFailPsram = true;
  assert(!Notifications_Push(doc.as<JsonObjectConst>()));
  assert(!Notifications_Count());
  testFailPsram = false;
  assert(Notifications_Push(doc.as<JsonObjectConst>()));
  assert(Notifications_Count() == 1 && Notifications_Visible() && Notifications_Tick());
  assert(Notifications_Push(doc.as<JsonObjectConst>()) && Notifications_Count() == 1);
  display.fillScreen(0);
  Notifications_Draw();
  assert(!Notifications_Tick());
  assert(display.drawing.str().find("x='43' y='115' width='394' height='197'") !=
         std::string::npos);
  assert(display.drawing.str().find("x='368' y='120' width='64'") ==
         std::string::npos);                    // invisible hit area
  assert(Notifications_Touch(10, 10, false));   // swipe never falls through an open banner
  assert(Notifications_Touch(100, 210, true));  // expanded
  testMillis += 150000;
  assert(Notifications_Visible());
  Notifications_Tick();
  assert(Notifications_Visible());
  display.fillScreen(0);
  Notifications_Draw();
  std::string all;
  for (const auto& s : display.texts) all += s;
  assert(all.find("Google Chat")==std::string::npos && all.find("Notifikacie")!=std::string::npos);
  assert(all.find("Matus") != std::string::npos && all.find("C:\\new") != std::string::npos);
  assert(Notifications_Touch(359, 95, true));
  assert(!Notifications_Count());  // edge of expanded X target
  assert(Notifications_Push(doc.as<JsonObjectConst>()) &&
         !Notifications_Count());  // dismissed ID stays deduped
  for (unsigned i = 1; i <= 6; ++i) {
    auto n = message(i);
    assert(Notifications_Push(n.as<JsonObjectConst>()));
  }
  auto extra = message(7);
  assert(!Notifications_Push(extra.as<JsonObjectConst>()));
  assert(Notifications_Dropped() == 1);
  testMillis += 15000;
  assert(Notifications_Tick());
  assert(!Notifications_Visible() && Notifications_Count() == 6);
  assert(!Notifications_Touch(100, 100, true));  // pages usable while badge only
  assert(Notifications_Touch(208, 391, true));
  assert(Notifications_Visible());
  assert(Notifications_Touch(431, 183, true));
  assert(Notifications_Count() == 5);  // full 64x64 invisible target
  assert(Notifications_Push(extra.as<JsonObjectConst>()));
  assert(Notifications_MemoryBytes() < 8192);
  const char* snapshots = getenv("NOTIFICATION_SCREENSHOTS");
  if (snapshots) {
    display.fillScreen(0);
    Notifications_Draw();
    display.snapshot((std::string(snapshots) + "/banner.svg").c_str());
    Notifications_Touch(100, 200, true);
    display.fillScreen(0);
    Notifications_Draw();
    display.snapshot((std::string(snapshots) + "/detail.svg").c_str());
  }
  std::cout << "Notification queue PSRAM: " << Notifications_MemoryBytes() << " B\n";
  // Return from the intentionally persistent detail to a timed banner.
  if (snapshots) assert(Notifications_Touch(359, 95, true));
  // An unread badge must leave calendar Today, forecast AQI and map range usable.
  testMillis += 15000; Notifications_Tick();
  Settings_SetScreen(SCREEN_CALENDAR_I);
  assert(!Notifications_Touch(240,420,true));
  assert(Notifications_Touch(366,134,true));
  testMillis += 15000; Notifications_Tick();
  Settings_SetScreen(SCREEN_SETTINGS_I);
  assert(!Notifications_Touch(240,390,true));
  assert(Notifications_Touch(240,450,true));
  testMillis += 15000; Notifications_Tick();
  Settings_SetScreen(SCREEN_METEO_I);
  assert(!Notifications_Touch(240,415,true)); // playback track remains usable
  assert(Notifications_Touch(240,330,true));
  auto generic=message(99);generic["service"]="notification";
  assert(!Notifications_Validate(generic.as<JsonObjectConst>()));
  settings(false);
  assert(Notifications_Tick());
  assert(!Notifications_Count() && !Notifications_Visible() && !Notifications_MemoryBytes());
  doc["service"] = "other";
  assert(Notifications_Validate(doc.as<JsonObjectConst>()));
  doc = message(8);
  doc["text"] = std::string(1025, 'x');
  assert(Notifications_Validate(doc.as<JsonObjectConst>()));
  JsonDocument config;
  config["notifications"]["key"] = std::string(64, 'a');
  assert(Settings_FromJson(config.as<JsonObjectConst>()));
  assert(Notifications_Authorized(std::string(64, 'a').c_str()));
  assert(!Notifications_Authorized(std::string(64, 'b').c_str()));
  assert(!Notifications_Authorized(nullptr));
  config["notifications"]["seconds"] = 4;
  assert(Settings_ValidateNotifications(config.as<JsonObjectConst>()));
  config["notifications"]["seconds"] = 0;
  assert(!Settings_ValidateNotifications(config.as<JsonObjectConst>()));
  config["notifications"]["key"] = "bad";
  assert(Settings_ValidateNotifications(config.as<JsonObjectConst>()));
  JsonDocument exported;
  Settings_ToJson(exported.to<JsonObject>());
  assert(exported["notifications"]["paired"].as<bool>());
  assert(exported["notifications"]["key"].isNull());
  Settings_Begin();
  assert(Notifications_Authorized(std::string(64, 'a').c_str()));
  Settings_ClearAll();
  assert(!Notifications_Authorized(std::string(64, 'a').c_str()));
  std::cout << "Notification input, queue, dedupe, touch capture, timeout, memory failure and "
               "secrets passed\n";
}
