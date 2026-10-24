// =============================================================================
//  MeteoPlaneRadar
//  Persisted settings - storage in NVS + JSON serialisation for the web UI.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Settings.h"
#include "Lang.h"
#include <Preferences.h>
#include <string.h>
#include <ctype.h>

static CalendarSourceSettings s_cal[CALENDAR_SOURCES] = {};
static uint8_t s_calView = 0;
static uint16_t s_calRefresh = 15;
static uint32_t s_calRevision = 1;
static void calendarDefaults() {
  const uint16_t colors[] = {0x05FF, 0xFD20, 0x07E0};
  for (int i = 0; i < CALENDAR_SOURCES; i++) {
    s_cal[i] = {};
    snprintf(s_cal[i].name, sizeof(s_cal[i].name), "Calendar %d", i + 1);
    s_cal[i].color = colors[i];
    s_cal[i].enabled = true;
  }
}
static bool s_notify = false;
static uint16_t s_notifySeconds = 15;
static char s_notifyKey[65] = "";
static Preferences prefs;
static const char* NS = "planeradar";

// --- WiFi ---
static char s_ssid[33] = "";
static char s_wpass[65] = "";

// --- Location ---
static double  s_lat = DEFAULT_LAT;
static double  s_lon = DEFAULT_LON;
static bool    s_hasLoc = false;

// --- Brightness / night mode ---
static uint8_t s_briDay   = 80;
static uint8_t s_briNight = 25;
static bool    s_nightAuto = true;
static int8_t  s_nightOff  = 0;
static bool    s_isNight   = false;

// --- Misc ---
static bool    s_metric = false;
static uint8_t s_lang   = LANG_CZ;

static uint8_t s_scrMask = (1 << SCREEN_CLOCK_I) | (1 << SCREEN_PLANES_I) |
                           (1 << SCREEN_METEO_I) | (1 << SCREEN_FORECAST_I);
static uint16_t s_autoRot = 0;
static uint8_t s_radarSrc = RADAR_SRC_CHMU;
static bool    s_mtLegend = true;

// --- Clock appearance ---
static uint8_t  s_secStyle = SEC_STYLE_DOTS;
static uint16_t s_clockCol = 0xFFFF;   // white
static uint16_t s_secCol   = 0x05FF;   // cyan

// --- Aircraft filters ---
static uint16_t s_altMin = 0;
static uint16_t s_altMax = 60000;
static bool     s_onlyCs = false;
static bool     s_sqAlert = true;
static char     s_watch[10] = "";

// --- UI state ---
static uint8_t  s_rngP = 1;
static uint8_t  s_rngM = 1;
static uint8_t  s_scr  = SCREEN_PLANES_I;
static uint16_t s_top  = 0;

// --- Admin password (see the note in Settings.h) ---
static char s_pw[33] = "";

static bool          s_uiDirty   = false;
static unsigned long s_uiDirtyAt = 0;
static void markDirty() { s_uiDirty = true; s_uiDirtyAt = millis(); }

// Immediate write for the settings that are changed rarely and deliberately
// (web UI, portal) rather than by dragging a finger.
static void putU8(const char* k, uint8_t v) {
  if (prefs.begin(NS, false)) { prefs.putUChar(k, v); prefs.end(); }
}
static void putU16(const char* k, uint16_t v) {
  if (prefs.begin(NS, false)) { prefs.putUShort(k, v); prefs.end(); }
}
static void putBool(const char* k, bool v) {
  if (prefs.begin(NS, false)) { prefs.putBool(k, v); prefs.end(); }
}
static void putStr(const char* k, const char* v) {
  if (prefs.begin(NS, false)) { prefs.putString(k, v); prefs.end(); }
}

void Settings_Begin() {
  bool migrateRotate = false, migrateCalendar = false;
  calendarDefaults();
  if (prefs.begin(NS, true)) {
    s_notify = prefs.getBool("ntOn", false);
    s_notifySeconds = prefs.getUShort("ntSec", 15);
    if (s_notifySeconds && (s_notifySeconds < 5 || s_notifySeconds > 120)) s_notifySeconds = 15;
    prefs.getString("ntKey", s_notifyKey, sizeof(s_notifyKey));
    s_lat    = prefs.getDouble("lat", DEFAULT_LAT);
    s_lon    = prefs.getDouble("lon", DEFAULT_LON);
    s_hasLoc = prefs.getBool("hasLoc", false);
    // "bl" was the single brightness value up to 0.5.5 - reuse it as the day
    // level so an updated device does not suddenly go dark.
    s_briDay   = prefs.getUChar("bl", 80);
    s_briNight = prefs.getUChar("blN", 25);
    s_nightAuto = prefs.getBool("nAuto", true);
    s_nightOff  = (int8_t)prefs.getChar("nOff", 0);
    s_metric = prefs.getBool("metric", false);
    s_lang   = prefs.getUChar("lang", LANG_CZ);
    // Ignore retired screen bits; an older device may have enabled only those.
    s_scrMask = prefs.getUChar("scrM", s_scrMask) & ((1 << SCREEN_SETTINGS_I) - 1);
    if (!s_scrMask) s_scrMask = 1 << SCREEN_CLOCK_I;
    // Cycling interval moved from minutes to seconds - see Settings.h. The old
    // key is converted exactly once, so an updated device keeps its setting.
    if (prefs.isKey("autoRS")) {
      s_autoRot = prefs.getUShort("autoRS", 0);
    } else {
      s_autoRot = (uint16_t)prefs.getUChar("autoR", 0) * 60;
      migrateRotate = true;          // written below, the handle is read-only here
    }
    s_radarSrc = prefs.getUChar("radSrc", RADAR_SRC_CHMU);
    s_mtLegend = prefs.getBool("mtLeg", true);
    s_secStyle = prefs.getUChar("secSt", SEC_STYLE_DOTS);
    s_clockCol = prefs.getUShort("clkC", 0xFFFF);
    s_secCol   = prefs.getUShort("secC", 0x05FF);
    s_altMin = prefs.getUShort("altLo", 0);
    s_altMax = prefs.getUShort("altHi", 60000);
    s_onlyCs = prefs.getBool("onlyCs", false);
    s_sqAlert = prefs.getBool("sqAl", true);
    prefs.getString("watch", s_watch, sizeof(s_watch));
    s_rngP   = prefs.getUChar("rngP", 1);
    s_rngM   = prefs.getUChar("rngM", 1);
    s_scr    = prefs.getUChar("scr", SCREEN_PLANES_I);
    if (!prefs.getBool("calV", false)) {
      s_scrMask &= 0x0f;
      if (s_scr == 4) s_scr = SCREEN_SETTINGS_I;
      migrateCalendar = true;
    }
    s_calView = prefs.getUChar("calView", 0) == 1 ? 1 : 0;
    s_calRefresh = prefs.getUShort("calPoll", 15);
    if (s_calRefresh < 5 || s_calRefresh > 1440) s_calRefresh = 15;
    for (int i = 0; i < CALENDAR_SOURCES; i++) {
      char key[16];
      snprintf(key, sizeof(key), "cal%dName", i);
      prefs.getString(key, s_cal[i].name, sizeof(s_cal[i].name));
      snprintf(key, sizeof(key), "cal%dUrl", i);
      prefs.getString(key, s_cal[i].url, sizeof(s_cal[i].url));
      snprintf(key, sizeof(key), "cal%dColor", i);
      s_cal[i].color = prefs.getUShort(key, s_cal[i].color);
      snprintf(key, sizeof(key), "cal%dOn", i);
      s_cal[i].enabled = prefs.getBool(key, true);
    }
    // Map any retired screen or the previous Settings index to Settings.
    if (s_scr >= SCREEN_SETTINGS_I) s_scr = SCREEN_SETTINGS_I;
    s_top    = prefs.getUShort("topb", 0);
    prefs.getString("pw", s_pw, sizeof(s_pw));
    prefs.getString("ssid", s_ssid, sizeof(s_ssid));
    prefs.getString("wpass", s_wpass, sizeof(s_wpass));
    prefs.end();
  }
  if (s_altMax == 0) s_altMax = 60000;
  if (s_autoRot > 3600) s_autoRot = 3600;
  if (migrateRotate && prefs.begin(NS, false)) {
    prefs.putUShort("autoRS", s_autoRot);
    prefs.remove("autoR");           // the old key would only confuse later
    prefs.end();
    if (s_autoRot) Serial.printf("Nastaveni: stridani prevedeno na %u s\n", s_autoRot);
  }
  if (migrateCalendar && prefs.begin(NS, false)) {
    prefs.putBool("calV", true);
    prefs.putUChar("scrM", s_scrMask);
    prefs.putUChar("scr", s_scr);
    prefs.end();
  }
  if (!s_scrMask) s_scrMask = 1 << SCREEN_CLOCK_I;
  Lang_Set(s_lang);
  s_lang = Lang_Get();
}

// --- WiFi -------------------------------------------------------------------
const char* Settings_WifiSsid() { return s_ssid; }
const char* Settings_WifiPass() { return s_wpass; }
bool        Settings_HasWifi()  { return s_ssid[0] != '\0'; }

void Settings_SetWifi(const char* ssid, const char* pass) {
  if (!ssid) ssid = "";
  if (!pass) pass = "";
  strncpy(s_ssid,  ssid, sizeof(s_ssid) - 1);   s_ssid[sizeof(s_ssid) - 1] = '\0';
  strncpy(s_wpass, pass, sizeof(s_wpass) - 1);  s_wpass[sizeof(s_wpass) - 1] = '\0';
  if (prefs.begin(NS, false)) {
    prefs.putString("ssid", s_ssid);
    prefs.putString("wpass", s_wpass);
    prefs.end();
  }
}

void Settings_ClearWifi() { Settings_SetWifi("", ""); }

// --- Location ---------------------------------------------------------------
double Settings_Lat() { return s_lat; }
double Settings_Lon() { return s_lon; }
bool   Settings_HasLocation() { return s_hasLoc; }

void Settings_SetLocation(double lat, double lon) {
  s_lat = lat; s_lon = lon; s_hasLoc = true;
  if (prefs.begin(NS, false)) {
    prefs.putDouble("lat", lat);
    prefs.putDouble("lon", lon);
    prefs.putBool("hasLoc", true);
    prefs.end();
  }
}

// --- Brightness -------------------------------------------------------------
uint8_t Settings_Backlight()   { return s_isNight ? s_briNight : s_briDay; }
uint8_t Settings_BrightDay()   { return s_briDay; }
uint8_t Settings_BrightNight() { return s_briNight; }

// Dragging the slider used to write flash on every touch sample - dozens of
// erase/write cycles for one adjustment. It goes through the same debounce as
// the rest of the UI state.
void Settings_SetBacklight(uint8_t pct) {
  if (s_isNight) { if (pct == s_briNight) return; s_briNight = pct; }
  else           { if (pct == s_briDay)   return; s_briDay   = pct; }
  markDirty();
}
void Settings_SetBrightDay(uint8_t pct)   { if (pct != s_briDay)   { s_briDay = pct;   markDirty(); } }
void Settings_SetBrightNight(uint8_t pct) { if (pct != s_briNight) { s_briNight = pct; markDirty(); } }
bool Settings_NightAuto() { return s_nightAuto; }
void Settings_SetNightAuto(bool on) { s_nightAuto = on; putBool("nAuto", on); }
int8_t Settings_NightOffsetMin() { return s_nightOff; }
void   Settings_SetNightOffsetMin(int8_t m) {
  if (m >  NIGHT_OFFSET_MIN_LIMIT) m =  NIGHT_OFFSET_MIN_LIMIT;
  if (m < -NIGHT_OFFSET_MIN_LIMIT) m = -NIGHT_OFFSET_MIN_LIMIT;
  s_nightOff = m;
  if (prefs.begin(NS, false)) { prefs.putChar("nOff", m); prefs.end(); }
}
bool Settings_IsNight() { return s_isNight; }
void Settings_SetNight(bool night) { s_isNight = night; }

// --- Units, language --------------------------------------------------------
bool Settings_MetricUnits() { return s_metric; }
void Settings_SetMetricUnits(bool metric) { s_metric = metric; putBool("metric", metric); }
uint8_t Settings_Language() { return s_lang; }
void    Settings_SetLanguage(uint8_t l) {
  s_lang = (l <= LANG_SK) ? l : LANG_CZ;
  Lang_Set(s_lang);
  putU8("lang", s_lang);
}

// --- Screens ----------------------------------------------------------------
bool Settings_ScreenEnabled(uint8_t idx) {
  if (idx == SCREEN_SETTINGS_I) return true;      // always reachable
  if (idx >= SCREEN_SETTINGS_I) return false;
  return (s_scrMask >> idx) & 1;
}

void Settings_SetScreenEnabled(uint8_t idx, bool on) {
  if (idx >= SCREEN_SETTINGS_I) return;
  uint8_t next = on ? (s_scrMask | (1 << idx)) : (s_scrMask & ~(1 << idx));
  // Never allow the last data screen to be turned off. With all of them gone
  // the device would boot into Settings and show nothing else - technically
  // recoverable, but it looks broken.
  if (next == 0) return;
  s_scrMask = next;
  putU8("scrM", s_scrMask);
}

uint8_t Settings_EnabledCount() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < SCREEN_SETTINGS_I; i++) if (Settings_ScreenEnabled(i)) n++;
  return n;
}

uint16_t Settings_AutoRotateSec() { return s_autoRot; }
void     Settings_SetAutoRotateSec(uint16_t s) {
  if (s > 3600) s = 3600;
  s_autoRot = s;
  putU16("autoRS", s);
}

// --- Weather radar ----------------------------------------------------------
uint8_t Settings_RadarSource() { return s_radarSrc; }
void    Settings_SetRadarSource(uint8_t s) {
  s_radarSrc = (s == RADAR_SRC_RAINVIEWER) ? RADAR_SRC_RAINVIEWER : RADAR_SRC_CHMU;
  putU8("radSrc", s_radarSrc);
}

bool Settings_MeteoLegend() { return s_mtLegend; }
void Settings_SetMeteoLegend(bool on) { s_mtLegend = on; putBool("mtLeg", on); }

// --- Clock appearance -------------------------------------------------------
uint8_t  Settings_SecondsStyle() { return s_secStyle; }
void     Settings_SetSecondsStyle(uint8_t s) { if (s > SEC_STYLE_COMET) s = 0; s_secStyle = s; putU8("secSt", s); }
uint16_t Settings_ClockColor() { return s_clockCol; }
void     Settings_SetClockColor(uint16_t c) { s_clockCol = c; putU16("clkC", c); }
uint16_t Settings_SecondsColor() { return s_secCol; }
void     Settings_SetSecondsColor(uint16_t c) { s_secCol = c; putU16("secC", c); }

// --- Aircraft filters -------------------------------------------------------
uint16_t Settings_AltMinFt() { return s_altMin; }
uint16_t Settings_AltMaxFt() { return s_altMax; }
void     Settings_SetAltRangeFt(uint16_t lo, uint16_t hi) {
  if (hi <= lo) { lo = 0; hi = 60000; }           // nonsense range = no filter
  s_altMin = lo; s_altMax = hi;
  if (prefs.begin(NS, false)) {
    prefs.putUShort("altLo", lo); prefs.putUShort("altHi", hi); prefs.end();
  }
}
bool Settings_OnlyWithCallsign() { return s_onlyCs; }
void Settings_SetOnlyWithCallsign(bool on) { s_onlyCs = on; putBool("onlyCs", on); }
bool Settings_SquawkAlert() { return s_sqAlert; }
void Settings_SetSquawkAlert(bool on) { s_sqAlert = on; putBool("sqAl", on); }
const char* Settings_WatchCallsign() { return s_watch; }
void Settings_SetWatchCallsign(const char* s) {
  if (!s) s = "";
  strncpy(s_watch, s, sizeof(s_watch) - 1);
  s_watch[sizeof(s_watch) - 1] = '\0';
  // Compared against callsigns and hex addresses, both of which arrive upper
  // case from adsb.fi - normalise here so the user does not have to.
  for (char* p = s_watch; *p; p++) *p = toupper((unsigned char)*p);
  putStr("watch", s_watch);
}

// --- UI state ---------------------------------------------------------------
uint8_t Settings_PlaneRange() { return s_rngP; }
void    Settings_SetPlaneRange(uint8_t idx) { if (idx != s_rngP) { s_rngP = idx; markDirty(); } }
uint8_t Settings_MeteoRange() { return s_rngM; }
void    Settings_SetMeteoRange(uint8_t idx) { if (idx != s_rngM) { s_rngM = idx; markDirty(); } }
uint16_t Settings_TopBearing() { return s_top; }
void     Settings_SetTopBearing(uint16_t deg) {
  deg %= 360;
  if (deg != s_top) { s_top = deg; markDirty(); }
}
uint8_t Settings_Screen() { return s_scr; }
void    Settings_SetScreen(uint8_t idx) { if (idx != s_scr) { s_scr = idx; markDirty(); } }

// --- Admin password ---------------------------------------------------------
bool Settings_HasAdminPassword() { return s_pw[0] != '\0'; }
const char* Settings_AdminPassword() { return s_pw; }

void Settings_SetAdminPassword(const char* plain) {
  if (!plain) plain = "";
  // A single space is the agreed way to say "remove the password" from the web
  // form, where an empty field has to mean "leave it alone" - otherwise every
  // save without retyping it would silently switch the protection off.
  if (strcmp(plain, " ") == 0) plain = "";
  strncpy(s_pw, plain, sizeof(s_pw) - 1);
  s_pw[sizeof(s_pw) - 1] = '\0';
  putStr("pw", s_pw);
}

bool Settings_CheckAdminPassword(const char* plain) {
  if (!Settings_HasAdminPassword()) return true;    // protection disabled
  if (!plain) return false;
  // Constant time over the full buffer: comparing only up to the first
  // difference leaks how much of a guess was right.
  uint8_t diff = 0;
  size_t n = sizeof(s_pw);
  for (size_t i = 0; i < n; i++) {
    char a = (i < strlen(plain)) ? plain[i] : '\0';
    diff |= (uint8_t)(a ^ s_pw[i]);
  }
  return diff == 0;
}

// --- Serialisation ----------------------------------------------------------
void Settings_ToJson(JsonObject o) {
  o["lat"] = s_lat;
  o["lon"] = s_lon;
  o["hasLoc"] = s_hasLoc;
  o["lang"] = s_lang;
  o["metric"] = s_metric;
  o["briDay"] = s_briDay;
  o["briNight"] = s_briNight;
  o["nightAuto"] = s_nightAuto;
  o["nightOffset"] = s_nightOff;
  o["radarSrc"] = s_radarSrc;
  o["meteoLegend"] = s_mtLegend;
  o["autoRotate"] = s_autoRot;   // seconds
  o["topBearing"] = s_top;
  o["secStyle"] = s_secStyle;
  o["clockColor"] = s_clockCol;
  o["secColor"] = s_secCol;
  o["altMin"] = s_altMin;
  o["altMax"] = s_altMax;
  o["onlyCallsign"] = s_onlyCs;
  o["squawkAlert"] = s_sqAlert;
  o["watch"] = s_watch;
  o["hasPassword"] = Settings_HasAdminPassword();
  JsonObject notification = o["notifications"].to<JsonObject>();
  notification["enabled"] = s_notify;
  notification["seconds"] = s_notifySeconds;
  notification["paired"] = strlen(s_notifyKey) == 64;
  o["calendarView"] = s_calView;
  o["calendarRefresh"] = s_calRefresh;
  JsonArray calendars = o["calendars"].to<JsonArray>();
  for (const auto& c : s_cal) {
    JsonObject v = calendars.add<JsonObject>();
    v["name"] = c.name;
    v["enabled"] = c.enabled;
    v["color"] = c.color;
    v["hasUrl"] = c.url[0] != '\0';  // Never expose secret feed URLs in GET or backups.
  }
  JsonObject scr = o["screens"].to<JsonObject>();
  scr["clock"]    = Settings_ScreenEnabled(SCREEN_CLOCK_I);
  scr["planes"]   = Settings_ScreenEnabled(SCREEN_PLANES_I);
  scr["meteo"]    = Settings_ScreenEnabled(SCREEN_METEO_I);
  scr["forecast"] = Settings_ScreenEnabled(SCREEN_FORECAST_I);
  scr["calendar"] = Settings_ScreenEnabled(SCREEN_CALENDAR_I);
}

bool Settings_FromJson(JsonObjectConst in) {
  if (Settings_ValidateCalendarJson(in) || Settings_ValidateNotifications(in)) return false;
  bool changed = false;
  JsonObjectConst notification = in["notifications"];
  if (!notification.isNull()) {
    if (!notification["enabled"].isNull()) {
      s_notify = notification["enabled"].as<bool>();
      putBool("ntOn", s_notify);
    }
    if (!notification["seconds"].isNull()) {
      s_notifySeconds = notification["seconds"].as<uint16_t>();
      putU16("ntSec", s_notifySeconds);
    }
    const char* key = notification["key"] | "";
    if (*key) {
      snprintf(s_notifyKey, sizeof(s_notifyKey), "%s", key);
      putStr("ntKey", s_notifyKey);
    }
    changed = true;
  }
  auto setIf = [&](const char* key, auto fn) {
    JsonVariantConst v = in[key];
    if (!v.isNull()) { fn(v); changed = true; }
  };

  // Location only counts when both halves are there and sane - a half-applied
  // position would put the radar in the Gulf of Guinea.
  if (!in["lat"].isNull() && !in["lon"].isNull()) {
    double la = in["lat"].as<double>(), lo = in["lon"].as<double>();
    if (la >= -90 && la <= 90 && lo >= -180 && lo <= 180 && (la != 0 || lo != 0)) {
      Settings_SetLocation(la, lo);
      changed = true;
    }
  }
  setIf("lang",         [](JsonVariantConst v){ Settings_SetLanguage(v.as<uint8_t>()); });
  setIf("metric",       [](JsonVariantConst v){ Settings_SetMetricUnits(v.as<bool>()); });
  setIf("briDay",       [](JsonVariantConst v){ Settings_SetBrightDay(v.as<uint8_t>()); });
  setIf("briNight",     [](JsonVariantConst v){ Settings_SetBrightNight(v.as<uint8_t>()); });
  setIf("nightAuto",    [](JsonVariantConst v){ Settings_SetNightAuto(v.as<bool>()); });
  setIf("nightOffset",  [](JsonVariantConst v){ Settings_SetNightOffsetMin(v.as<int8_t>()); });
  setIf("radarSrc",     [](JsonVariantConst v){ Settings_SetRadarSource(v.as<uint8_t>()); });
  setIf("meteoLegend",  [](JsonVariantConst v){ Settings_SetMeteoLegend(v.as<bool>()); });
  setIf("autoRotate",   [](JsonVariantConst v){ Settings_SetAutoRotateSec(v.as<uint16_t>()); });
  setIf("topBearing",   [](JsonVariantConst v){ Settings_SetTopBearing(v.as<uint16_t>()); });
  setIf("secStyle",     [](JsonVariantConst v){ Settings_SetSecondsStyle(v.as<uint8_t>()); });
  setIf("clockColor",   [](JsonVariantConst v){ Settings_SetClockColor(v.as<uint16_t>()); });
  setIf("secColor",     [](JsonVariantConst v){ Settings_SetSecondsColor(v.as<uint16_t>()); });
  setIf("onlyCallsign", [](JsonVariantConst v){ Settings_SetOnlyWithCallsign(v.as<bool>()); });
  setIf("squawkAlert",  [](JsonVariantConst v){ Settings_SetSquawkAlert(v.as<bool>()); });
  setIf("watch",        [](JsonVariantConst v){ Settings_SetWatchCallsign(v.as<const char*>()); });

  if (!in["altMin"].isNull() || !in["altMax"].isNull()) {
    uint16_t lo = in["altMin"].isNull() ? s_altMin : in["altMin"].as<uint16_t>();
    uint16_t hi = in["altMax"].isNull() ? s_altMax : in["altMax"].as<uint16_t>();
    Settings_SetAltRangeFt(lo, hi);
    changed = true;
  }

  if (!in["calendarView"].isNull()) {
    Settings_SetCalendarView(in["calendarView"].as<uint8_t>());
    changed = true;
  }
  if (!in["calendarRefresh"].isNull()) {
    s_calRefresh = in["calendarRefresh"].as<uint16_t>();
    putU16("calPoll", s_calRefresh);
    changed = true;
  }
  if (in["calendars"].is<JsonArrayConst>()) {
    int i = 0;
    bool sourceChanged = false;
    for (JsonObjectConst v : in["calendars"].as<JsonArrayConst>()) {
      auto& c = s_cal[i];
      const CalendarSourceSettings previous = c;
      if (!v["name"].isNull()) snprintf(c.name, sizeof(c.name), "%s", v["name"].as<const char*>());
      if (!v["enabled"].isNull()) c.enabled = v["enabled"].as<bool>();
      if (!v["color"].isNull()) c.color = v["color"].as<uint16_t>();
      if (v["clearUrl"] == true) c.url[0] = 0;
      const char* url = v["url"] | "";
      if (*url) {
        if (!strncmp(url, "webcal://", 9))
          snprintf(c.url, sizeof(c.url), "https://%s", url + 9);
        else
          snprintf(c.url, sizeof(c.url), "%s", url);
      }
      char key[16];
      snprintf(key, sizeof(key), "cal%dName", i);
      putStr(key, c.name);
      snprintf(key, sizeof(key), "cal%dUrl", i);
      putStr(key, c.url);
      snprintf(key, sizeof(key), "cal%dColor", i);
      putU16(key, c.color);
      snprintf(key, sizeof(key), "cal%dOn", i);
      putBool(key, c.enabled);
      if (memcmp(&previous, &c, sizeof(c))) sourceChanged = true;
      ++i;
    }
    if (sourceChanged) ++s_calRevision;
    changed = true;
  }
  JsonObjectConst scr = in["screens"];
  if (!scr.isNull()) {
    struct { const char* key; uint8_t idx; } M[] = {
      { "clock",    SCREEN_CLOCK_I },
      { "planes",   SCREEN_PLANES_I },
      { "meteo",    SCREEN_METEO_I },
      { "forecast", SCREEN_FORECAST_I },
      { "calendar", SCREEN_CALENDAR_I },
    };
    for (auto& m : M) {
      JsonVariantConst v = scr[m.key];
      if (!v.isNull()) { Settings_SetScreenEnabled(m.idx, v.as<bool>()); changed = true; }
    }
  }
  return changed;
}

// --- Debounced flush --------------------------------------------------------
void Settings_Tick() {
  if (!s_uiDirty) return;
  if (millis() - s_uiDirtyAt < 2000) return;
  if (prefs.begin(NS, false)) {
    prefs.putUChar("rngP", s_rngP);
    prefs.putUChar("rngM", s_rngM);
    prefs.putUChar("scr",  s_scr);
    prefs.putUShort("topb", s_top);
    prefs.putUChar("bl",   s_briDay);
    prefs.putUChar("blN",  s_briNight);
    prefs.end();
  }
  s_uiDirty = false;
}

void Settings_ClearAll() {
  if (prefs.begin(NS, false)) { prefs.clear(); prefs.end(); }
  s_lat = DEFAULT_LAT; s_lon = DEFAULT_LON; s_hasLoc = false;
  s_briDay = 80; s_briNight = 25; s_nightAuto = true; s_nightOff = 0; s_isNight = false;
  s_notify = false;
  s_notifySeconds = 15;
  s_notifyKey[0] = 0;
  calendarDefaults();
  s_calView = 0;
  s_calRefresh = 15;
  ++s_calRevision;
  s_metric = false; s_lang = LANG_CZ; Lang_Set(s_lang);
  s_scrMask = (1 << SCREEN_CLOCK_I) | (1 << SCREEN_PLANES_I) |
              (1 << SCREEN_METEO_I) | (1 << SCREEN_FORECAST_I);
  s_autoRot = 0; s_radarSrc = RADAR_SRC_CHMU; s_mtLegend = true;
  s_secStyle = SEC_STYLE_DOTS; s_clockCol = 0xFFFF; s_secCol = 0x05FF;
  s_altMin = 0; s_altMax = 60000; s_onlyCs = false; s_sqAlert = true; s_watch[0] = '\0';
  s_rngP = 1; s_rngM = 1; s_scr = SCREEN_PLANES_I; s_top = 0;
  s_pw[0] = '\0';
  s_ssid[0] = '\0'; s_wpass[0] = '\0';
  s_uiDirty = false;
}

const CalendarSourceSettings& Settings_CalendarSource(uint8_t i) {
  return s_cal[i < CALENDAR_SOURCES ? i : 0];
}
uint8_t Settings_CalendarCount() {
  uint8_t n = 0;
  for (const auto& c : s_cal)
    if (c.enabled && *c.url) n++;
  return n;
}
uint8_t Settings_CalendarView() { return s_calView; }
void Settings_SetCalendarView(uint8_t v) {
  s_calView = v == 1 ? 1 : 0;
  putU8("calView", s_calView);
}
uint16_t Settings_CalendarRefresh() { return s_calRefresh; }
uint32_t Settings_CalendarRevision() { return s_calRevision; }
const char* Settings_ValidateCalendarJson(JsonObjectConst in) {
  if (!in["calendarView"].isNull() &&
      (!in["calendarView"].is<unsigned>() || in["calendarView"].as<unsigned>() > 1))
    return "calendarView";
  if (!in["calendarRefresh"].isNull() &&
      (!in["calendarRefresh"].is<unsigned>() || in["calendarRefresh"].as<unsigned>() < 5 ||
       in["calendarRefresh"].as<unsigned>() > 1440))
    return "calendarRefresh";
  if (in["calendars"].isNull()) return nullptr;
  if (!in["calendars"].is<JsonArrayConst>() || in["calendars"].size() > CALENDAR_SOURCES)
    return "calendars";
  for (JsonVariantConst item : in["calendars"].as<JsonArrayConst>()) {
    if (!item.is<JsonObjectConst>()) return "calendar";
    JsonObjectConst v = item.as<JsonObjectConst>();
    if (!v["name"].isNull() && (!v["name"].is<const char*>() ||
                                strlen(v["name"].as<const char*>()) >= sizeof(s_cal[0].name)))
      return "calendarName";
    if (!v["color"].isNull() && (!v["color"].is<unsigned>() || v["color"].as<unsigned>() > 65535))
      return "calendarColor";
    if (!v["enabled"].isNull() && !v["enabled"].is<bool>()) return "calendarEnabled";
    if (!v["clearUrl"].isNull() && !v["clearUrl"].is<bool>()) return "calendarClear";
    if (!v["url"].isNull()) {
      if (!v["url"].is<const char*>()) return "calendarUrl";
      const char* url = v["url"].as<const char*>();
      if (strlen(url) >= CALENDAR_URL_MAX) return "calendarUrl";
      if (*url) {
        const char* host = !strncmp(url, "https://", 8)    ? url + 8
                           : !strncmp(url, "webcal://", 9) ? url + 9
                                                           : nullptr;
        if (!host || !*host || *host == '/' || *host == '?' || *host == '#') return "calendarUrl";
        for (const char* c = url; *c; c++)
          if ((unsigned char)*c <= 32 || *c == '\\') return "calendarUrl";
        for (const char* c = host; *c && *c != '/'; c++)
          if (*c == '@') return "calendarUrl";
      }
    }
  }
  return nullptr;
}

bool Settings_NotificationsEnabled() { return s_notify; }
uint16_t Settings_NotificationSeconds() { return s_notifySeconds; }
const char* Settings_NotificationKey() { return s_notifyKey; }
const char* Settings_ValidateNotifications(JsonObjectConst in) {
  if (in["notifications"].isNull()) return nullptr;
  if (!in["notifications"].is<JsonObjectConst>()) return "notifications";
  JsonObjectConst n = in["notifications"];
  if (!n["enabled"].isNull() && !n["enabled"].is<bool>()) return "notificationEnabled";
  if (!n["seconds"].isNull()) {
    if (!n["seconds"].is<unsigned>()) return "notificationSeconds";
    unsigned seconds = n["seconds"].as<unsigned>();
    if (seconds && (seconds < 5 || seconds > 120)) return "notificationSeconds";
  }
  if (!n["key"].isNull()) {
    if (!n["key"].is<const char*>()) return "notificationKey";
    const char* key = n["key"];
    if (*key) {
      if (strlen(key) != 64) return "notificationKey";
      for (const char* c = key; *c; ++c)
        if (!isxdigit((unsigned char)*c)) return "notificationKey";
    }
  }
  return nullptr;
}
