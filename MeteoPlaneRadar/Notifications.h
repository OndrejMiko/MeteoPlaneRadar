#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

// All mutation and drawing runs on loop(), including WebServer handlers.
const char* Notifications_Validate(JsonObjectConst message);
bool Notifications_Push(JsonObjectConst message);  // false: disabled, full, no PSRAM
bool Notifications_Authorized(const char* token);
bool Notifications_Tick();
void Notifications_Draw();
bool Notifications_Visible();
// Captures every gesture over an open notification; badge captures taps only.
bool Notifications_Touch(int x, int y, bool tap);
size_t Notifications_MemoryBytes();
unsigned Notifications_Count();
uint32_t Notifications_Dropped();
uint32_t Notifications_LastReceived();
