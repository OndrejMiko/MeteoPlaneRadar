// =============================================================================
//  MeteoPlaneRadar
//  Interface language - the string tables.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Lang.h"

static uint8_t s_lang = LANG_CZ;

static const char* const CZ_DISP[STR_COUNT] = {
#define X(id, cz, czw, en, sk, skw) cz,
  LANG_STRINGS(X)
#undef X
};

static const char* const CZ_WEB[STR_COUNT] = {
#define X(id, cz, czw, en, sk, skw) czw,
  LANG_STRINGS(X)
#undef X
};

// English has no diacritics, so the panel and the browser share one table.
static const char* const EN_ALL[STR_COUNT] = {
#define X(id, cz, czw, en, sk, skw) en,
  LANG_STRINGS(X)
#undef X
};

static const char* const SK_DISP[STR_COUNT] = {
#define X(id, cz, czw, en, sk, skw) sk,
  LANG_STRINGS(X)
#undef X
};
static const char* const SK_WEB[STR_COUNT] = {
#define X(id, cz, czw, en, sk, skw) skw,
  LANG_STRINGS(X)
#undef X
};

void    Lang_Set(uint8_t lang) { s_lang = (lang <= LANG_SK) ? lang : LANG_CZ; }
uint8_t Lang_Get()             { return s_lang; }

const char* T(StrId id) {
  if (id >= STR_COUNT) return "";
  return (s_lang == LANG_EN) ? EN_ALL[id] : (s_lang == LANG_SK) ? SK_DISP[id] : CZ_DISP[id];
}

const char* TW(StrId id) {
  if (id >= STR_COUNT) return "";
  return (s_lang == LANG_EN) ? EN_ALL[id] : (s_lang == LANG_SK) ? SK_WEB[id] : CZ_WEB[id];
}

// Sunday first, to line up with struct tm's tm_wday.
static const char* const WD_CZ[7] = { "Ne", "Po", "Ut", "St", "Ct", "Pa", "So" };
static const char* const WD_EN[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static const char* const MON_CZ[12] = { "ledna", "unora", "brezna", "dubna", "kvetna",
                                        "cervna", "cervence", "srpna", "zari",
                                        "rijna", "listopadu", "prosince" };
static const char* const MON_EN[12] = { "January", "February", "March", "April", "May",
                                        "June", "July", "August", "September",
                                        "October", "November", "December" };

static const char* const WD_SK[7] = { "Ne", "Po", "Ut", "St", "St", "Pi", "So" };
static const char* const MON_SK[12] = { "januara", "februara", "marca", "aprila", "maja", "juna", "jula", "augusta", "septembra", "oktobra", "novembra", "decembra" };

const char* Lang_WeekdayShort(int wday) {
  if (wday < 0 || wday > 6) return "";
  return (s_lang == LANG_EN) ? WD_EN[wday] : (s_lang == LANG_SK) ? WD_SK[wday] : WD_CZ[wday];
}

const char* Lang_MonthName(int mon) {
  if (mon < 0 || mon > 11) return "";
  return (s_lang == LANG_EN) ? MON_EN[mon] : (s_lang == LANG_SK) ? MON_SK[mon] : MON_CZ[mon];
}

const char* Lang_MonthLabel(int mon) {
  static const char* CZ[]={"Leden","Unor","Brezen","Duben","Kveten","Cerven","Cervenec","Srpen","Zari","Rijen","Listopad","Prosinec"};
  static const char* SK[]={"Januar","Februar","Marec","April","Maj","Jun","Jul","August","September","Oktober","November","December"};
  if(mon<0||mon>11)return "";
  return s_lang==LANG_EN?MON_EN[mon]:s_lang==LANG_SK?SK[mon]:CZ[mon];
}
