#include "Notifications.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>

#include "CalendarIcs.h"
#include "Lang.h"
#include "Settings.h"
#include "UI.h"
#include "Overlay.h"
#include "Layout.h"

namespace {
constexpr unsigned CAPACITY = 6;
struct Message {
  char sender[64], room[64], text[1025];
};
struct Store {
  Message messages[CAPACITY];
  // Dedupe survives local dismissal. No message bodies retained in this history.
  uint64_t recent[32];
};
Store* store = nullptr;
unsigned count = 0, recentIndex = 0, detailPage = 0;
uint32_t dropped = 0, lastReceived = 0, openedAt = 0;
bool visible = false, expanded = false, dirty = false;
constexpr uint16_t PANEL = OVERLAY_PANEL;
struct BadgeBox {
  int x, y, w, h;
};
BadgeBox badgeBox() {
  switch (Settings_Screen()) {
    case SCREEN_PLANES_I:
      return {208, 391, 64, 48};
    case SCREEN_METEO_I:
      return {208, LY_PLAYBACK_NOTE - 55, 64, 40}; // clear of playback and loading note  // above the lower range notch
    case SCREEN_CALENDAR_I:
      return {366, 134, 48, 40};  // beside view tabs, clear of Today
    default:
      return {208, 438, 64, 40};  // clear of forecast AQI and settings buttons
  }
}

const char* label(const char* cz, const char* en, const char* sk) {
  return Lang_Get() == LANG_EN ? en : (Lang_Get() == LANG_SK ? sk : cz);
}
uint64_t hash(const char* s) {
  uint64_t h = 14695981039346656037ULL;
  while (*s) {
    h ^= (uint8_t)*s++;
    h *= 1099511628211ULL;
  }
  return h ? h : 1;
}
void text(const char* s, int x, int y, uint16_t color, unsigned size = 2) {
  gfx->setTextColor(color);
  gfx->setTextSize(size);
  gfx->setCursor(x, y);
  gfx->print(s);
}
void clipped(const char* s, int x, int y, unsigned chars, uint16_t color) {
  char b[65];
  size_t n = strlen(s);
  if (n > chars) n = chars;
  memcpy(b, s, n);
  b[n] = 0;
  if (strlen(s) > n && n >= 3) memcpy(b + n - 3, "...", 3);
  text(b, x, y, color);
}
// Monospaced ASCII after transliteration. Word wrap, honouring explicit newlines.
size_t line(const char* s, size_t at, char* out, unsigned width) {
  size_t start = at, end = at;
  while (s[end] && s[end] != '\n' && end - start < width) ++end;
  size_t next = end;
  if (s[end] && s[end] != '\n' && end - start == width) {
    size_t space = end;
    while (space > start && s[space] != ' ') --space;
    if (space > start) {
      end = space;
      next = space;
    }
  }
  memcpy(out, s + start, end - start);
  out[end - start] = 0;
  while (s[next] == ' ') ++next;
  if (s[next] == '\n') ++next;
  return next;
}
void show() {
  visible = true;
  expanded = false;
  detailPage = 0;
  openedAt = millis();
  dirty = true;
}
void dismiss() {
  if (count) {
    --count;
    memmove(store->messages, store->messages + 1, count * sizeof(Message));
    memset(&store->messages[count], 0, sizeof(Message));
  }
  visible = count != 0;
  expanded = false;
  detailPage = 0;
  openedAt = millis();
  dirty = true;
}
unsigned pageCount(const char* s) {
  unsigned lines = 0;
  size_t at = 0;
  char b[33];
  do {
    at = line(s, at, b, 28);
    ++lines;
  } while (s[at]);
  return (lines + 7) / 8;
}
}  // namespace

const char* Notifications_Validate(JsonObjectConst n) {
  if (n.isNull()) return "message";
  struct Field {
    const char* name;
    size_t max;
  };
  const Field fields[] = {{"id", 159}, {"sender", 127}, {"room", 127}, {"text", 1024}};
  for (const auto& f : fields) {
    if (!n[f.name].is<const char*>()) return f.name;
    const char* s = n[f.name];
    if (strlen(s) > f.max || (!*s && (!strcmp(f.name, "id") || !strcmp(f.name, "text"))))
      return f.name;
  }
  if (!n["service"].is<const char*>()) return "service";
  const char* service = n["service"];
  if (strcmp(service, "google-chat") && strcmp(service, "notification")) return "service";
  return nullptr;
}
bool Notifications_Authorized(const char* token) {
  const char* key = Settings_NotificationKey();
  if (!token || strlen(key) != 64 || strlen(token) != 64) return false;
  unsigned diff = 0;
  for (unsigned i = 0; i < 64; ++i) diff |= (unsigned char)(token[i] ^ key[i]);
  return diff == 0;
}
bool Notifications_Push(JsonObjectConst n) {
  if (!Settings_NotificationsEnabled() || Notifications_Validate(n)) return false;
  if (!store)
    store = (Store*)heap_caps_calloc(1, sizeof(Store), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!store) return false;
  uint64_t id = hash(n["id"]);
  for (auto seen : store->recent)
    if (seen == id) return true;
  // Backpressure: sender retries instead of silently losing old or new messages.
  if (count == CAPACITY) {
    ++dropped;
    return false;
  }
  Message& m = store->messages[count++];
  Calendar_Ascii(n["sender"], m.sender, sizeof(m.sender), false, false);
  Calendar_Ascii(n["room"], m.room, sizeof(m.room), false, false);
  Calendar_Ascii(n["text"], m.text, sizeof(m.text), false, true);
  store->recent[recentIndex++ % 32] = id;
  lastReceived = millis();
  if (!visible) show();
  dirty = true;
  return true;
}
bool Notifications_Tick() {
  if (!Settings_NotificationsEnabled()) {
    if (store) {
      free(store);
      store = nullptr;
      count = 0;
      recentIndex = 0;
      visible = expanded = false;
      dirty = true;
    }
  }
  unsigned seconds = Settings_NotificationSeconds();
  if (visible && !expanded && seconds && (uint32_t)(millis() - openedAt) >= seconds * 1000UL) {
    visible = false;
    dirty = true;  // retain messages behind badge
  }
  return dirty;
}
void Notifications_Draw() {
  dirty = false;
  if (!count || !Settings_NotificationsEnabled()) return;
  char b[64];
  if (!visible) {
    const auto box = badgeBox();
    gfx->fillRoundRect(box.x, box.y, box.w, box.h, 20, PANEL);
    snprintf(b, sizeof(b), "%u", count);
    UI_TextCenteredIn(b, box.x, box.w, box.y + (box.h - 16) / 2, C_WHITE, 2);
    return;
  }
  Message& m = store->messages[0];
  if (!expanded) {
    gfx->fillRoundRect(43, 115, 394, 197, 20, PANEL);
    gfx->fillCircle(66, 139, 4, C_GREEN);
    text(label("Notifikace", "Notifications", "Notifikacie"), 80, 132, C_GRAY);
    snprintf(b, sizeof(b), "%u/%u", 1, count);
    text(b, 294, 132, C_GRAY, 2);
    Overlay_DrawClose(368, 120);
    clipped(m.sender, 63, 165, 24, C_WHITE);
    size_t at = line(m.text, 0, b, 28);
    text(b, 63, 199, C_WHITE);
    line(m.text, at, b, 28);
    text(b, 63, 222, C_WHITE);
    text(label("Tukni pro detail", "Tap for details", "Tukni pre detail"), 63, 275, C_GRAY, 2);
  } else {
    gfx->fillScreen(C_BLACK);
    const char* title = label("Notifikace", "Notifications", "Notifikacie");
    const int titleX = (480 - (int)strlen(title) * 12) / 2;
    gfx->fillCircle(titleX - 14, 82, 4, C_GREEN);
    text(title, titleX, 75, C_GRAY, 2);
    Overlay_DrawClose(359, 95);
    clipped(m.sender, 85, 120, 22, C_WHITE);
    clipped(m.room, 85, 147, 25, C_GRAY);
    snprintf(b, sizeof(b), "%u / %u", 1, count);
    text(b, 85, 175, C_CYAN, 1);
    gfx->drawFastHLine(70, 195, 340, C_DKGRAY);
    size_t at = 0;
    for (unsigned i = 0; i < detailPage * 8 && m.text[at]; ++i) at = line(m.text, at, b, 28);
    for (unsigned i = 0; i < 8 && m.text[at]; ++i) {
      at = line(m.text, at, b, 28);
      text(b, 72, 210 + i * 19, C_WHITE);
    }
    unsigned pages = pageCount(m.text);
    snprintf(b, sizeof(b), "<  %u/%u  >", detailPage + 1, pages);
    UI_TextCentered(b, 370, C_GRAY, 2);
    gfx->fillRoundRect(108, 400, 264, 48, 16, C_DKGRAY);
    UI_TextCentered(label("Zavrit / dalsi", "Dismiss / next", "Zavriet / dalsia"), 416, C_WHITE, 2);
  }
}
bool Notifications_Visible() { return visible && Settings_NotificationsEnabled(); }
bool Notifications_Touch(int x, int y, bool tap) {
  if (!count || !Settings_NotificationsEnabled()) return false;
  if (!visible) {
    const auto box = badgeBox();
    if (tap && x >= box.x && x < box.x + box.w && y >= box.y && y < box.y + box.h) {
      show();
      return true;
    }
    return false;
  }
  if (!tap) return true;
  if (!expanded) {
    if (Overlay_CloseHit(x, y, 368, 120))
      dismiss();
    else if (x >= 43 && x < 437 && y >= 115 && y < 312) {
      expanded = true;
      detailPage = 0;
      dirty = true;
    }
  } else {
    if (Overlay_CloseHit(x, y, 359, 95) || (x >= 108 && x < 372 && y >= 400 && y < 448))
      dismiss();
    else if (y >= 360 && y < 400) {
      unsigned pages = pageCount(store->messages[0].text);
      if (x < 240 && detailPage) --detailPage;
      if (x >= 240 && detailPage + 1 < pages) ++detailPage;
      dirty = true;
    }
  }
  return true;
}
size_t Notifications_MemoryBytes() { return store ? sizeof(Store) : 0; }
unsigned Notifications_Count() { return count; }
uint32_t Notifications_Dropped() { return dropped; }
uint32_t Notifications_LastReceived() { return lastReceived; }
