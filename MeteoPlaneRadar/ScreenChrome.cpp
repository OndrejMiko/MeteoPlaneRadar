#include "ScreenChrome.h"
#include <string.h>
#include "Layout.h"
#include "Outside.h"
#include "UI.h"

namespace {
constexpr int HEADER_X = 80, HEADER_W = 320;
constexpr int RANGE_X = 168, RANGE_Y = 432, RANGE_W = 144, RANGE_H = 48;
constexpr int CLEARANCE = 12;
int headerHeight(bool weather) { return weather ? 144 : 128; }
bool overlap(int x, int y, int w, int h, int bx, int by, int bw, int bh) {
  return x < bx + bw && x + w > bx && y < by + bh && y + h > by;
}
}  // namespace

void Chrome_ReserveMap(bool weather) {
  Layout_Reserve(HEADER_X - CLEARANCE, 0, HEADER_W + 2 * CLEARANCE,
                 headerHeight(weather) + CLEARANCE);
  Layout_Reserve(RANGE_X - CLEARANCE, RANGE_Y - CLEARANCE, RANGE_W + 2 * CLEARANCE,
                 RANGE_H + CLEARANCE);
}
bool Chrome_MapOverlaps(int x, int y, int width, int height, bool weather) {
  return overlap(x, y, width, height, HEADER_X - CLEARANCE, 0, HEADER_W + 2 * CLEARANCE,
                 headerHeight(weather) + CLEARANCE) ||
         overlap(x, y, width, height, RANGE_X - CLEARANCE, RANGE_Y - CLEARANCE,
                 RANGE_W + 2 * CLEARANCE, RANGE_H + CLEARANCE);
}
void Chrome_DrawMapHeader(bool weather) {
  gfx->fillRoundRect(HEADER_X, -24, HEADER_W, headerHeight(weather) + 24, 24, C_BLACK);
}
void Chrome_DrawHeaderDivider(int y) { gfx->drawFastHLine(LCD_WIDTH / 2 - 64, y, 128, C_DKGRAY); }
void Chrome_DrawStatus(int y) {
  char text[OUTSIDE_TEXT_MAX];
  Outside_StatusText(text, sizeof(text));
  if (!*text) return;
  const int textWidth = Layout_TextW(text, 2);
  // Text itself must fit at BOTH edges on a round display. The black backing
  // may meet the physical rim, but labels must never be clipped there.
  int half = Layout_ChordHalf(y);
  int bottom = Layout_ChordHalf(y + LY_CHAR_H(2));
  if (bottom < half) half = bottom;
  if (textWidth > 2 * half - 16) return;
  int width = textWidth + 32;
  if (width < 192) width = 192;
  gfx->fillRoundRect((LCD_WIDTH - width) / 2, y - 10, width, 38, 18, C_BLACK);
  UI_TextCentered(text, y, C_WHITE, 2);
  Chrome_DrawHeaderDivider(y + 30);
}
void Chrome_DrawRange(const char* text) {
  gfx->fillRoundRect(RANGE_X, RANGE_Y, RANGE_W, RANGE_H, 20, C_BLACK);
  if (Layout_TextW(text, 2) > 2 * Layout_ChordHalf(LY_RANGE + LY_CHAR_H(2)) - 16) return;
  UI_TextCentered(text, LY_RANGE, C_YELLOW, 2);
}
