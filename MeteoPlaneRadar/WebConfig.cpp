// =============================================================================
//  MeteoPlaneRadar
//  The configuration web server. See WebConfig.h.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "WebConfig.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <math.h>
#include <string.h>

#include "Board.h"
#include "Calendar.h"
#include "Config.h"
#include "Display_ST7701.h"
#include "Forecast.h"
#include "Lang.h"
#include "Net.h"
#include "NightMode.h"
#include "Notifications.h"
#include "NotificationPairing.h"
#include "ScreenCalendar.h"
#include "Energy.h"
#include "ScreenPrice.h"
#include "ScreenPlanes.h"
#include "ScreenWeather.h"
#include "Settings.h"
#include "Status.h"
#include "TimeZone.h"
#include "UI.h"
#include "Version.h"
#include "Watchdog.h"
#include "WebPage.h"
#include "StormWatch.h"
#include "PlaneWatch.h"
#include "PlaneIcon.h"
#include "ADSB.h"
#include "Outside.h"
#include <time.h>

// Defined in the .ino - the screen actually on the display.
extern int  MPR_CurrentScreen();
extern bool MPR_AlertActive();

static WebServer  s_srv(WEB_PORT);
static DNSServer  s_dns;
static uint32_t s_bridgeContact = 0;
static bool s_bridgeSeen = false;
static NotificationPairing s_notificationPairing;
static bool s_apMode = false;
static bool s_running = false;
static bool s_wantConnect = false;
static bool s_wantRestart = false;
static volatile bool s_updating = false;

// Queued remote-control requests - see the note in WebConfig.h.
static int s_reqScreen     = -1;
static int s_reqScreenStep = 0;
static int s_reqRangeStep  = 0;

bool WebConfig_UpdateBusy()        { return s_updating; }
bool WebConfig_WantsWifiConnect()  { return s_wantConnect; }
void WebConfig_ClearWifiConnect()  { s_wantConnect = false; }
bool WebConfig_WantsRestart()      { return s_wantRestart; }

int WebConfig_TakeScreen()     { int v = s_reqScreen;     s_reqScreen = -1;    return v; }
int WebConfig_TakeScreenStep() { int v = s_reqScreenStep; s_reqScreenStep = 0; return v; }
int WebConfig_TakeRangeStep()  { int v = s_reqRangeStep;  s_reqRangeStep = 0;  return v; }

// --- Helpers ----------------------------------------------------------------
static void sendJson(int code, JsonDocument& doc) {
  String out;
  serializeJson(doc, out);
  s_srv.send(code, "application/json", out);
}

static bool readBody(JsonDocument& doc) {
  if (!s_srv.hasArg("plain")) return false;
  return deserializeJson(doc, s_srv.arg("plain")) == DeserializationError::Ok;
}

// The set of enabled data screens as one value, so "did this change?" is one
// comparison. Built from the loop rather than written out screen by screen -
// the hand-written version needed two edits whenever a screen was added.
static uint8_t screenMask() {
  uint8_t m = 0;
  for (uint8_t i = 0; i < SCREEN_SETTINGS_I && i < 8; i++)
    if (Settings_ScreenEnabled(i)) m |= (uint8_t)(1u << i);
  return m;
}

// Every destructive endpoint goes through here. When no password is set this
// waves everything through - that is the documented default, and the device
// only listens on the home LAN.
static bool authed(JsonDocument& body) {
  const char* pw = body["password"] | "";
  if (Settings_CheckAdminPassword(pw)) return true;
  s_srv.send(403, "application/json", "{\"error\":\"password\"}");
  return false;
}

// --- Endpoints --------------------------------------------------------------
static void handleRoot() {
  s_srv.sendHeader("Cache-Control", "no-store");
  // The charset belongs in the HEADER, not only in a meta tag: a browser that
  // reads the header first would otherwise guess, and the Czech accents on the
  // page would come out as mojibake.
  s_srv.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

static void handleGetConfig() {
  JsonDocument doc;
  JsonObject o = doc.to<JsonObject>();
  Settings_ToJson(o);
  // The remembered networks: names only. A password never leaves the device;
  // the page is told whether one is stored, so it can say so.
  {
    JsonArray w = o["wifi"].to<JsonArray>();
    for (uint8_t i = 0; i < WIFI_SLOTS; i++) {
      JsonObject e = w.add<JsonObject>();
      e["ssid"] = Settings_WifiSsid(i);
      e["hasPass"] = Settings_WifiPass(i)[0] != '\0';
    }
  }
  o["version"] = FW_VERSION;
  o["apMode"]  = s_apMode;
  o["ip"]      = s_apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  sendJson(200, doc);
}

static void handlePostConfig() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }

  if (const char* error = Settings_ValidateCalendarJson(doc.as<JsonObjectConst>())) {
    JsonDocument res;
    res["error"] = error;
    sendJson(400, res);
    return;
  }
  if (const char* error = Settings_ValidateNotifications(doc.as<JsonObjectConst>())) {
    JsonDocument res;
    res["error"] = error;
    sendJson(400, res);
    return;
  }
  if (!doc["notifications"].isNull() && !authed(doc)) return;
  if (!doc["notifications"].isNull()) s_notificationPairing.cancel();
  // Calendar URLs grant read access to private events. Editing them uses the
  // same optional admin password as import; URLs never round-trip through GET.
  if (!doc["calendars"].isNull() && !authed(doc)) return;

  // The password is write-only and never round-trips through the page, so it is
  // handled apart from the rest of the settings.
  //
  // Changing it requires the current one. Without that check anyone who can
  // reach the page could set a password and lock the owner out of the update
  // and reset endpoints - the settings themselves are deliberately open, but
  // that must not be a way to take the device over.
  JsonVariantConst np = doc["newPassword"];
  if (!np.isNull()) {
    const char* p = np.as<const char*>();
    if (p && *p) {
      if (!authed(doc)) return;                 // sends 403 and explains itself
      s_notificationPairing.cancel();
      Settings_SetAdminPassword(p);
      // The update page checks the password on every request, so a new one is
      // live immediately - no restart needed.
    }
  }

  const double oldLat = Settings_Lat(), oldLon = Settings_Lon();
  const uint8_t oldSrc = Settings_RadarSource();
  const uint8_t oldMask = screenMask();

  Settings_FromJson(doc.as<JsonObjectConst>());
  if (!doc["mixCountry"].isNull()) Energy_Invalidate();

  // Applied straight away - these are the ones you want to see change while
  // you are still looking at the slider.
  NightMode_Apply();

  const uint8_t newMask = screenMask();
  const bool moved = (fabs(oldLat - Settings_Lat()) > 1e-6) ||
                     (fabs(oldLon - Settings_Lon()) > 1e-6);
  if (moved) {
    Forecast_Invalidate();
    Energy_Invalidate();
  }

  // These reach too far into cached state (decoded radar frames, allocated
  // buffers, which screen is even reachable) to be worth unpicking at runtime.
  // A restart is a second and a half and guarantees a clean result.
  if (moved || oldSrc != Settings_RadarSource() || oldMask != newMask)
    s_wantRestart = true;

  JsonDocument res;
  res["ok"] = true;
  res["restart"] = s_wantRestart;
  sendJson(200, res);
}

// Jump to a screen, or step one along. Disabled screens are refused rather than
// silently ignored, so the page can say why nothing happened.
static void handleScreen() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }

  JsonVariantConst idx = doc["index"];
  if (!idx.isNull()) {
    int i = idx.as<int>();
    if (i < 0 || i >= SCREEN_N) {
      s_srv.send(400, "application/json", "{\"error\":\"range\"}");
      return;
    }
    if (!Settings_ScreenEnabled((uint8_t)i)) {
      s_srv.send(409, "application/json", "{\"error\":\"disabled\"}");
      return;
    }
    s_reqScreen = i;
  } else {
    int st = doc["step"] | 0;
    s_reqScreenStep = (st < 0) ? -1 : ((st > 0) ? +1 : 0);
  }
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

// Change the range on whichever radar screen is showing. The screens that have
// no range simply ignore it, exactly as a swipe does.
static void handleRange() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  int st = doc["step"] | 0;
  s_reqRangeStep = (st < 0) ? -1 : ((st > 0) ? +1 : 0);
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

static void handleStatus() {
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  // Which panel the firmware decided it is running on. One binary serves both
  // boards, so this is the first thing to ask about when a bug report says the
  // picture rolls or the touch is dead. A "?" means detection fell back.
  doc["board"] = Board_Detected() ? Board_Name() : "? -> " + String(Board_Name());
  doc["ip"]   = s_apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  doc["ssid"] = s_apMode ? String(AP_SSID) : WiFi.SSID();
  doc["rssi"] = s_apMode ? 0 : WiFi.RSSI();

  unsigned long up = millis() / 1000UL;
  char ub[32];
  snprintf(ub, sizeof(ub), "%lud %02lu:%02lu:%02lu",
           up / 86400UL, (up / 3600UL) % 24UL, (up / 60UL) % 60UL, up % 60UL);
  doc["uptime"] = ub;

  doc["heap"]  = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  doc["psram"] = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  const char* rr = "?";
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:  rr = "power on"; break;
    case ESP_RST_SW:       rr = "software"; break;
    case ESP_RST_PANIC:    rr = "PANIC"; break;
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:      rr = "WATCHDOG"; break;
    case ESP_RST_BROWNOUT: rr = "BROWNOUT"; break;
    case ESP_RST_EXT:      rr = "reset pin"; break;
    default: break;
  }
  doc["resetReason"] = rr;

  // The time zone, because "the clock is wrong" is otherwise impossible to
  // diagnose remotely. Says where the offset came from: the compiled-in Czech
  // rule, or the forecast for the user's own coordinates.
  {
    char tzb[48];
    const int off = TimeZone_Offset();
    snprintf(tzb, sizeof(tzb), "%s (UTC%+d:%02d, %s)", TimeZone_Text(),
             off / 3600, (off < 0 ? -off : off) % 3600 / 60,
             TimeZone_FromNetwork() ? "podle polohy" : "z Config.h");
    doc["timezone"] = tzb;
  }

  // What the device is showing right now, so the remote control can highlight
  // the active screen and print the range instead of guessing.
  const uint8_t scr = (uint8_t)MPR_CurrentScreen();
  doc["screen"] = scr;
  char rb[24] = "";
  if      (scr == SCREEN_PLANES_I) ScreenPlanes_RangeText(rb, sizeof(rb));
  else if (scr == SCREEN_METEO_I)  ScreenWeather_RangeText(rb, sizeof(rb));
  else if (scr == SCREEN_PRICE_I) ScreenPrice_RangeText(rb, sizeof(rb));
  else if (scr == SCREEN_CALENDAR_I)
    ScreenCalendar_RangeText(rb, sizeof(rb));
  doc["range"] = rb;                       // empty on screens without one

  JsonArray en = doc["enabled"].to<JsonArray>();
  for (uint8_t i = 0; i < SCREEN_N; i++) en.add(Settings_ScreenEnabled(i));

  char b[64];
  Status_Text(ST_ADSB, b, sizeof(b));     doc["adsb"] = b;
  Status_Text(ST_RADAR, b, sizeof(b));    doc["radar"] = b;
  Status_Text(ST_FORECAST, b, sizeof(b)); doc["forecast"] = b;
  if (Settings_Language() == LANG_CZ) {
    Status_Text(ST_PRICE, b, sizeof(b)); doc["price"] = b;
    Status_Text(ST_MIX, b, sizeof(b)); doc["mix"] = b;
  }
  doc["notificationMemory"] = Notifications_MemoryBytes();
  doc["notificationCount"] = Notifications_Count();
  doc["notificationPairingActive"] = s_notificationPairing.active(millis());
  doc["notificationPairingComplete"] = s_notificationPairing.completed();
  doc["notificationRetries"] = Notifications_Dropped();
  doc["notificationBridgeAge"] =
      s_bridgeSeen ? (long)((uint32_t)(millis() - s_bridgeContact) / 1000) : -1;
  doc["calendarMemory"] = Calendar_MemoryBytes();
  JsonArray calendars = doc["calendarSources"].to<JsonArray>();
  for (int i = 0; i < CALENDAR_SOURCES; i++) {
    JsonObject c = calendars.add<JsonObject>();
    c["state"] = Calendar_StateText(Calendar_State(i));
    c["updated"] = (long long)Calendar_Updated(i);
    c["unsupportedTimezone"] = Calendar_UnsupportedTimezone(i);
  }
  Status_Text(ST_STORM, b, sizeof(b)); doc["storm"] = b;
  Status_Text(ST_PLANEWATCH, b, sizeof(b)); doc["planeWatch"] = b;
  sendJson(200, doc);
}

// --- Remembered WiFi networks (0.8.0) ----------------------------------------
// Saves the three slots WITHOUT connecting. The point is to enter the office
// network at home, before the device is carried there; it is tried whenever
// the current network goes away. A missing or empty "pass" keeps whatever is
// stored, because the page never knows the stored passwords.
static void handleWifiList() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  JsonArrayConst nets = doc["nets"];
  if (nets.isNull()) { s_srv.send(400, "application/json", "{\"error\":\"nets\"}"); return; }
  for (uint8_t i = 0; i < WIFI_SLOTS && i < nets.size(); i++) {
    const char* ssid = nets[i]["ssid"] | "";
    const char* pass = nets[i]["pass"] | "";
    Settings_SetWifiSlot(i, ssid, (pass && *pass) ? pass : nullptr);
  }
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

// --- Home Assistant (0.8.0) ---------------------------------------------------
// One flat-ish JSON object for Home Assistant's RESTful integration: poll it
// every minute or so and pull the values out with value_template. Read-only,
// no password - it says nothing the screen does not already show to anyone in
// the room. Off unless switched on in the web UI. NOT TESTED against a real
// Home Assistant: the shape follows the documented REST sensor, but nobody has
// pointed one at it yet.
static const char* const SCREEN_KEYS[SCREEN_N] = {
  "clock", "planes", "meteo", "forecast", "calendar", "price", "mix", "settings" };

static void handleHa() {
  if (!Settings_HaEnabled()) {
    s_srv.send(404, "application/json", "{\"error\":\"disabled\"}");
    return;
  }
  JsonDocument doc;
  doc["version"] = FW_VERSION;
  doc["uptime_s"] = (uint32_t)(millis() / 1000UL);
  doc["ip"]   = WiFi.localIP().toString();
  doc["ssid"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();
  doc["lat"]  = Settings_Lat();
  doc["lon"]  = Settings_Lon();

  const int scr = MPR_CurrentScreen();
  doc["screen"] = (scr >= 0 && scr < SCREEN_N) ? SCREEN_KEYS[scr] : "?";
  doc["night"] = Settings_IsNight();
  doc["brightness"] = Settings_Backlight();

  // Weather. null rather than 0 when not known yet: a 0 degC reading and "no
  // data" must not look the same in a graph.
  if (Forecast_CurrentValid()) {
    doc["temperature"]  = roundf(Forecast_CurrentTemp() * 10) / 10;
    doc["wind_kmh"]     = roundf(Forecast_CurrentWind() * 10) / 10;
    doc["precip_mm"]    = roundf(Forecast_CurrentPrecip() * 10) / 10;
    doc["weather_code"] = Forecast_CurrentCode();
  } else {
    doc["temperature"] = nullptr; doc["wind_kmh"] = nullptr;
    doc["precip_mm"] = nullptr;   doc["weather_code"] = nullptr;
  }
  if (Forecast_DayCount() > 0) {
    const FcDay& d = Forecast_Days()[0];
    doc["today_min"] = roundf(d.tmin * 10) / 10;
    doc["today_max"] = roundf(d.tmax * 10) / 10;
  }
  time_t rise, set;
  if (Forecast_SunTimes(&rise, &set)) {
    char t[8]; struct tm lt;
    localtime_r(&rise, &lt); snprintf(t, sizeof(t), "%02d:%02d", lt.tm_hour, lt.tm_min); doc["sunrise"] = t;
    localtime_r(&set, &lt);  snprintf(t, sizeof(t), "%02d:%02d", lt.tm_hour, lt.tm_min); doc["sunset"] = t;
  }
  if (AirQuality_Valid()) {
    if (AirQuality_Aqi() >= 0) doc["aqi"] = AirQuality_Aqi();
    doc["pm25"] = roundf(AirQuality_Pm25() * 10) / 10;
  }

  // Storm watch. Runs whenever this endpoint is on, so these are live even with
  // the "radar only when a storm is coming" option off.
  {
    JsonObject st = doc["storm"].to<JsonObject>();
    st["active"]    = StormWatch_Active();
    st["valid"]     = StormWatch_Valid();
    st["coverage"]  = StormWatch_InCoverage();
    st["threshold_dbz"] = Settings_StormDbz();
    st["radius_km"] = Settings_StormKm();
    if (StormWatch_Valid()) {
      st["max_dbz"] = StormWatch_MaxDbz() >= 0 ? (int)StormWatch_MaxDbz() : 0;
      if (StormWatch_NearestKm() >= 0) st["nearest_km"] = roundf(StormWatch_NearestKm());
      else                             st["nearest_km"] = nullptr;
      st["area_km2"] = StormWatch_Pixels();
      st["frame"]    = StormWatch_FrameTime();
    }
  }

  // Aircraft. The list is whatever was fetched last: the small watch area
  // while another screen is up, the radar range while the aircraft screen is.
  {
    JsonObject ac = doc["aircraft"].to<JsonObject>();
    ac["count"] = ADSB_DataStale() ? 0 : ADSB_Count();
    if (ADSB_DataAgeMs() != 0xFFFFFFFFUL) ac["data_age_s"] = ADSB_DataAgeMs() / 1000UL;
    ac["stale"] = ADSB_DataStale();
    ac["alert"] = MPR_AlertActive();
    float km = 0, brg = 0; bool win = false;
    const int i = PlaneWatch_Best(&km, &brg, &win);
    if (i >= 0) {
      const Aircraft& a = ADSB_List()[i];
      ac["callsign"]    = a.callsign[0] ? a.callsign : a.hex;
      ac["hex"]         = a.hex;
      ac["type"]        = a.type;
      ac["registration"]= a.reg;
      ac["kind"]        = PlaneIcon_Name(a.icon);
      ac["distance_km"] = roundf(km * 10) / 10;
      ac["bearing"]     = (int)lroundf(brg) % 360;
      ac["altitude_m"]  = (int)lroundf(a.altFt * 0.3048f);
      ac["speed_kmh"]   = (int)lroundf(a.gsKt * 1.852f);
      ac["in_window"]   = win;
      ac["squawk"]      = a.squawk;
    } else {
      ac["callsign"] = nullptr;
      ac["distance_km"] = nullptr;
    }
  }

  sendJson(200, doc);
}

static void handleScan() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n && i < 20; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
  }
  WiFi.scanDelete();
  sendJson(200, doc);
}

static void handleWifi() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  const char* ssid = doc["ssid"] | "";
  const char* pass = doc["pass"] | "";
  if (!*ssid) { s_srv.send(400, "application/json", "{\"error\":\"ssid\"}"); return; }
  Settings_SetWifi(ssid, pass);
  s_wantConnect = true;
  JsonDocument res; res["ok"] = true;
  sendJson(200, res);
}

// Town name -> coordinates, so nobody has to look up their latitude by hand.
// Proxied through the device because the page is served from the device and a
// browser would refuse the cross-origin call.
static void handleGeocode() {
  String q = s_srv.arg("q");
  if (q.length() == 0) { s_srv.send(400, "application/json", "[]"); return; }

  String enc;
  for (size_t i = 0; i < q.length(); i++) {
    char ch = q[i];
    if (isalnum((unsigned char)ch)) enc += ch;
    else { char b[4]; snprintf(b, sizeof(b), "%%%02X", (unsigned char)ch); enc += b; }
  }

  char url[220];
  snprintf(url, sizeof(url), "%s?name=%s&count=8&language=%s&format=json",
           GEOCODE_URL, enc.c_str(), Lang_Get() == LANG_EN ? "en" : (Lang_Get() == LANG_SK ? "sk" : "cs"));

  String body;
  if (!Net_GetString(url, body, "GEOKOD")) { s_srv.send(502, "application/json", "[]"); return; }

  JsonDocument filter;
  JsonObject f = filter["results"].add<JsonObject>();
  f["name"] = true; f["latitude"] = true; f["longitude"] = true; f["country"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter))) {
    s_srv.send(502, "application/json", "[]");
    return;
  }

  JsonDocument out;
  JsonArray arr = out.to<JsonArray>();
  for (JsonObjectConst r : doc["results"].as<JsonArrayConst>()) {
    JsonObject o = arr.add<JsonObject>();
    o["name"] = r["name"];
    o["country"] = r["country"];
    o["lat"] = r["latitude"];
    o["lon"] = r["longitude"];
  }
  sendJson(200, out);
}

static void handleExport() {
  JsonDocument doc;
  JsonObject o = doc.to<JsonObject>();
  Settings_ToJson(o);
  o["version"] = FW_VERSION;
  // Deliberately absent: the WiFi password and the admin password. A backup
  // file ends up in a download folder, in an email, in a support ticket.
  String out;
  serializeJsonPretty(doc, out);
  s_srv.sendHeader("Content-Disposition", "attachment; filename=meteoplaneradar.json");
  s_srv.send(200, "application/json", out);
}

static void handleImport() {
  JsonDocument doc;
  if (!readBody(doc)) { s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return; }
  if (!authed(doc)) return;
  JsonObjectConst cfg = doc["config"];
  if (cfg.isNull()) { s_srv.send(400, "application/json", "{\"error\":\"config\"}"); return; }
  if (const char* error = Settings_ValidateCalendarJson(cfg)) {
    JsonDocument res;
    res["error"] = error;
    sendJson(400, res);
    return;
  }
  if (const char* error = Settings_ValidateNotifications(cfg)) {
    JsonDocument res;
    res["error"] = error;
    sendJson(400, res);
    return;
  }
  Settings_FromJson(cfg);
  NightMode_Apply();
  s_wantRestart = true;
  JsonDocument res; res["ok"] = true;
  sendJson(200, res);
}

static void handleReboot() {
  s_srv.send(200, "application/json", "{\"ok\":true}");
  delay(200);
  ESP.restart();
}

static void handleReset() {
  JsonDocument doc;
  readBody(doc);
  if (!authed(doc)) return;
  s_srv.send(200, "application/json", "{\"ok\":true}");
  delay(200);
  Settings_ClearAll();
  ESP.restart();
}

// Captive portal: whatever the phone asks for, hand it the setup page. Without
// this the "sign in to network" banner never appears and the user is left
// typing an IP address they have not been told.
static void handleNotFound() {
  if (s_apMode) {
    s_srv.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    s_srv.send(302, "text/plain", "");
    return;
  }
  s_srv.send(404, "text/plain", "404");
}

// --- OTA callbacks ----------------------------------------------------------
// The RGB panel streams its framebuffer out of PSRAM continuously, and writing
// flash suspends the cache from under it - the picture tears and jumps. Nothing
// the drawing code can do about it, so the backlight goes off for the duration
// and the browser shows the real progress bar.
static void otaStart() {
  const bool en = (Lang_Get() == LANG_EN);
  s_updating = true;
  gfx->fillScreen(C_BLACK);
  UI_TextCentered(en ? "Updating firmware..." : (Lang_Get() == LANG_SK ? "Prebieha aktualizacia..." : "Probiha aktualizace..."),
                  LCD_HEIGHT / 2 - 10, C_WHITE, 2);
  UI_TextCentered(en ? "Do not disconnect power" : (Lang_Get() == LANG_SK ? "Neodpajaj napajanie" : "Neodpojuj napajeni"),
                  LCD_HEIGHT / 2 + 20, C_GRAY, 1);
  gfx->flush();
  delay(700);
  Set_Backlight(0);
}

static void otaEnd(bool ok) {
  const bool en = (Lang_Get() == LANG_EN);
  Set_Backlight(Settings_Backlight());
  gfx->fillScreen(C_BLACK);
  UI_TextCentered(ok ? (en ? "Done, restarting..." : (Lang_Get() == LANG_SK ? "Hotovo, restartujem..." : "Hotovo, restartuji..."))
                     : (en ? "Update failed"       : (Lang_Get() == LANG_SK ? "Aktualizacia zlyhala" : "Aktualizace selhala")),
                  LCD_HEIGHT / 2, ok ? C_GREEN : C_RED, 2);
  gfx->flush();
  s_updating = false;
}

// --- Firmware update --------------------------------------------------------
// This replaces the ElegantOTA library. That library is AGPL-3.0, which would
// have made every binary built from this project AGPL too; the Update class
// ships with the ESP32 core and the web server was already here, so the page
// below was the only piece actually missing.

static bool   s_updOk = false;
static String s_updErr;          // empty = no failure yet

// HTTP Basic, and only when a password is set - the open default is documented
// in Settings.h and in the README.
static bool updateAuthed() {
  if (!Settings_HasAdminPassword()) return true;
  return s_srv.authenticate(WEB_ADMIN_USER, Settings_AdminPassword());
}

static const char UPDATE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="{{LANG}}"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>{{TITLE}}</title><style>
body{background:#111;color:#eee;font:16px system-ui,sans-serif;margin:0;padding:24px;
 max-width:520px;margin-inline:auto}
h1{font-size:20px;margin:0 0 4px}
p{color:#aaa;line-height:1.5}
input[type=file]{width:100%;padding:12px;background:#1c1c1c;border:1px solid #333;
 border-radius:8px;color:#eee;box-sizing:border-box}
button{width:100%;padding:14px;margin-top:12px;font-size:16px;border:0;border-radius:8px;
 background:#2d7;color:#000;font-weight:600;cursor:pointer}
button:disabled{background:#444;color:#888;cursor:default}
progress{width:100%;height:10px;margin-top:16px}
#s{margin-top:10px;min-height:1.4em;font-weight:600}
a{color:#5bf}
</style></head><body>
<h1>{{TITLE}}</h1>
<p>{{HINT}}</p>
<input type="file" id="f" accept=".bin">
<button id="b">{{SEND}}</button>
<progress id="p" value="0" max="100"></progress>
<div id="s"></div>
<p><a href="/">{{BACK}}</a></p>
<script>
var f=document.getElementById('f'),b=document.getElementById('b'),
    p=document.getElementById('p'),s=document.getElementById('s');
b.onclick=function(){
 if(!f.files.length){s.textContent='{{PICK}}';return;}
 var fd=new FormData();fd.append('update',f.files[0]);
 var x=new XMLHttpRequest();x.open('POST','/update');
 x.upload.onprogress=function(e){if(e.lengthComputable){
   var v=Math.round(e.loaded/e.total*100);p.value=v;s.textContent=v+' %';}};
 x.onload=function(){b.disabled=false;f.disabled=false;
   if(x.status==200){p.value=100;s.textContent='{{OK}}';}
   else{s.textContent='{{FAIL}}: '+(x.responseText||x.status);}};
 x.onerror=function(){b.disabled=false;f.disabled=false;s.textContent='{{FAIL}}';};
 b.disabled=true;f.disabled=true;s.textContent='0 %';
 x.send(fd);};
</script></body></html>)rawliteral";

static void handleUpdatePage() {
  if (!updateAuthed()) { s_srv.requestAuthentication(); return; }
  const bool en = (Lang_Get() == LANG_EN);
  String p = FPSTR(UPDATE_HTML);
  p.replace("{{LANG}}",  en ? "en" : (Lang_Get() == LANG_SK ? "sk" : "cs"));
  p.replace("{{TITLE}}", en ? "Firmware update" : (Lang_Get() == LANG_SK ? "Aktualizácia firmvéru" : "Aktualizace firmwaru"));
  p.replace("{{HINT}}",  en ? "Pick the <b>.ino.bin</b> file (the one without "
                              "<i>merged</i>). The display goes dark while the "
                              "flash is written and comes back on when it is done."
                            : Lang_Get() == LANG_SK
                            ? "Vyberte súbor <b>.ino.bin</b> (ten bez <i>merged</i>). Displej počas zápisu zhasne a po dokončení sa znova rozsvieti."
                            : "Vyberte soubor <b>.ino.bin</b> (ten bez <i>merged</i>). "
                              "Displej po dobu zapisu zhasne a po dokonceni se "
                              "sam rozsviti.");
  p.replace("{{SEND}}",  en ? "Upload"           : (Lang_Get() == LANG_SK ? "Nahrať" : "Nahrat"));
  p.replace("{{BACK}}",  en ? "Back to settings" : (Lang_Get() == LANG_SK ? "Späť na nastavenia" : "Zpet na nastaveni"));
  p.replace("{{PICK}}",  en ? "Pick a file first." : (Lang_Get() == LANG_SK ? "Najprv vyberte súbor." : "Nejdriv vyberte soubor."));
  p.replace("{{OK}}",    en ? "Done. The device is restarting."
                            : (Lang_Get() == LANG_SK ? "Hotovo. Zariadenie sa reštartuje." : "Hotovo. Zarizeni se restartuje."));
  p.replace("{{FAIL}}",  en ? "Update failed"    : (Lang_Get() == LANG_SK ? "Aktualizacia zlyhala" : "Aktualizace selhala"));
  s_srv.sendHeader("Cache-Control", "no-store");
  s_srv.send(200, "text/html; charset=utf-8", p);
}

// Called repeatedly by WebServer as the body arrives. The whole transfer runs
// inside one handleClient(), so nothing else can be drawing meanwhile - but the
// watchdog still has to be fed by hand.
static void handleUpdateUpload() {
  HTTPUpload& up = s_srv.upload();

  switch (up.status) {
    case UPLOAD_FILE_START:
      s_updOk = false;
      s_updErr = "";
      if (!updateAuthed()) { s_updErr = "auth"; return; }
      Serial.printf("OTA: %s\n", up.filename.c_str());
      otaStart();
      // The browser does not announce the image size up front, so let Update
      // take the whole free OTA slot.
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        s_updErr = Update.errorString();
        otaEnd(false);
      }
      break;

    case UPLOAD_FILE_WRITE:
      if (s_updErr.length()) return;              // already failed, drain the body
      if (Update.write(up.buf, up.currentSize) != up.currentSize) {
        s_updErr = Update.errorString();
        Update.abort();
        otaEnd(false);
        return;
      }
      Watchdog_Feed();
      break;

    case UPLOAD_FILE_END:
      if (s_updErr.length()) return;
      if (Update.end(true)) {
        s_updOk = true;
        Serial.printf("OTA: hotovo, %u B\n", (unsigned)up.totalSize);
        otaEnd(true);
      } else {
        s_updErr = Update.errorString();
        otaEnd(false);
      }
      break;

    case UPLOAD_FILE_ABORTED:
      if (s_updErr == "auth") return;             // never started, keep the 401
      if (Update.isRunning()) Update.abort();
      if (s_updating) otaEnd(false);              // only if the screen was taken over
      s_updErr = "aborted";
      break;
  }
}

// Runs once the body has been consumed, so this is where the verdict is sent.
static void handleUpdateDone() {
  if (s_updErr == "auth") { s_updErr = ""; s_srv.requestAuthentication(); return; }
  s_srv.sendHeader("Connection", "close");
  if (s_updOk) {
    s_srv.send(200, "text/plain", "OK");
    delay(400);
    ESP.restart();
    return;
  }
  s_srv.send(500, "text/plain", s_updErr.length() ? s_updErr : String("update failed"));
  s_updErr = "";
}

static void handleCalendarRefresh() {
  JsonDocument doc;
  if (!readBody(doc)) {
    s_srv.send(400, "application/json", "{}");
    return;
  }
  if (!authed(doc)) return;
  Calendar_Refresh();
  s_srv.send(200, "application/json", "{\"ok\":true}");
}

// --- Lifecycle --------------------------------------------------------------
// Notification ingress is deliberately independent of the optional admin password.
// A random, revocable pairing key grants only delivery, never config access.
// Pairing is explicitly opened in the authenticated web UI, never discoverable
// through config/status. Codes expire, have a fixed guess budget and are single-use.
static bool readPairBody(JsonDocument& doc) {
  s_srv.sendHeader("Cache-Control", "no-store");
  if (s_apMode) { s_srv.send(403, "application/json", "{\"error\":\"apMode\"}"); return false; }
  if (!s_srv.hasArg("plain") || s_srv.arg("plain").length() > 512 || !readBody(doc)) {
    s_srv.send(400, "application/json", "{\"error\":\"json\"}"); return false;
  }
  return true;
}
static void handlePairStart() {
  JsonDocument doc;
  if (!readPairBody(doc) || !authed(doc)) return;
  const char* code = s_notificationPairing.begin(millis(), esp_random());
  JsonDocument response;
  response["code"] = code; response["expiresIn"] = NotificationPairing::TTL_MS / 1000;
  sendJson(200, response);
}
static void handlePairCancel() {
  JsonDocument doc;
  if (!readPairBody(doc) || !authed(doc)) return;
  s_notificationPairing.cancel();
  s_srv.send(200, "application/json", "{\"ok\":true}");
}
static void handlePairClaim() {
  JsonDocument doc;
  if (!readPairBody(doc)) return;
  if (!s_notificationPairing.active(millis())) {
    s_srv.send(410, "application/json", "{\"error\":\"expired\"}"); return;
  }
  const char* code = doc["code"] | "";
  if (!s_notificationPairing.take(code, millis())) {
    s_srv.send(403, "application/json", "{\"error\":\"code\"}"); return;
  }
  uint8_t bytes[32]; esp_fill_random(bytes, sizeof(bytes));
  char key[65];
  for (unsigned i = 0; i < sizeof(bytes); ++i) snprintf(key + 2*i, 3, "%02x", bytes[i]);
  JsonDocument settings;
  settings["notifications"]["key"] = key;
  settings["notifications"]["enabled"] = true;
  Settings_FromJson(settings.as<JsonObjectConst>());
  JsonDocument response; response["key"] = key;
  sendJson(200, response);
  memset(key, 0, sizeof(key)); memset(bytes, 0, sizeof(bytes));
}
static bool notificationAuth() {
  String header = s_srv.header("Authorization");
  if (s_apMode || !header.startsWith("Bearer ") || !Notifications_Authorized(header.c_str() + 7)) {
    s_srv.send(403, "application/json", "{\"error\":\"pairing\"}");
    return false;
  }
  s_bridgeContact = millis();
  s_bridgeSeen = true;
  return true;
}
static void handleNotificationPush() {
  if (!notificationAuth()) return;
  if (!s_srv.hasArg("plain") || s_srv.arg("plain").length() > 2048) {
    s_srv.send(413, "application/json", "{\"error\":\"size\"}");
    return;
  }
  JsonDocument doc;
  if (!readBody(doc)) {
    s_srv.send(400, "application/json", "{\"error\":\"json\"}");
    return;
  }
  if (const char* error = Notifications_Validate(doc.as<JsonObjectConst>())) {
    JsonDocument res;
    res["error"] = error;
    sendJson(400, res);
    return;
  }
  if (!Settings_NotificationsEnabled()) {
    s_srv.send(409, "application/json", "{\"error\":\"disabled\"}");
    return;
  }
  if (!Notifications_Push(doc.as<JsonObjectConst>())) {
    s_srv.sendHeader("Retry-After", "15");
    s_srv.send(503, "application/json", "{\"error\":\"queueOrMemory\"}");
    return;
  }
  s_srv.send(200, "application/json", "{\"ok\":true}");
}
static void handleNotificationPing() {
  if (!notificationAuth()) return;
  JsonDocument doc;
  doc["enabled"] = Settings_NotificationsEnabled();
  doc["queued"] = Notifications_Count();
  doc["version"] = FW_VERSION;
  sendJson(200, doc);
}
static void handleNotificationTest() {
  JsonDocument doc;
  if (!readBody(doc)) {
    s_srv.send(400, "application/json", "{\"error\":\"json\"}");
    return;
  }
  if (!authed(doc)) return;
  doc.clear();
  char id[48];
  snprintf(id, sizeof(id), "test-%lu", (unsigned long)millis());
  doc["id"] = id;
  doc["service"] = "notification";
  doc["sender"] = "MeteoPlaneRadar";
  doc["room"] = "Test";
  doc["text"] =
      Settings_Language() == LANG_EN
          ? "Notifications work. Tap the banner for details, or close it with the large X."
          : "Notifikacie funguju. Tukni na banner pre detail alebo ho zavri velkym krizikom.";
  bool ok = !s_apMode && Notifications_Push(doc.as<JsonObjectConst>());
  s_srv.send(ok ? 200 : 409, "application/json",
             ok ? "{\"ok\":true}" : "{\"error\":\"enableOrQueue\"}");
}

void WebConfig_Begin(bool apMode) {
  s_apMode = apMode;
  if (apMode) s_notificationPairing.cancel();

  // The handlers are registered ONCE for the life of the process. This function
  // is called at least twice in a normal boot - first for the access point,
  // then again after joining the home network - and WebServer::on() appends to
  // a list rather than replacing, so registering again would leave a duplicate
  // of every route behind.
  if (s_running) {
    // Only the role-specific parts change.
    if (apMode) {
      s_dns.setErrorReplyCode(DNSReplyCode::NoError);
      s_dns.start(53, "*", WiFi.softAPIP());
    } else {
      s_dns.stop();
      if (MDNS.begin(WEB_HOSTNAME)) MDNS.addService("http", "tcp", WEB_PORT);
      Serial.printf("Web: http://%s.local/ nebo http://%s/\n",
                    WEB_HOSTNAME, WiFi.localIP().toString().c_str());
    }
    return;
  }

  const char* headers[] = {"Authorization"};
  s_srv.collectHeaders(headers, 1);
  s_srv.on("/api/notifications/pair/start", HTTP_POST, handlePairStart);
  s_srv.on("/api/notifications/pair/cancel", HTTP_POST, handlePairCancel);
  s_srv.on("/api/notifications/pair/claim", HTTP_POST, handlePairClaim);
  s_srv.on("/api/notifications", HTTP_POST, handleNotificationPush);
  s_srv.on("/api/notifications/ping", HTTP_GET, handleNotificationPing);
  s_srv.on("/api/notifications/test", HTTP_POST, handleNotificationTest);
  s_srv.on("/", HTTP_GET, handleRoot);
  s_srv.on("/api/config", HTTP_GET, handleGetConfig);
  s_srv.on("/api/config", HTTP_POST, handlePostConfig);
  s_srv.on("/api/status", HTTP_GET, handleStatus);
  s_srv.on("/api/calendar/refresh", HTTP_POST, handleCalendarRefresh);
  s_srv.on("/api/screen", HTTP_POST, handleScreen);
  s_srv.on("/api/range", HTTP_POST, handleRange);
  s_srv.on("/api/scan", HTTP_GET, handleScan);
  s_srv.on("/api/wifi", HTTP_POST, handleWifi);
  s_srv.on("/api/wifilist", HTTP_POST, handleWifiList);
  s_srv.on("/api/ha", HTTP_GET, handleHa);
  s_srv.on("/api/geocode", HTTP_GET, handleGeocode);
  s_srv.on("/api/export", HTTP_GET, handleExport);
  s_srv.on("/api/import", HTTP_POST, handleImport);
  s_srv.on("/api/reboot", HTTP_POST, handleReboot);
  s_srv.on("/api/reset", HTTP_POST, handleReset);
  // The update page authenticates with HTTP Basic when a password is set. See
  // the note in Settings.h about why the password is stored in the clear.
  s_srv.on("/update", HTTP_GET, handleUpdatePage);
  s_srv.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
  s_srv.onNotFound(handleNotFound);

  s_srv.begin();
  s_running = true;

  if (apMode) {
    s_dns.setErrorReplyCode(DNSReplyCode::NoError);
    s_dns.start(53, "*", WiFi.softAPIP());
    Serial.printf("Web: portal na http://%s/\n", WiFi.softAPIP().toString().c_str());
  } else {
    if (MDNS.begin(WEB_HOSTNAME)) {
      MDNS.addService("http", "tcp", WEB_PORT);
      Serial.printf("Web: http://%s.local/ nebo http://%s/\n",
                    WEB_HOSTNAME, WiFi.localIP().toString().c_str());
    } else {
      Serial.printf("Web: http://%s/  (mDNS se nespustilo)\n",
                    WiFi.localIP().toString().c_str());
    }
  }
}

void WebConfig_Loop() {
  if (!s_running) return;
  if (s_apMode) s_dns.processNextRequest();
  s_srv.handleClient();
}
