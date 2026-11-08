#include "AAFont.h"
#include "Settings.h"
#include "Lang.h"
#include "UI.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <algorithm>
Arduino_GFX display;
Arduino_GFX* gfx = &display;
int main() {
  Settings_ClearAll(); Settings_Begin();
  const char* const letters[] = {"á","ä","č","ď","é","í","ĺ","ľ","ň","ó","ô","ŕ","š","ť","ú","ý","ž","Ľ","Ĺ","Ô","Ŕ"};
  for (int face=1;face<=3;++face) {
    Settings_SetClockFont(face); Settings_SetLanguage(LANG_SK);
    assert(std::string(T(S_FORECAST))=="Predpoveď");
    assert(std::string(Lang_WeekdayShort(4))=="Št");
    assert(std::string(Lang_MonthName(9))=="októbra");
    const auto* fs = AAFont_ClockSet(face); assert(fs);
    for (const char* letter:letters) {
      assert(AAFont_TextW(fs->small,letter)>0);
      for (int size=1;size<=5;++size)
        assert(AAText_Width((const uint8_t*)letter,strlen(letter),size)>0);
    }
    for (int size=1;size<=5;++size) {
      const char* label="Ľubľana — Predpoveď";
      int w=AAText_Width((const uint8_t*)label,strlen(label),size);
      assert(w>0 && w<=int(strlen(label))*6*size);
      const uint8_t partial[]={0xe2,0x80}; // non-terminated partial UTF-8 slice
      (void)AAText_Width(partial,sizeof(partial),size);
      (void)AAText_Print(partial,sizeof(partial),0,0,size,0xffff,0xffff);
    }
    std::vector<uint16_t> first(480*480+2,0x1234), second(first);
    AAFont_SetFramebuffer(first.data()+1);
    AAFont_Draw(fs->small,"Ľúbim dážď",-4,12,0xffff);
    assert(first.front()==0x1234 && first.back()==0x1234);
    assert(std::count(first.begin(),first.end(),0x1234)<int(first.size()));
    const auto saved=first;
    AAFont_SetFramebuffer(second.data()+1);
    AAFont_Draw(fs->small,"štvrtok",470,479,0xffff);
    assert(first==saved && second.front()==0x1234 && second.back()==0x1234);
    AAFont_SetFramebuffer(nullptr);
  }
  Settings_SetClockFont(CLOCK_FONT_PIXEL);
  assert(std::string(T(S_FORECAST))=="Predpoved");
  std::cout<<"Fonts: Slovak glyph coverage, pixel fallback, bounded UTF-8 and framebuffer switching passed\n";
}
