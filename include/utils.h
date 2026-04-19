#pragma once

#include "order.h"
#include "simulator.h"

#include <string>
#include <string_view>
#include <vector>

namespace ob {

[[nodiscard]] std::vector<std::string> split_csv_line(std::string_view line);
[[nodiscard]] std::vector<std::string> split_whitespace(std::string_view line);
[[nodiscard]] Side parse_side(std::string_view token);
[[nodiscard]] EventType parse_event_type(std::string_view token);
[[nodiscard]] bool is_blank(std::string_view token);

}  // namespace ob
