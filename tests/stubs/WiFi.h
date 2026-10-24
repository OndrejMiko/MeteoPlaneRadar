#pragma once
#define WL_CONNECTED 3
inline bool testWifiConnected=true;
struct TestWiFi {int status(){return testWifiConnected?WL_CONNECTED:0;}};
inline TestWiFi WiFi;
