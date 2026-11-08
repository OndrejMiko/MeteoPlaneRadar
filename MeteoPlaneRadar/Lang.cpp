// =============================================================================
//  MeteoPlaneRadar
//  Interface language - the string tables.
//
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
// =============================================================================
#include "Lang.h"
#include "AAFont.h"   // AAText_Enabled - the smooth faces carry the accents

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

// 0.8.5: with a smooth typeface selected the display gets the accented
// strings too - the fonts have the Czech letters, the built-in one never did.
const char* T(StrId id) {
  if (id >= STR_COUNT) return "";
  return AAText_Enabled() ? TW(id) : (s_lang == LANG_EN) ? EN_ALL[id] : (s_lang == LANG_SK) ? SK_DISP[id] : CZ_DISP[id];
}

const char* TW(StrId id) {
  if (id >= STR_COUNT) return "";
  return (s_lang == LANG_EN) ? EN_ALL[id] : (s_lang == LANG_SK) ? SK_WEB[id] : CZ_WEB[id];
}

// Sunday first, to line up with struct tm's tm_wday.
static const char* const WD_CZ[7] = { "Ne", "Po", "Ut", "St", "Ct", "Pa", "So" };
static const char* const WD_CZ_U[7] = { "Ne", "Po", "Út", "St", "Čt", "Pá", "So" };
static const char* const WD_EN[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static const char* const MON_CZ[12] = { "ledna", "unora", "brezna", "dubna", "kvetna",
                                        "cervna", "cervence", "srpna", "zari",
                                        "rijna", "listopadu", "prosince" };
static const char* const MON_CZ_U[12] = { "ledna", "února", "března", "dubna", "května",
                                          "června", "července", "srpna", "září",
                                          "října", "listopadu", "prosince" };
static const char* const MON_EN[12] = { "January", "February", "March", "April", "May",
                                        "June", "July", "August", "September",
                                        "October", "November", "December" };

static const char* const WD_SK_U[7] = {"Ne","Po","Ut","St","Št","Pi","So"};
static const char* const MON_SK_U[12] = {"januára","februára","marca","apríla","mája","júna","júla","augusta","septembra","októbra","novembra","decembra"};
static const char* const WD_SK[7] = { "Ne", "Po", "Ut", "St", "St", "Pi", "So" };
static const char* const MON_SK[12] = { "januara", "februara", "marca", "aprila", "maja", "juna", "jula", "augusta", "septembra", "oktobra", "novembra", "decembra" };

const char* Lang_WeekdayShort(int wday) {
  if (wday < 0 || wday > 6) return "";
  return (s_lang == LANG_EN) ? WD_EN[wday] : (s_lang == LANG_SK) ? (AAText_Enabled()?WD_SK_U[wday]:WD_SK[wday]) : (AAText_Enabled()?WD_CZ_U[wday]:WD_CZ[wday]);
}

const char* Lang_MonthName(int mon) {
  if (mon < 0 || mon > 11) return "";
  return (s_lang == LANG_EN) ? MON_EN[mon] : (s_lang == LANG_SK) ? (AAText_Enabled()?MON_SK_U[mon]:MON_SK[mon]) : (AAText_Enabled()?MON_CZ_U[mon]:MON_CZ[mon]);
}

const char* Lang_MonthLabel(int mon) {
  static const char* CZ[]={"Leden","Unor","Brezen","Duben","Kveten","Cerven","Cervenec","Srpen","Zari","Rijen","Listopad","Prosinec"};
  static const char* SK[]={"Januar","Februar","Marec","April","Maj","Jun","Jul","August","September","Oktober","November","December"};
  if(mon<0||mon>11)return "";
  static const char* CZ_U[]={"Leden","Únor","Březen","Duben","Květen","Červen","Červenec","Srpen","Září","Říjen","Listopad","Prosinec"};
  static const char* SK_U[]={"Január","Február","Marec","Apríl","Máj","Jún","Júl","August","September","Október","November","December"};
  return s_lang==LANG_EN?MON_EN[mon]:s_lang==LANG_SK?(AAText_Enabled()?SK_U[mon]:SK[mon]):(AAText_Enabled()?CZ_U[mon]:CZ[mon]);
}
