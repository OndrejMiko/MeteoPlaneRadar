#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "UI.h"
#include "Layout.h"
#include "ScreenChrome.h"
#include "Overlay.h"
#include "ScreenPlanes.h"
#include "ScreenClock.h"
#include "ClaudeUsage.h"
#include "ScreenForecast.h"
#include "ScreenSettings.h"
#include "Settings.h"
#include "Lang.h"
#include "ADSB.h"
#include "Route.h"
#include "Forecast.h"
#include "Navigation.h"
#include "EuBorder.h"

Arduino_GFX display;
Arduino_GFX* gfx = &display;
static time_t now = 1793275200;
extern "C" time_t time(time_t* out) {
  if (out) *out = now;
  return now;
}
void Outside_StatusText(char* out, size_t n) { snprintf(out, n, "15:00  -12 C"); }
bool Outside_TimeValid() { return true; }
bool UI_StatusLineChanged() { return false; }
void UI_DrawStatusLine(int y) { Chrome_DrawStatus(y); }
void UI_TextCenteredIn(const char* s, int x, int w, int y, uint16_t c, uint8_t size) {
  gfx->setTextSize(size);
  gfx->setTextColor(c);
  gfx->setCursor(x + (w - (int)strlen(s) * 6 * size) / 2, y);
  gfx->print(s);
}
void UI_TextCentered(const char* s, int y, uint16_t c, uint8_t size) {
  UI_TextCenteredIn(s, 0, 480, y, c, size);
}
void Set_Backlight(uint8_t) {}
void NightMode_Apply() {}
void NightMode_Toggle() {}
bool WiFi_IsConnected() { return true; }
bool WiFi_IsAP() { return false; }
String WiFi_SSID() { return "A_very_long_home_network_name_32"; }
String WiFi_IP() { return "192.168.1.42"; }
static Aircraft planes[5];
bool ADSB_Fetch(double, double, float) { return true; }
int ADSB_Count() { return 5; }
const Aircraft* ADSB_List() { return planes; }
int ADSB_FindByHex(const char* hex) {
  for (int i = 0; i < 5; ++i)
    if (*hex && !strcmp(hex, planes[i].hex)) return i;
  return -1;
}
const char* ADSB_EmergencyCode(const Aircraft&) { return nullptr; }
void Route_Clear() {}
bool Route_TakeChanged() { return false; }
void Route_Select(const char*, float, float) {}
static RouteInfo route;
static bool routeReady = false;
RouteState Route_GetState() { return routeReady ? ROUTE_OK : ROUTE_NONE; }
const RouteInfo* Route_Get() { return routeReady ? &route : nullptr; }
void EuBorder_Draw(ProjectFn, uint16_t, float, float, float, float) {
  for (int y = 20; y < 470; y += 18) gfx->drawLine(0, y, 480, y - 14, C_DKGRAY);
}
void EuBorder_DrawCities(ProjectFn, int, int, int, uint16_t, uint16_t, bool, uint8_t, float, float,
                         float, float) {}
static FcHour hours[12];
static FcDay days[7];
bool Forecast_Valid() { return true; }
int Forecast_HourCount() { return 12; }
const FcHour* Forecast_Hours() { return hours; }
int Forecast_DayCount() { return 7; }
const FcDay* Forecast_Days() { return days; }
bool Forecast_CurrentValid() { return true; }
float Forecast_CurrentTemp() { return -12; }
float Forecast_CurrentPrecip() { return 1.2; }
float Forecast_CurrentWind() { return 24; }
int Forecast_CurrentCode() { return 61; }
bool AirQuality_Valid() { return true; }
int AirQuality_Aqi() { return 34; }
float AirQuality_Pm25() { return 12; }
float AirQuality_PollenMax() { return 0; }
const char* AirQuality_PollenWorst() { return ""; }
static void snapshot(const char* name) {
  if (const char* dir = getenv("SCREEN_LAYOUT_SCREENSHOTS"))
    display.snapshot((std::string(dir) + "/" + name + ".svg").c_str());
}
int main() {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  Settings_ClearAll();
  Settings_Begin();
  Settings_SetLocation(48.15, 17.11);
  Settings_SetLanguage(LANG_SK);
  // Layout reservations protect complete icon bounds, not only text baselines.
  Layout_Begin();
  Chrome_ReserveMap(false);
  assert(!Layout_Claim(200, 30, 80, 16));
  assert(!Layout_Claim(200, 444, 80, 16));
  assert(Layout_Claim(200, 160, 80, 16));
  assert(Chrome_MapOverlaps(220, 110, 44, 44));
  assert(!Chrome_MapOverlaps(220, 150, 44, 44));
  assert(Chrome_MapOverlaps(220, 420, 44, 44));
  assert(LY_STATUS + 16 + 20 <= LY_SUB);
  assert(LY_RANGE > 439);               // count badge ends at y=439
  assert(72 + 7 - 3 > LY_STATUS + 30);  // forecast starts below the divider
  // Draw through a yellow target, then verify opaque backing precedes the label.
  display.fillScreen(C_YELLOW);
  Chrome_DrawRange("100 km");
  auto svg = display.drawing.str();
  assert(svg.find("x='168' y='432' width='144' height='48'") < svg.find(">100 km</text>"));
  assert(Layout_TextW("100 km", 2) + 32 < 2 * Layout_ChordHalf(LY_RANGE + 16));
  // Source positions are deliberately placed behind the clock, inside its gap,
  // in free map space and behind the range. Hidden targets must not accept taps.
  const int xs[5] = {240, 240, 240, 160, 240}, ys[5] = {40, 132, 245, 290, 444};
  for (int i = 0; i < 5; ++i) {
    planes[i].lat = 48.15 + (240 - ys[i]) / 230.0 * 25.0 / 111.32;
    planes[i].lon = 17.11 + (xs[i] - 240) / 230.0 * 25.0 / (111.32 * cos(48.15 * 0.0174532925));
    planes[i].altFt = 25000;
    planes[i].hasTrack = true;
    planes[i].track = 30 + i * 35;
    snprintf(planes[i].hex, 8, "test%u", i);
    snprintf(planes[i].callsign, 10, "TEST%u", i);
  }
  Navigation_ScreenChanged();
  ScreenPlanes_Enter();
  ScreenPlanes_Tick();
  ScreenPlanes_Draw();
  snapshot("aircraft");
  assert(!ScreenPlanes_HandleTap(240, 40));
  assert(!ScreenPlanes_HandleTap(240, 132));
  assert(!ScreenPlanes_HandleTap(240, 444));
  assert(ScreenPlanes_HandleTap(240, 245));
  routeReady = true;
  snprintf(route.from, sizeof(route.from), "Frankfurt am Main");
  snprintf(route.to, sizeof(route.to), "Bratislava");
  snprintf(planes[2].type, sizeof(planes[2].type), "A320");
  snprintf(planes[2].reg, sizeof(planes[2].reg), "OM-TEST");
  ScreenPlanes_Draw();
  snapshot("aircraft-detail");
  svg = display.drawing.str();
  assert(svg.find("x='55' y='100' width='370' height='300' rx='20' fill='" +
                  Arduino_GFX::rgb(OVERLAY_PANEL)) != std::string::npos);
  assert(svg.find("M372 120L403 151") != std::string::npos); // shared 32px X
  assert(svg.find("x='188' y='98' width='26' height='6'") == std::string::npos);
  assert(svg.find("cx='376' cy='132' r='15'") == std::string::npos); // no red button
  assert(ScreenPlanes_HandleTap(200, 220) && ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(10, 10) && ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(355, 105) && ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(356, 104) && ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(420, 105) && ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(356, 169) && ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(419, 168) && !ScreenPlanes_DetailOpen());
  assert(ScreenPlanes_HandleTap(240, 245));
  assert(ScreenPlanes_HandleTap(356, 105) && !ScreenPlanes_DetailOpen());
  // Data, type/registration and both route rows stay inside the taller panel
  // in every language and both unit systems.
  for (uint8_t lang = 0; lang < 3; ++lang) {
    Settings_SetLanguage(lang);
    for (bool metric : {false, true}) {
      JsonDocument config; config["metric"] = metric;
      assert(Settings_FromJson(config.as<JsonObjectConst>()));
      assert(ScreenPlanes_HandleTap(240, 245));
      ScreenPlanes_Draw();
      snapshot((std::string("aircraft-detail-") + std::to_string(lang) +
                (metric ? "-metric" : "-aviation")).c_str());
      assert(display.drawing.str().find("Bratislava</text>") != std::string::npos);
      ScreenPlanes_CloseDetail();
    }
  }
  Settings_SetLanguage(LANG_SK);

  for (int i = 0; i < 12; ++i) {
    hours[i].t = now + i * 3600;
    hours[i].temp = 8 + i * .5;
    hours[i].code = 61;
    hours[i].precip = .6;
    hours[i].wind = 18;
  }
  for (int i = 0; i < 7; ++i) {
    days[i].t = now + i * 86400;
    days[i].tmax = 13 + i;
    days[i].tmin = 4 + i;
    days[i].code = i % 2 ? 3 : 61;
    days[i].wind = 18;
    days[i].precip = .6;
  }
  ScreenForecast_Draw();
  snapshot("forecast");
  ScreenClock_Draw();
  snapshot("clock");
  JsonDocument claude;
  claude["observedAt"] = (int64_t)now;
  claude["five_hour"]["used_percentage"] = 68.5;
  claude["five_hour"]["resets_at"] = (int64_t)now + 3600;
  claude["seven_day"]["used_percentage"] = 42;
  claude["seven_day"]["resets_at"] = (int64_t)now + 3 * 86400;
  assert(ClaudeUsage_Accept(claude.as<JsonObjectConst>(), now));
  Settings_SetClaudeUsage(true);
  assert(ScreenClock_Tick()); // update works even when the seconds ring is off
  ScreenClock_Draw(); snapshot("clock-claude");
  svg = display.drawing.str();
  assert(svg.find("31%") != std::string::npos && svg.find("58%") != std::string::npos);
  now += 901; ScreenClock_Draw(); snapshot("clock-claude-stale");
  assert(display.drawing.str().find(" ~") != std::string::npos);
  now += 3600; ScreenClock_Draw(); snapshot("clock-claude-reset");
  assert(display.drawing.str().find("31%") == std::string::npos);
  Settings_SetClaudeUsage(false); ScreenClock_Draw();
  assert(display.drawing.str().find("Claude") == std::string::npos);
  for (uint8_t lang = 0; lang < 3; ++lang) {
    Settings_SetLanguage(lang);
    ScreenSettings_Draw();
    snapshot((std::string("settings-") + std::to_string(lang)).c_str());
  }
  // The longest English brightness label and its mode suffix have a 12px gap.
  Settings_SetLanguage(LANG_EN);
  assert(Layout_TextW(T(S_BRIGHTNESS), 2) + 12 + 7 * 6 < 250);
  // Real shared weather overlays on a deliberately bright radar background.
  display.fillScreen(C_CYAN);
  Chrome_DrawPrecipLegend(false);
  Chrome_DrawWeatherTimeline(2,6,"-15 min  12:30","Starsie data");
  Chrome_DrawStatus(LY_STATUS);
  Chrome_DrawRange("200 km");
  snapshot("weather-chrome");
  svg=display.drawing.str();
  assert(svg.find("x='80' y='-24' width='320'")==std::string::npos);
  assert(svg.find("x='156' y='414' width='168' height='3'")!=std::string::npos);
  assert(svg.find("x='30' y='142' width='108' height='160'")!=std::string::npos);
  assert(svg.find("cy='86'")==std::string::npos); // playback has no dot markers
  Layout_Begin(); Chrome_ReserveMap(true);
  assert(Layout_Claim(140,90,200,22)); // map is free under the compact clock
  assert(!Layout_Claim(180,395,120,24)); // timeline keeps city names out
  for(int frame : {0,5}) {
    display.fillScreen(C_CYAN);
    Chrome_DrawWeatherTimeline(frame,6,"12:30",nullptr);
    auto track=display.drawing.str();
    assert(track.find(frame ? "x='316' y='409'" : "x='156' y='409'")!=std::string::npos);
  }
  std::cout << "Screen chrome, icon exclusion/touch, round-panel fit and screen rendering passed\n";
}
