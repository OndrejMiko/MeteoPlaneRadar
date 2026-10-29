#pragma once
#include <stdint.h>

// Fixed screen furniture. Black notches are painted AFTER map content, while
// reservations and icon exclusion keep meaningful content away from their edges.
void Chrome_ReserveMap(bool weather);
void Chrome_DrawMapHeader(bool weather);
bool Chrome_MapOverlaps(int x, int y, int width, int height, bool weather = false);
void Chrome_DrawStatus(int y);
void Chrome_DrawRange(const char* text);
void Chrome_DrawHeaderDivider(int y);
