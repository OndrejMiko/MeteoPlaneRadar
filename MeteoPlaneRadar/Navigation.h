#pragma once

// Transient navigation hints, shared by screen switching and both map screens.
// Timers start after a screen has entered, not before a potentially slow fetch.
void Navigation_ScreenChanged();
void Navigation_RangeChanged();
bool Navigation_ScreenDotsVisible();
bool Navigation_RangeDotsVisible();
bool Navigation_Tick();       // redraw stays pending until Drawn(), including expiry
void Navigation_Drawn();
void Navigation_ReserveRangeDots(int count);
void Navigation_DrawRangeDots(int selected, int count);
