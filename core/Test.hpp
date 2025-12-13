#pragma once
#include <string>
#include <optional>

struct Test {
    int id;
    std::string title;
    std::optional<std::string> description;
    std::optional<int> author_id;
    bool is_published;
};

inline std::string testToJson(const Test& t) {
    std::string json = "{";
    json += "\"id\":" + std::to_string(t.id) + ",";
    json += "\"title\":\"" + t.title + "\"";
    if (t.description.has_value())
        json += ",\"description\":\"" + *t.description + "\"";
    if (t.author_id.has_value())
        json += ",\"author_id\":" + std::to_string(*t.author_id);
    json += ",\"is_published\":" + std::string(t.is_published ? "true" : "false");
    json += "}";
    return json;
}
