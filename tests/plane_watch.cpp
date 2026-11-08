#include "PlaneWatch.h"
#include "PlaneIcon.h"
#include "Settings.h"
#include "ADSB.h"
#include "Status.h"
#include <cassert>
#include <cstring>
#include <iostream>
static Aircraft aircraft[2];
static bool stale=false;
static int fetches=0;
static float radius=0;
const Aircraft* ADSB_List(){return aircraft;}
int ADSB_Count(){return 2;}
bool ADSB_DataStale(){return stale;}
unsigned long ADSB_PauseLeftMs(){return 0;}
bool ADSB_Fetch(double,double,float r){++fetches;radius=r;return true;}
void Status_Set(StatusSlot,const char*,...){}
int main(){
  Settings_ClearAll(); Settings_Begin(); Settings_SetLocation(48,17);
  Settings_SetPlaneAlert(true); Settings_SetPlaneAlertKm(5);
  for(int i=0;i<2;++i){aircraft[i].lat=i?48.09:47.982;aircraft[i].lon=17;aircraft[i].altFt=1000;strcpy(aircraft[i].hex,i?"BBB":"AAA");strcpy(aircraft[i].callsign,"TEST");}
  float km=0,bearing=0;bool window=false;
  assert(PlaneWatch_Best(&km,&bearing,&window)==0 && !window);
  Settings_SetWindowOn(true);Settings_SetWindowAz(0);Settings_SetWindowWidth(90);Settings_SetWindowKm(20);
  assert(PlaneWatch_Best(&km,&bearing,&window)==1 && window);
  char hex[8];assert(PlaneWatch_Take(hex,sizeof(hex),&km,&window) && !strcmp(hex,"BBB") && window);
  assert(PlaneWatch_Take(hex,sizeof(hex),&km,&window) && !strcmp(hex,"AAA"));
  assert(!PlaneWatch_Take(hex,sizeof(hex),&km,&window));
  stale=true;assert(PlaneWatch_Best(nullptr,nullptr,nullptr)==-1);assert(!PlaneWatch_Take(hex,sizeof(hex),nullptr,nullptr));
  stale=false;testMillis+=PLANE_ALERT_FORGET_MS+1;
  assert(PlaneWatch_Take(hex,sizeof(hex),nullptr,nullptr) && !strcmp(hex,"BBB"));
  Settings_SetOnlyWithCallsign(true);aircraft[1].callsign[0]=0;
  assert(PlaneWatch_Best(nullptr,nullptr,nullptr)==0);
  Settings_SetAltRangeFt(5000,6000);assert(PlaneWatch_Best(nullptr,nullptr,nullptr)==-1);
  aircraft[0].altFt=5500;assert(PlaneWatch_Best(nullptr,nullptr,nullptr)==0);
  aircraft[0].onGround=true;assert(PlaneWatch_Best(nullptr,nullptr,nullptr)==-1);
  PlaneWatch_Tick(true);assert(fetches==0);
  PlaneWatch_Tick(false);assert(fetches==1 && radius>=20);
  Settings_SetPlaneAlert(false);Settings_SetHaEnabled(false);testMillis+=100000;
  PlaneWatch_Tick(false);assert(fetches==1);
  assert(PlaneIcon_Classify("A320","")==PI_NARROW);
  assert(PlaneIcon_Classify("b748","")==PI_QUAD);
  assert(PlaneIcon_Classify("C172","")==PI_LIGHT);
  assert(PlaneIcon_Classify("UNKNOWN","")==PI_GENERIC);
  std::cout<<"Aircraft watch: window priority, dedupe, stale data, filters, polling and icon classification passed\n";
}
