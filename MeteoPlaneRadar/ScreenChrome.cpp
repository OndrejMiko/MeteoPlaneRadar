#include "ScreenChrome.h"
#include <string.h>
#include "Layout.h"
#include "Outside.h"
#include "UI.h"

namespace {
constexpr int RANGE_X = 168, RANGE_Y = 432, RANGE_W = 144, RANGE_H = 48;
constexpr int CLEARANCE = 12;
struct Box { int x, y, w, h; };
Box statusBox() {
  char text[OUTSIDE_TEXT_MAX]; Outside_StatusText(text, sizeof(text));
  int width = Layout_TextW(text, 2) + 24;
  if (width < 128) width = 128;
  return {(LCD_WIDTH-width)/2, LY_STATUS-8, width, 32};
}
Box metadataBox() { return {164, 70, 152, 54}; }
Box rangeBox() { return {RANGE_X, RANGE_Y, RANGE_W, RANGE_H}; }
Box timelineBox() { return {130, LY_PLAYBACK_LABEL-6, 220, 45}; }
bool overlap(int x,int y,int w,int h,Box b) {
  return x < b.x+b.w+CLEARANCE && x+w > b.x-CLEARANCE &&
         y < b.y+b.h+CLEARANCE && y+h > b.y-CLEARANCE;
}
void reserve(Box b) { Layout_Reserve(b.x-CLEARANCE,b.y-CLEARANCE,b.w+2*CLEARANCE,b.h+2*CLEARANCE); }
}

void Chrome_ReserveMap(bool weather) {
  reserve(statusBox()); reserve(rangeBox());
  if (weather) reserve(timelineBox());
  else { reserve(metadataBox()); reserve({80, LY_SUB-4, 320, 20}); }
}
bool Chrome_MapOverlaps(int x,int y,int width,int height,bool weather) {
  return overlap(x,y,width,height,statusBox()) || overlap(x,y,width,height,rangeBox()) ||
    overlap(x,y,width,height,weather ? timelineBox() : metadataBox()) ||
    (!weather && overlap(x,y,width,height,{80, LY_SUB-4, 320, 20}));
}
void Chrome_DrawPlaneMetadata() {
  const auto b=metadataBox(); gfx->fillRoundRect(b.x,b.y,b.w,b.h,10,C_BLACK);
}
void Chrome_DrawHeaderDivider(int y) { gfx->drawFastHLine(LCD_WIDTH/2-64,y,128,C_DKGRAY); }
void Chrome_DrawStatus(int y) {
  char text[OUTSIDE_TEXT_MAX]; Outside_StatusText(text,sizeof(text));
  if (!*text) return;
  const int textWidth=Layout_TextW(text,2);
  int half=Layout_ChordHalf(y), bottom=Layout_ChordHalf(y+LY_CHAR_H(2));
  if (bottom<half) half=bottom;
  if (textWidth>2*half-16) return;
  // Only the clock gets a small measured backing; no map-wide top rectangle.
  gfx->fillRoundRect((LCD_WIDTH-textWidth-24)/2,y-8,textWidth+24,32,12,C_BLACK);
  UI_TextCentered(text,y,C_WHITE,2);
  Chrome_DrawHeaderDivider(y+30);
}
void Chrome_DrawRange(const char* text) {
  gfx->fillRoundRect(RANGE_X,RANGE_Y,RANGE_W,RANGE_H,20,C_BLACK);
  if (Layout_TextW(text,2)>2*Layout_ChordHalf(LY_RANGE+LY_CHAR_H(2))-16) return;
  UI_TextCentered(text,LY_RANGE,C_YELLOW,2);
}
void Chrome_DrawWeatherTimeline(int frame,int count,const char* label,const char* note) {
  const auto b=timelineBox(); gfx->fillRoundRect(b.x,b.y,b.w,b.h,12,C_BLACK);
  if (count<1) count=1;
  if (frame<0) frame=0;
  if (frame>=count) frame=count-1;
  UI_TextCentered(label,LY_PLAYBACK_LABEL,C_CYAN,1);
  constexpr int x=156, y=LY_PLAYBACK_TRACK, width=168, thumb=8;
  gfx->fillRoundRect(x,y,width,3,1,C_GRAY);
  int pos=count>1 ? frame*(width-thumb)/(count-1) : 0;
  gfx->fillRoundRect(x+pos,y-5,thumb,12,2,C_YELLOW);
  if (note && *note) {
    int w=Layout_TextW(note,1)+16;
    gfx->fillRoundRect((LCD_WIDTH-w)/2,LY_PLAYBACK_NOTE-4,w,16,6,C_BLACK);
    UI_TextCentered(note,LY_PLAYBACK_NOTE,C_YELLOW,1);
  }
}
void Chrome_ReservePrecipLegend() { Layout_Reserve(30,142,108,160); }
void Chrome_DrawPrecipLegend(bool rv) {
  static const uint16_t chmu[6]={0xA000,0xF800,0xFC20,0xE6E0,0x05E0,0x001F};
  static const char* const chmuText[6]={">56 / >100","52 / 65","46 / 27","40 / 12","32 / 3.6","20 / <1"};
  static const uint16_t rain[6]={0xFD5F,0xC000,0xFA20,0xFD40,0x02B1,0x051C};
  static const char* const rainText[6]={">55 / >100","50 / 49","45 / 24","40 / 12","30 / 2.7","20 / 0.7"};
  constexpr int x=42,y=154;
  gfx->fillRoundRect(30,142,108,160,10,C_BLACK);
  gfx->setTextSize(1); gfx->setTextColor(rv ? C_CYAN : C_GREEN);
  gfx->setCursor(x,y); gfx->print(rv ? "RainViewer" : "CHMU");
  gfx->setTextColor(C_GRAY); gfx->setCursor(x,y+16); gfx->print("dBZ / mm/h");
  for (int i=0;i<6;++i) {
    int row=y+36+i*18;
    gfx->fillRect(x,row,9,9,rv ? rain[i] : chmu[i]);
    gfx->drawRect(x,row,9,9,C_DKGRAY);
    gfx->setTextColor(C_WHITE); gfx->setCursor(x+17,row+1);
    gfx->print(rv ? rainText[i] : chmuText[i]);
  }
}
