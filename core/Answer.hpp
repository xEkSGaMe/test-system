#pragma once
#include <string>

struct Answer {
    int id;
    int question_id;
    std::string text;
    bool is_correct;
};

inline std::string answerToJson(const Answer& a) {
    return "{\"id\":" + std::to_string(a.id) +
           ",\"question_id\":" + std::to_string(a.question_id) +
           ",\"text\":\"" + a.text + "\"" +
           ",\"is_correct\":" + (a.is_correct ? "true" : "false") + "}";
}
