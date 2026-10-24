// =============================================================================
//  MeteoPlaneRadar
//  Interface languages: Czech, English and Slovak.
//
//  Czech and Slovak need separate display and web spellings. The GFX font is 7-bit
//  ASCII, so anything drawn on the panel has to be written without diacritics
//  ("Predpoved"); a browser has no such problem and gets the real thing
//  ("Predpoved" with the accents). Keeping both in one table means a string can
//  never be updated in one place and forgotten in the other.
//
//  English needs only one spelling, so it is stored once and used for both.
//
//  The web PAGE does its own translation in JavaScript - it ships all three
//  languages and picks one from the config. Only the strings that C code has to
//  produce (captive portal labels, JSON status text) live here.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

#define LANG_CZ 0
#define LANG_EN 1
#define LANG_SK 2

// X(id, Czech display, Czech web, English, Slovak display, Slovak web)
//
// One list, five translations - the enum and every table below are generated from
// it, so they cannot drift apart.
#define LANG_STRINGS(X) \
  X(S_WIFI_WAIT,     "Cekam na WiFi",      "Čekám na WiFi",      "Waiting for WiFi", "Cakam na WiFi", "Čakám na WiFi") \
  X(S_DOWNLOADING,   "Stahuji...",         "Stahuji...",         "Downloading...", "Stahujem...", "Sťahujem...") \
  X(S_LOADING,       "Nacitam...",         "Načítám...",         "Loading...", "Nacitavam...", "Načítavam...") \
  X(S_ERROR,         "Chyba",              "Chyba",              "Error", "Chyba", "Chyba") \
  X(S_OK,            "OK",                 "OK",                 "OK", "OK", "OK") \
  X(S_NO_LOCATION,   "Nastav polohu",      "Nastavte polohu",    "Set your location", "Nastavte polohu", "Nastavte polohu") \
  X(S_KM,            "km",                 "km",                 "km", "km", "km") \
  X(S_SETTINGS,      "Nastaveni",          "Nastavení",          "Settings", "Nastavenia", "Nastavenia") \
  X(S_BRIGHTNESS,    "Jas",                "Jas",                "Brightness", "Jas", "Jas") \
  X(S_NOT_CONNECTED, "nepripojeno",        "nepřipojeno",        "not connected", "nepripojene", "nepripojené") \
  X(S_LOCATION,      "Poloha:",            "Poloha:",            "Location:", "Poloha:", "Poloha:") \
  X(S_TOP,           "Nahore",             "Nahoře",             "Top", "Hore", "Hore") \
  X(S_UNITS_AVIA,    "Jednotky: letecke",  "Jednotky: letecké",  "Units: aviation", "Jednotky: letecke", "Jednotky: letecké") \
  X(S_UNITS_METRIC,  "Jednotky: metricke", "Jednotky: metrické", "Units: metric", "Jednotky: metricke", "Jednotky: metrické") \
  X(S_WIFI_LOC,      "WiFi / poloha",      "WiFi / poloha",      "WiFi / location", "WiFi / poloha", "WiFi / poloha") \
  X(S_FW_UPDATE,     "Aktualizace FW",     "Aktualizace FW",     "Firmware update", "Aktualizacia FW", "Aktualizácia FW") \
  X(S_WEB_HINT,      "Nastaveni v prohlizeci:", "Nastavení v prohlížeči:", "Settings in a browser:", "Nastavenia v prehliadaci:", "Nastavenia v prehliadači:") \
  X(S_AIRCRAFT,      "Letadel",            "Letadel",            "Aircraft", "Lietadiel", "Lietadiel") \
  X(S_ALTITUDE,      "Vyska",              "Výška",              "Altitude", "Vyska", "Výška") \
  X(S_SPEED,         "Rychlost",           "Rychlost",           "Speed", "Rychlost", "Rýchlosť") \
  X(S_TRACK,         "Kurz",               "Kurz",               "Track", "Kurz", "Kurz") \
  X(S_CLIMB,         "Stoupani",           "Stoupání",           "Climb", "Stupanie", "Stúpanie") \
  X(S_TYPE,          "Typ",                "Typ",                "Type", "Typ", "Typ") \
  X(S_FROM,          "Z",                  "Z",                  "From", "Z", "Z") \
  X(S_TO,            "Do",                 "Do",                 "To", "Do", "Do") \
  X(S_ROUTE_WAIT,    "zjistuji trasu",     "zjišťuji trasu",     "looking up route", "zistujem trasu", "zisťujem trasu") \
  X(S_SIGNAL_LOST,   "signal ztracen",     "signál ztracen",     "signal lost", "signal strateny", "signál stratený") \
  X(S_UNKNOWN,       "neznamy",            "neznámý",            "unknown", "neznamy", "neznámy") \
  X(S_EMERGENCY,     "NOUZE",              "NOUZE",              "EMERGENCY", "NUDZA", "NÚDZA") \
  X(S_HIJACK,        "UNOS",               "ÚNOS",               "HIJACK", "UNOS", "ÚNOS") \
  X(S_RADIO_FAIL,    "BEZ RADIA",          "BEZ RÁDIA",          "RADIO FAIL", "BEZ RADIA", "BEZ RÁDIA") \
  X(S_METEORADAR,    "Meteoradar",         "Meteoradar",         "Weather radar", "Meteoradar", "Meteoradar") \
  X(S_NOW,           "nyni",               "nyní",               "now", "teraz", "teraz") \
  X(S_MIN,           "min",                "min",                "min", "min", "min") \
  X(S_WHOLE_CZ,      "cela CR",            "celá ČR",            "whole CZ", "cela CR", "celá ČR") \
  X(S_LOADING_NEWER, "nacitam novejsi snimky...", "načítám novější snímky...", "loading newer frames...", "nacitavam novsie snimky...", "načítavam novšie snímky...") \
  X(S_OLD_DATA,      "bez spojeni, zobrazena starsi data", "bez spojení, zobrazena starší data", "no link, showing older data", "bez spojenia, zobrazene starsie data", "bez spojenia, zobrazené staršie dáta") \
  X(S_FRAME_WIDE,    "snimek moc siroky",  "snímek moc široký",  "frame too wide", "snimka prilis siroka", "snímka príliš široká") \
  X(S_FORECAST,      "Predpoved",          "Předpověď",          "Forecast", "Predpoved", "Predpoveď") \
  X(S_AIR,           "Ovzdusi",            "Ovzduší",            "Air quality", "Ovzdusie", "Ovzdušie") \
  X(S_POLLEN,        "Pyl",                "Pyl",                "Pollen", "Pel", "Peľ") \
  X(S_TODAY,         "dnes",               "dnes",               "today", "dnes", "dnes") \
  X(S_LAT_LABEL,     "Zemepisna sirka",    "Zeměpisná šířka",    "Latitude", "Zemepisna sirka", "Zemepisná šírka") \
  X(S_LON_LABEL,     "Zemepisna delka",    "Zeměpisná délka",    "Longitude", "Zemepisna dlzka", "Zemepisná dĺžka") \
  X(S_ALDER, "olse", "olše", "alder", "jelsa", "jelša") \
  X(S_BIRCH, "briza", "bříza", "birch", "breza", "breza") \
  X(S_GRASS, "travy", "trávy", "grass", "travy", "trávy") \
  X(S_TOMORROW,      "zitra",              "zítra",              "tomorrow", "zajtra", "zajtra")

enum StrId : uint16_t {
#define X(id, cz, czw, en, sk, skw) id,
  LANG_STRINGS(X)
#undef X
  STR_COUNT
};

void    Lang_Set(uint8_t lang);     // LANG_CZ / LANG_EN / LANG_SK; anything else = CZ
uint8_t Lang_Get();

// For the PANEL - ASCII only, safe with the built-in font.
const char* T(StrId id);

// For a browser / captive portal - real UTF-8 with diacritics.
const char* TW(StrId id);

// Calendar names in the active language. Both are ASCII-only: they are drawn on
// the clock and forecast screens, never sent to a browser.
// wday 0 = Sunday (matches struct tm), mon 0 = January.
const char* Lang_WeekdayShort(int wday);
const char* Lang_MonthName(int mon);
