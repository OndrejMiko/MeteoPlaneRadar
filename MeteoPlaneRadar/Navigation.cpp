#include "Navigation.h"

#include <Arduino.h>
#include <stdint.h>

#include "Layout.h"
#include "UI.h"

namespace {
constexpr uint32_t SCREEN_MS = 5000, RANGE_MS = 10000;
uint32_t screenAt = 0, rangeAt = 0;
bool screenVisible = false, rangeVisible = false, dirty = false;
int top(int count) { return LY_ZOOM_DOTS_Y - (count - 1) * LY_ZOOM_DOTS_GAP / 2; }
}  // namespace

void Navigation_ScreenChanged() {
  screenAt = rangeAt = (uint32_t)millis();
  screenVisible = rangeVisible = dirty = true;
}
void Navigation_RangeChanged() {
  rangeAt = (uint32_t)millis();
  rangeVisible = dirty = true;
}
bool Navigation_ScreenDotsVisible() { return screenVisible; }
bool Navigation_RangeDotsVisible() { return rangeVisible; }
bool Navigation_Tick() {
  uint32_t now = (uint32_t)millis();
  if (screenVisible && (uint32_t)(now - screenAt) >= SCREEN_MS) {
    screenVisible = false;
    dirty = true;
  }
  if (rangeVisible && (uint32_t)(now - rangeAt) >= RANGE_MS) {
    rangeVisible = false;
    dirty = true;
  }
  return dirty;
}
void Navigation_Drawn() { dirty = false; }
void Navigation_ReserveRangeDots(int count) {
  if (rangeVisible && count > 1)
    Layout_Reserve(LY_ZOOM_DOTS_X - 11, top(count) - 11, 22,
                   (count - 1) * LY_ZOOM_DOTS_GAP + 22);
}
void Navigation_DrawRangeDots(int selected, int count) {
  if (!rangeVisible || count <= 1 || selected < 0 || selected >= count) return;
  int y = top(count);
  gfx->fillRoundRect(LY_ZOOM_DOTS_X - 11, y - 11, 22,
                     (count - 1) * LY_ZOOM_DOTS_GAP + 22, 10, C_BLACK);
  for (int i = 0; i < count; ++i) {
    if (i == selected) gfx->fillCircle(LY_ZOOM_DOTS_X, y, 4, C_YELLOW);
    else
      gfx->drawCircle(LY_ZOOM_DOTS_X, y, 4, C_GRAY);
    y += LY_ZOOM_DOTS_GAP;
  }
}
