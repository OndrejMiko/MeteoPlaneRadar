#include "Navigation.h"
#include "Layout.h"
#include "UI.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

Arduino_GFX display;
Arduino_GFX* gfx = &display;
int reserved = 0, rx = 0, ry = 0, rw = 0, rh = 0;
void Layout_Reserve(int x, int y, int w, int h) {
  ++reserved; rx=x; ry=y; rw=w; rh=h;
}
int main() {
  assert(!Navigation_ScreenDotsVisible() && !Navigation_RangeDotsVisible());
  assert(!Navigation_Tick());
  testMillis = 100;
  Navigation_ScreenChanged();
  assert(Navigation_Tick()); Navigation_Drawn(); assert(!Navigation_Tick());
  assert(Navigation_ScreenDotsVisible() && Navigation_RangeDotsVisible());
  testMillis = 5099; assert(!Navigation_Tick());
  testMillis = 5100; assert(Navigation_Tick());
  assert(!Navigation_ScreenDotsVisible() && Navigation_RangeDotsVisible());
  assert(Navigation_Tick()); // expiry survives a skipped/throttled frame
  Navigation_Drawn(); assert(!Navigation_Tick());
  testMillis = 10099; assert(!Navigation_Tick());
  testMillis = 10100; assert(Navigation_Tick());
  assert(!Navigation_RangeDotsVisible()); Navigation_Drawn();
  Navigation_DrawRangeDots(1,5); assert(display.circles==0);
  Navigation_ReserveRangeDots(5); assert(!reserved);
  testMillis = 11000; Navigation_RangeChanged(); Navigation_Drawn();
  assert(!Navigation_ScreenDotsVisible()); // zoom never restarts screen hints
  Navigation_ReserveRangeDots(5); assert(reserved==1);
  assert(rx==433 && ry==197 && rw==22 && rh==86);
  display.fillScreen(C_BLACK); Navigation_DrawRangeDots(1,5);
  assert(display.circles==5);
  auto svg=display.drawing.str();
  assert(svg.find("cx='444' cy='224' r='4' fill='#ffff00'")!=std::string::npos);
  assert(LY_RANGE==444); // bottom notch, below the notification count badge
  testMillis = 20999; assert(!Navigation_Tick());
  Navigation_RangeChanged(); Navigation_Drawn();
  testMillis = 21000; assert(!Navigation_Tick()); // interaction extends timeout
  testMillis = 30999; assert(Navigation_Tick());
  Navigation_Drawn();
  // Same elapsed arithmetic as ESP32, even when millis wraps at 49.7 days.
  testMillis = UINT32_MAX-2000; Navigation_ScreenChanged(); Navigation_Drawn();
  testMillis = 2998; assert(!Navigation_Tick());
  testMillis = 2999; assert(Navigation_Tick());
  assert(!Navigation_ScreenDotsVisible() && Navigation_RangeDotsVisible());
  Navigation_Drawn(); testMillis = 7999; assert(Navigation_Tick());
  assert(!Navigation_RangeDotsVisible());
  std::cout<<"Navigation timers, redraw persistence, zoom geometry and millis wrap passed\n";
}
