#pragma once
#include "UI.h"

// Shared notification/aircraft window chrome. The 64px touch target has no fill;
// only the 32px X is visible, with the same strokes on both overlays.
constexpr uint16_t OVERLAY_PANEL = 0x1105;
constexpr int OVERLAY_CLOSE_SIZE = 64;
inline void Overlay_DrawClose(int x, int y) {
  for (int t = -1; t <= 1; ++t) {
    gfx->drawLine(x + 16, y + 16 + t, x + 47, y + 47 + t, C_GRAY);
    gfx->drawLine(x + 16, y + 47 + t, x + 47, y + 16 + t, C_GRAY);
  }
}
inline bool Overlay_CloseHit(int x, int y, int targetX, int targetY) {
  return x >= targetX && x < targetX + OVERLAY_CLOSE_SIZE &&
         y >= targetY && y < targetY + OVERLAY_CLOSE_SIZE;
}
