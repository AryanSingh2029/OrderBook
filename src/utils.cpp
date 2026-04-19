#include "utils.h"

#include <cctype>
#include <stdexcept>

namespace ob {

std::vector<std::string> split_csv_line(std::string_view line) {
    std::vector<std::string> fields;
    std::string current;

    for (const char ch : line) {
        if (ch == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    fields.push_back(current);

    return fields;
}

std::vector<std::string> split_whitespace(std::string_view line) {
    std::vector<std::string> tokens;
    std::string current;

    for (const char ch : line) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

Side parse_side(std::string_view token) {
    if (token == "BUY") {
        return Side::Buy;
    }
    if (token == "SELL") {
        return Side::Sell;
    }
    throw std::invalid_argument("invalid side token");
}

EventType parse_event_type(std::string_view token) {
    if (token == "ADD") {
        return EventType::Add;
    }
    if (token == "LIMIT") {
        return EventType::Add;
    }
    if (token == "MARKET") {
        return EventType::Market;
    }
    if (token == "CANCEL") {
        return EventType::Cancel;
    }
    throw std::invalid_argument("invalid event type token");
}

bool is_blank(std::string_view token) {
    for (const char ch : token) {
        if (std::isspace(static_cast<unsigned char>(ch)) == 0 && ch != ',') {
            return false;
        }
    }
    return true;
}

}  // namespace ob
