#pragma once
#include <string>

struct Question {
    int id;
    int test_id;
    std::string text;
    std::string type;   // single, multiple, text
    int order_index;
};

inline std::string questionToJson(const Question& q) {
    return "{\"id\":" + std::to_string(q.id) +
           ",\"test_id\":" + std::to_string(q.test_id) +
           ",\"text\":\"" + q.text + "\"" +
           ",\"type\":\"" + q.type + "\"" +
           ",\"order_index\":" + std::to_string(q.order_index) + "}";
}
