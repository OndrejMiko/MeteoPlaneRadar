#pragma once
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <fstream>
#include <cstdint>
#include "AAFont.h"

// Record the real screen's drawing calls so host tests can inspect geometry,
// source markers and text without an ESP32 panel.
class Arduino_GFX {
 public:
  int width() const { return 480; }
  int height() const { return 480; }
  int x=0,y=0,size=1;uint16_t color=0xffff;
  int circles=0;std::vector<std::string> texts;std::ostringstream drawing;
  static std::string rgb(uint16_t c){char s[10];snprintf(s,sizeof(s),"#%02x%02x%02x",((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31);return s;}
  void fillScreen(uint16_t c){circles=0;texts.clear();drawing.str("");drawing.clear();drawing<<"<rect width='480' height='480' fill='"<<rgb(c)<<"'/>";}
  void setTextColor(uint16_t c){color=c;}
  void setTextSize(uint8_t s){size=s;}
  void setCursor(int a,int b){x=a;y=b;}
  void print(const char* s){texts.push_back(s);if(AAText_Enabled()&&size<=5){x+=AAText_Print((const uint8_t*)s,strlen(s),x,y,size,color,color);return;}std::string esc;for(char c:std::string(s)){if(c=='&')esc+="&amp;";else if(c=='<')esc+="&lt;";else if(c=='>')esc+="&gt;";else esc+=c;}drawing<<"<text x='"<<x<<"' y='"<<y+7*size<<"' font-family='monospace' font-size='"<<8*size<<"' textLength='"<<strlen(s)*6*size<<"' lengthAdjust='spacingAndGlyphs' fill='"<<rgb(color)<<"'>"<<esc<<"</text>";x+=(int)strlen(s)*6*size;}
  void print(const std::string& s){print(s.c_str());}
  template<typename T> void print(T value){std::ostringstream stream;stream<<value;print(stream.str().c_str());}
  void fillRect(int a,int b,int w,int h,uint16_t c){fillRoundRect(a,b,w,h,0,c);}
  void drawRect(int a,int b,int w,int h,uint16_t c){drawRoundRect(a,b,w,h,0,c);}
  void drawPixel(int a,int b,uint16_t c){fillRect(a,b,1,1,c);}
  void drawFastVLine(int a,int b,int h,uint16_t c){drawLine(a,b,a,b+h,c);}
  void fillTriangle(int a,int b,int c,int d,int e,int f,uint16_t color){drawing<<"<polygon points='"<<a<<","<<b<<" "<<c<<","<<d<<" "<<e<<","<<f<<"' fill='"<<rgb(color)<<"'/>";}
  void fillRoundRect(int a,int b,int w,int h,int r,uint16_t c){drawing<<"<rect x='"<<a<<"' y='"<<b<<"' width='"<<w<<"' height='"<<h<<"' rx='"<<r<<"' fill='"<<rgb(c)<<"'/>";}
  void drawRoundRect(int a,int b,int w,int h,int r,uint16_t c){drawing<<"<rect x='"<<a<<"' y='"<<b<<"' width='"<<w<<"' height='"<<h<<"' rx='"<<r<<"' fill='none' stroke='"<<rgb(c)<<"'/>";}
  void fillCircle(int a,int b,int r,uint16_t c){circles++;drawing<<"<circle cx='"<<a<<"' cy='"<<b<<"' r='"<<r<<"' fill='"<<rgb(c)<<"'/>";}
  void drawCircle(int a,int b,int r,uint16_t c){circles++;drawing<<"<circle cx='"<<a<<"' cy='"<<b<<"' r='"<<r<<"' fill='none' stroke='"<<rgb(c)<<"'/>";}
  void drawLine(int a,int b,int c,int d,uint16_t color){drawing<<"<path d='M"<<a<<" "<<b<<"L"<<c<<" "<<d<<"' stroke='"<<rgb(color)<<"'/>";}
  void drawFastHLine(int a,int b,int w,uint16_t c){drawing<<"<path d='M"<<a<<" "<<b<<"h"<<w<<"' stroke='"<<rgb(c)<<"'/>";}
  void snapshot(const char* path){std::ofstream f(path);f<<"<svg xmlns='http://www.w3.org/2000/svg' width='480' height='480'><defs><clipPath id='round'><circle cx='240' cy='240' r='239'/></clipPath></defs><g clip-path='url(#round)'>"<<drawing.str()<<"</g></svg>";}
};
