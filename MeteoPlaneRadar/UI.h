// =============================================================================
//  MeteoPlaneRadar
//  Shared UI helpers - colours, global gfx, interface.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
//  Board:   Waveshare ESP32-S3-Touch-LCD-2.1 (round 480x480 display, ST7701)
// =============================================================================
#pragma once
#include <Arduino_GFX_Library.h>

// Colours (RGB565)
#define C_BLACK  0x0000
#define C_BLUE   0x001F
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_WHITE  0xFFFF
#define C_YELLOW 0xFFE0
#define C_GRAY   0x8410
#define C_DKGRAY 0x2124
#define C_CYAN   0x05FF
#define C_ORANGE 0xFC00   // altitude band 2-6 km

// Global display (defined in the .ino).
extern Arduino_GFX* gfx;

// Draw a WiFi QR code (for joining the AP). open=true -> open network.
void UI_DrawWifiQR(const char* ssid, const char* password, bool open,
                   int x, int y, int size_px);

// Horizontally centred text (size 1-4).
void UI_TextCentered(const char* text, int cy, uint16_t color, uint8_t size);

// Text centred inside the rectangle [x, x+w) - used for labels above the map.
void UI_TextCenteredIn(const char* text, int x, int w, int cy,
                       uint16_t color, uint8_t size);

// Half the width of the display circle at height y - i.e. how much room a line
// of text actually has there. On a round panel the usable width shrinks fast
// towards the top, so anything near the edge has to be measured, not assumed.

// The clock + outside temperature line, centred under the screen dots. Draws
// nothing when neither is known yet, and refuses to draw text that would not
// fit inside the circle rather than letting it run off the edge.
void UI_DrawStatusLine(int cy);

// Has that line changed since the last time anyone asked?
//
// The screens only redraw when their own Tick() says something happened, and
// the clock at the top is drawn as part of that redraw. On a screen whose data
// changes rarely - the forecast arrives twice an hour - the clock would
// otherwise sit still between updates and look frozen.
// Those screens call this from their Tick() so the minute keeps moving.
//
// Compares the whole line rather than just the minute, so the outside
// temperature changing is caught too. One shared record of the last text is
// enough because only the active screen is ticked, and switching screens
// redraws everything anyway.
bool UI_StatusLineChanged();
