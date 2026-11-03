#pragma once
#include <ArduinoJson.h>
#include <stdint.h>

struct ClaudeWindow {
  bool present;
  float used;
  int64_t resetsAt;
};
struct ClaudeSnapshot {
  int64_t observedAt;
  ClaudeWindow fiveHour, sevenDay;
};
// RAM only. Missing/expired windows are unknown, never assumed to be unused.
bool ClaudeUsage_Accept(JsonObjectConst input, int64_t now);
const ClaudeSnapshot& ClaudeUsage_Get();
uint32_t ClaudeUsage_Revision();
void ClaudeUsage_Clear();
