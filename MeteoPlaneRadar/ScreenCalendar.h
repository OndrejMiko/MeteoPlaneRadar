#pragma once
#include <Arduino.h>
#include <time.h>
void ScreenCalendar_Enter();
void ScreenCalendar_Draw();
bool ScreenCalendar_Tick();
bool ScreenCalendar_HandleTap(int x, int y);
void ScreenCalendar_ChangeRange(int direction);
void ScreenCalendar_RangeText(char* out, size_t cap);
void ScreenCalendar_SelectDay(time_t day);
