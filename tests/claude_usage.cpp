#include "ClaudeUsage.h"
#include <cassert>
#include <iostream>

int main() {
  const int64_t now = 1793700000;
  auto accept = [&](const char* json) {
    JsonDocument doc; assert(!deserializeJson(doc, json));
    return ClaudeUsage_Accept(doc.as<JsonObjectConst>(), now);
  };
  ClaudeUsage_Clear();
  assert(!ClaudeUsage_Get().fiveHour.present);
  assert(accept(R"({"observedAt":1793700000,"five_hour":{"used_percentage":68.5,"resets_at":1793706000},"seven_day":{"used_percentage":42,"resets_at":1793900000}})"));
  assert(ClaudeUsage_Get().fiveHour.used == 68.5f);
  assert(ClaudeUsage_Get().sevenDay.resetsAt == 1793900000);
  auto revision = ClaudeUsage_Revision();
  for (const char* bad : {
      R"({"observedAt":1793700000,"five_hour":{"used_percentage":101,"resets_at":1793706000}})",
      R"({"observedAt":1793700000,"five_hour":{"used_percentage":-1,"resets_at":1793706000}})",
      R"({"observedAt":1793700000,"five_hour":{"used_percentage":"50","resets_at":1793706000}})",
      R"({"observedAt":1793700000,"five_hour":{"used_percentage":true,"resets_at":1793706000}})",
      R"({"observedAt":1793700000,"five_hour":{"used_percentage":50,"resets_at":1793700000}})",
      R"({"observedAt":1793700000,"five_hour":{"used_percentage":50,"resets_at":1794706000}})",
      R"({"observedAt":1793699999})", R"({"observedAt":1793700061})",
      R"({"observedAt":1793000000})", R"({"observedAt":"1793700000"})", R"({})"}) {
    assert(!accept(bad));
    assert(ClaudeUsage_Revision() == revision); // malformed input cannot partially replace data
    assert(ClaudeUsage_Get().fiveHour.used == 68.5f);
  }
  assert(accept(R"({"observedAt":1793700000,"seven_day":{"used_percentage":0,"resets_at":1793900000}})"));
  assert(!ClaudeUsage_Get().fiveHour.present); // absent is unknown, not 100% remaining
  assert(ClaudeUsage_Get().sevenDay.present && ClaudeUsage_Get().sevenDay.used == 0);
  assert(accept(R"({"observedAt":1793700000})"));
  assert(!ClaudeUsage_Get().sevenDay.present);
  std::cout << "Claude usage: bounded windows, clock errors, replay and atomic validation passed\n";
}
