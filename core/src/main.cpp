#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <map>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pqxx/pqxx>
#include <optional>

#include "database/Database.hpp"
#include "models/Test.hpp"
#include "models/Question.hpp"
#include "models/Answer.hpp"
#include "services/TestService.hpp"
#include "services/QuestionService.hpp"
#include "services/AnswerService.hpp"

// Чтение HTTP-запроса
std::string read_http_request(int client_socket) {
    char buffer[4096] = {0};
    ssize_t valread = read(client_socket, buffer, 4096);
    if (valread > 0) return std::string(buffer, valread);
    return "";
}

// Парсинг первой строки запроса
// Заменить существующую parse_request на этот код
std::map<std::string, std::string> parse_request(const std::string& request) {
    std::map<std::string, std::string> parsed;
    std::stringstream ss(request);
    std::string line;

    // Первая строка: METHOD PATH HTTP/1.1
    if (!std::getline(ss, line)) return parsed;
    std::stringstream request_line(line);
    std::string method, path, http_version;
    request_line >> method >> path >> http_version;
    parsed["method"] = method;
    parsed["path"] = path;

    // Заголовки: читаем до пустой строки
    while (std::getline(ss, line)) {
        if (line == "\r" || line == "") break;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = line.substr(0, colon);
            size_t value_start = colon + 1;
            while (value_start < line.size() && (line[value_start] == ' ' || line[value_start] == '\t')) ++value_start;
            std::string value = line.substr(value_start);
            if (!value.empty() && value.back() == '\r') value.pop_back();
            parsed["hdr:" + name] = value;
        }
    }

    // Тело (если есть)
    size_t body_start = request.find("\r\n\r\n");
    if (body_start != std::string::npos) parsed["body"] = request.substr(body_start + 4);
    else parsed["body"] = "";

    return parsed;
}


// Вспомогательная функция для извлечения ID из пути
std::optional<int> extract_id_from_path(const std::string& path, const std::string& prefix) {
    if (path.rfind(prefix, 0) == 0 && path.length() > prefix.length()) {
        std::string id_str = path.substr(prefix.length());
        try { return std::stoi(id_str); } catch (...) { return std::nullopt; }
    }
    return std::nullopt;
}

// Обработка запроса
std::string handle_request(const std::string& full_request,
                           TestService& testService,
                           QuestionService& questionService,
                           AnswerService& answerService,
                           Database& db) {
    auto request_data = parse_request(full_request);
    const std::string& path = request_data.at("path");
    const std::string& method = request_data.at("method");
    std::string response_body;
    std::string status_line;

    // ---------- TESTS ----------
    if (method == "GET" && path == "/tests") {
        auto tests = testService.list();
        std::stringstream ss;
        ss << "[";
        bool first = true;
        for (const auto& test : tests) {
            if (!first) ss << ",";
            ss << testToJson(test);
            first = false;
        }
        ss << "]";
        status_line = "HTTP/1.1 200 OK";
        response_body = ss.str();
    }
    else if (method == "GET" && extract_id_from_path(path, "/tests/")) {
        int test_id = extract_id_from_path(path, "/tests/").value();
        auto test = testService.get(test_id);
        if (test) { status_line = "HTTP/1.1 200 OK"; response_body = testToJson(*test); }
        else { status_line = "HTTP/1.1 404 Not Found"; response_body = "{\"message\":\"Test not found\"}"; }
    }
    else if (method == "POST" && path == "/tests") {
        std::string body = request_data.count("body") ? request_data.at("body") : "";
        std::string title = "Untitled";
        size_t pos = body.find("\"title\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            size_t quote1 = body.find("\"", colon);
            size_t quote2 = body.find("\"", quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                title = body.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }
        int new_id = testService.create(title, {});
        status_line = "HTTP/1.1 201 Created";
        response_body = "{\"id\":" + std::to_string(new_id) + "}";
    }
    else if (method == "PUT" && extract_id_from_path(path, "/tests/")) {
        int test_id = extract_id_from_path(path, "/tests/").value();
        std::string body = request_data.count("body") ? request_data.at("body") : "";
        std::optional<std::string> new_title;
        size_t pos = body.find("\"title\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            size_t quote1 = body.find("\"", colon);
            size_t quote2 = body.find("\"", quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                new_title = body.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }
        bool ok = testService.update(test_id, new_title, {}, {});
        if (ok) { status_line = "HTTP/1.1 200 OK"; response_body = "{\"message\":\"Test updated\"}"; }
        else { status_line = "HTTP/1.1 404 Not Found"; response_body = "{\"message\":\"Test not found\"}"; }
    }
    else if (method == "DELETE" && extract_id_from_path(path, "/tests/")) {
        int test_id = extract_id_from_path(path, "/tests/").value();
        bool ok = testService.remove(test_id);
        if (ok) { status_line = "HTTP/1.1 200 OK"; response_body = "{\"message\":\"Test deleted\"}"; }
        else { status_line = "HTTP/1.1 404 Not Found"; response_body = "{\"message\":\"Test not found\"}"; }
    }

    // ---------- QUESTIONS ----------
    else if (method == "GET" && path.find("/tests/") == 0 && path.find("/questions") != std::string::npos) {
        int test_id = std::stoi(path.substr(7, path.find("/questions") - 7));
        auto questions = questionService.list_by_test(test_id);
        std::stringstream ss;
        ss << "[";
        bool first = true;
        for (const auto& q : questions) {
            if (!first) ss << ",";
            ss << questionToJson(q);
            first = false;
        }
        ss << "]";
        status_line = "HTTP/1.1 200 OK";
        response_body = ss.str();
    }
    else if (method == "POST" && path.find("/tests/") == 0 && path.find("/questions") != std::string::npos) {
        int test_id = std::stoi(path.substr(7, path.find("/questions") - 7));
        std::string body = request_data.count("body") ? request_data.at("body") : "";
        std::string text = "Question";
        std::string type = "single";
        int order_index = 1;
        size_t pos = body.find("\"text\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            size_t q1 = body.find("\"", colon);
            size_t q2 = body.find("\"", q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) text = body.substr(q1 + 1, q2 - q1 - 1);
        }
        pos = body.find("\"type\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            size_t q1 = body.find("\"", colon);
            size_t q2 = body.find("\"", q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) type = body.substr(q1 + 1, q2 - q1 - 1);
        }
        pos = body.find("\"order_index\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            order_index = std::stoi(body.substr(colon+1));
        }
        int qid = questionService.create(test_id, text, type, order_index);
        status_line = "HTTP/1.1 201 Created";
        response_body = "{\"id\":" + std::to_string(qid) + "}";
    }
        else if (method == "DELETE" && path.find("/questions/") == 0) {
        int qid = std::stoi(path.substr(11));
        bool ok = questionService.remove(qid);
        status_line = ok ? "HTTP/1.1 200 OK" : "HTTP/1.1 404 Not Found";
        response_body = ok ? "{\"message\":\"Question deleted\"}" : "{\"message\":\"Question not found\"}";
    }

    // ---------- ANSWERS ----------
    else if (method == "GET" && path.find("/questions/") == 0 && path.find("/answers") != std::string::npos) {
        int qid = std::stoi(path.substr(11, path.find("/answers") - 11));
        auto answers = answerService.list_by_question(qid);
        std::stringstream ss;
        ss << "[";
        bool first = true;
        for (const auto& a : answers) {
            if (!first) ss << ",";
            ss << answerToJson(a);
            first = false;
        }
        ss << "]";
        status_line = "HTTP/1.1 200 OK";
        response_body = ss.str();
    }
    else if (method == "POST" && path.find("/questions/") == 0 && path.find("/answers") != std::string::npos) {
        int qid = std::stoi(path.substr(11, path.find("/answers") - 11));
        std::string body = request_data.count("body") ? request_data.at("body") : "";
        std::string text = "Answer";
        bool is_correct = false;

        size_t pos = body.find("\"text\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            size_t q1 = body.find("\"", colon);
            size_t q2 = body.find("\"", q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) text = body.substr(q1 + 1, q2 - q1 - 1);
        }
        pos = body.find("\"is_correct\"");
        if (pos != std::string::npos) {
            size_t colon = body.find(":", pos);
            std::string val = body.substr(colon+1);
            if (val.find("true") != std::string::npos) is_correct = true;
        }

        int aid = answerService.create(qid, text, is_correct);
        status_line = "HTTP/1.1 201 Created";
        response_body = "{\"id\":" + std::to_string(aid) + "}";
    }
    else if (method == "DELETE" && path.find("/answers/") == 0) {
        int aid = std::stoi(path.substr(9));
        bool ok = answerService.remove(aid);
        status_line = ok ? "HTTP/1.1 200 OK" : "HTTP/1.1 404 Not Found";
        response_body = ok ? "{\"message\":\"Answer deleted\"}" : "{\"message\":\"Answer not found\"}";
    }

    // ---------- HEALTH & ROOT ----------
    // ---------- ATTEMPTS: POST /api/tests/{id}/submit ----------
else if (method == "POST" && path.find("/api/tests/") == 0 && path.size() > std::string("/api/tests/").size() + std::string("/submit").size() && path.rfind("/submit") == path.size() - 7) {
    // извлечь id между "/api/tests/" и "/submit"
    std::string id_part = path.substr(std::string("/api/tests/").length(), path.length() - std::string("/api/tests/").length() - std::string("/submit").length());
    int test_id = 0;
    try { test_id = std::stoi(id_part); } catch (...) {
        status_line = "HTTP/1.1 400 Bad Request";
        response_body = "{\"code\":\"BAD_REQUEST\",\"message\":\"Invalid test id\"}";
        // продолжим к отправке ответа
    } 

    if (test_id != 0) {
        // Получить Authorization header (временно: "Bearer <user_id>" используется для тестирования)
        std::string auth_header = "";
        if (request_data.count("hdr:Authorization")) auth_header = request_data.at("hdr:Authorization");
        int user_id = 0;
        if (!auth_header.empty()) {
            size_t pos = auth_header.find(" ");
            if (pos != std::string::npos) {
                std::string token = auth_header.substr(pos + 1);
                try { user_id = std::stoi(token); } catch (...) { user_id = 0; }
            }
        }
        if (user_id == 0) {
            status_line = "HTTP/1.1 401 Unauthorized";
            response_body = "{\"code\":\"UNAUTHORIZED\",\"message\":\"Missing or invalid Authorization header (use 'Bearer <user_id>' for now)\"}";
        } else {
            // Тело запроса (опционально initial_answers) — сохраняем как JSON строку
            std::string answers_json = "null";
            if (request_data.count("body") && !request_data.at("body").empty()) answers_json = request_data.at("body");

            // Проверка существования теста
            try {
                pqxx::work w(db.conn());
                pqxx::result r = w.exec_params("SELECT id FROM tests WHERE id = $1", test_id);
                if (r.empty()) {
                    status_line = "HTTP/1.1 404 Not Found";
                    response_body = "{\"code\":\"NOT_FOUND\",\"message\":\"Test not found\"}";
                } else {
                    // Вставка попытки (answers сохраняем в поле answers JSONB)
                    pqxx::work tx(db.conn());
                    pqxx::result ins = tx.exec_params(
                        "INSERT INTO attempts (user_id, test_id, answers, status) VALUES ($1, $2, $3::jsonb, $4) RETURNING id, started_at",
                        user_id, test_id, answers_json, std::string("in_progress")
                    );
                    tx.commit();

                    int attempt_id = ins[0][0].as<int>();
                    std::string started_at = ins[0][1].as<std::string>();

                    // Сформировать ответ вручную (без внешних JSON-библиотек)
                    std::ostringstream oss;
                    oss << "{\"attempt_id\":" << attempt_id
                        << ",\"test_id\":" << test_id
                        << ",\"user_id\":" << user_id
                        << ",\"started_at\":\"" << started_at << "\""
                        << ",\"status\":\"in_progress\"}";
                    status_line = "HTTP/1.1 201 Created";
                    response_body = oss.str();
                }
            } catch (const std::exception &e) {
                status_line = "HTTP/1.1 500 Internal Server Error";
                response_body = std::string("{\"code\":\"DB_ERROR\",\"message\":\"") + e.what() + "\"}";
            }
        }
    }
}

    else if (path == "/health") {
        try {
            pqxx::connection C(db.get_connection_string());
            status_line = "HTTP/1.1 200 OK";
            response_body = "{\"status\":\"ok\",\"db\":\"connected\"}";
        } catch (...) {
            status_line = "HTTP/1.1 503 Service Unavailable";
            response_body = "{\"status\":\"error\",\"db\":\"disconnected\"}";
        }
    }
    else if (path == "/") {
        status_line = "HTTP/1.1 200 OK";
        response_body = "{\"message\":\"Core API Server running\"}";
    }
    else {
        status_line = "HTTP/1.1 404 Not Found";
        response_body = "{\"message\":\"Not Found\"}";
    }

    std::string response = status_line + "\r\n";
    response += "Content-Type: application/json\r\n";
    response += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
    response += "\r\n";
    response += response_body;
    return response;
}
int main() {
    const char* db_url_env = std::getenv("DATABASE_URL");
    if (!db_url_env) {
        std::cerr << "❌ DATABASE_URL not set." << std::endl;
        return 1;
    }

    Database db(db_url_env);
    TestService testService(db);
    QuestionService questionService(db);
    AnswerService answerService(db);

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int port = 8080;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "=== CORE API SERVER ===" << std::endl;

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }
        std::string request = read_http_request(new_socket);
        std::string response = handle_request(request, testService, questionService, answerService, db);
        send(new_socket, response.c_str(), response.size(), 0);
        close(new_socket);
    }

    return 0;
}
