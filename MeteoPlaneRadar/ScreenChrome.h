#pragma once
#include <stdint.h>

// Fixed screen furniture. Black notches are painted AFTER map content, while
// reservations and icon exclusion keep meaningful content away from their edges.
void Chrome_ReserveMap(bool weather);
void Chrome_DrawPlaneMetadata();
void Chrome_DrawWeatherTimeline(int frame, int count, const char* label, const char* note);
void Chrome_ReservePrecipLegend();
void Chrome_DrawPrecipLegend(bool rainViewer);
bool Chrome_MapOverlaps(int x, int y, int width, int height, bool weather = false);
void Chrome_DrawStatus(int y);
void Chrome_DrawRange(const char* text);
void Chrome_DrawHeaderDivider(int y);
