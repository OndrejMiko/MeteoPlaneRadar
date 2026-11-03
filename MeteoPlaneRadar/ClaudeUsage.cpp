#include "ClaudeUsage.h"
#include <math.h>

static ClaudeSnapshot snapshot = {};
static uint32_t revision = 0;
static bool readWindow(JsonVariantConst v, ClaudeWindow& out, int64_t observed) {
  if (v.isNull()) return true;
  if (!v.is<JsonObjectConst>() || !v["used_percentage"].is<float>() ||
      !v["resets_at"].is<int64_t>()) return false;
  float used = v["used_percentage"].as<float>();
  int64_t reset = v["resets_at"].as<int64_t>();
  if (!isfinite(used) || used < 0 || used > 100 || reset <= observed ||
      reset > observed + 8 * 86400) return false;
  out = {true, used, reset};
  return true;
}
bool ClaudeUsage_Accept(JsonObjectConst input, int64_t now) {
  if (!input["observedAt"].is<int64_t>()) return false;
  ClaudeSnapshot next = {};
  next.observedAt = input["observedAt"].as<int64_t>();
  // Reject clock errors and replay of older snapshots; retrying the same one is OK.
  if (now < 1700000000 || next.observedAt < now - 7 * 86400 ||
      next.observedAt > now + 60 || next.observedAt < snapshot.observedAt) return false;
  if (!readWindow(input["five_hour"], next.fiveHour, next.observedAt) ||
      !readWindow(input["seven_day"], next.sevenDay, next.observedAt)) return false;
  snapshot = next; ++revision;
  return true;
}
const ClaudeSnapshot& ClaudeUsage_Get() { return snapshot; }
uint32_t ClaudeUsage_Revision() { return revision; }
void ClaudeUsage_Clear() { snapshot = {}; ++revision; }
