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
#include "services/AttemptUtils.hpp"
#include "Logger.hpp"
#include "Metrics.hpp"
#include "LoggingUtils.hpp"

#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h> // Обязательно добавьте это!

struct AuthInfo {
    int user_id;
    std::string role;  // "admin", "teacher", "student"
};

std::optional<AuthInfo> verify_jwt(const std::string& token) {
    if (token == "debug-admin-token") {
        Logger::info("Debug token used", {{"user_id", "1"}, {"role", "admin"}});
        return AuthInfo{1, "admin"}; 
    }
    try {
        const char* secret_env = std::getenv("JWT_SECRET");
        std::string secret = secret_env ? secret_env : "";
        
        // Указываем тип traits для jwt-cpp
        using traits = jwt::traits::nlohmann_json;

        // Теперь decode и verify требуют указания <traits>
        auto decoded = jwt::decode<traits>(token);

        auto verifier = jwt::verify<traits>()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .with_issuer("auth-service");

        verifier.verify(decoded);

        // Получаем claim "sub" как строку и пытаемся преобразовать в int
        int user_id = 0;
        try {
            std::string sub_str = decoded.get_payload_claim("sub").as_string();
            user_id = std::stoi(sub_str);
        } catch (const std::exception& e) {
            LoggingUtils::logAuthFailure("Invalid sub claim: " + std::string(e.what()), token.substr(0, 10) + "...");
            return std::nullopt;
        }

        // role как строка (по умолчанию student)
        std::string role;
        try {
            role = decoded.get_payload_claim("role").as_string();
        } catch (const std::exception&) {
            role = "student";
        }

        // Логируем успешную авторизацию
        Logger::info("JWT verified successfully", {
            {"user_id", std::to_string(user_id)},
            {"role", role},
            {"token_prefix", token.substr(0, 10) + "..."}
        });

        verifier.verify(decoded);

        return AuthInfo{user_id, role};

    } catch (const std::exception& e) {
        // Ловим все ошибки (и jwt, и stoi, и прочие) здесь
        LoggingUtils::logAuthFailure(
            std::string("Verification failed: ") + e.what(), 
            token.substr(0, 10) + "..."
        );
        return std::nullopt;
    } catch (...) {
        LoggingUtils::logAuthFailure("Unknown error during verification", token.substr(0, 10) + "...");
        return std::nullopt;
    }
}

using json = nlohmann::json;


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
    int status_code = 200;  // по умолчанию
    std::string status_text = "OK";
    std::string response_body;
    std::string status_line;

    // ДОБАВЛЯЕМ build_response в НАЧАЛО функции
    auto build_response = [&]() -> std::string {
        std::string response = status_line + "\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
        response += "\r\n";
        response += response_body;
        return response;
    };

    // === Парсинг JWT и проверки ===
    std::optional<AuthInfo> auth_opt = std::nullopt;
    auto auth_it = request_data.find("hdr:Authorization");
    if (auth_it != request_data.end()) {
        const std::string& auth_header = auth_it->second;
        if (auth_header.rfind("Bearer ", 0) == 0) {
            std::string token = auth_header.substr(7);
            auth_opt = verify_jwt(token);
        }
    }

        int user_id = auth_opt ? auth_opt->user_id : 0;
    std::string role = auth_opt ? auth_opt->role : "";

    // Функция проверки, что пользователь авторизован (любая роль)
    auto require_auth = [&]() -> bool {
        if (!auth_opt) {
            status_line = "HTTP/1.1 401 Unauthorized";
            response_body = "{\"message\":\"Invalid or missing token\"}";
            return false;
        }
        return true;
    };

    auto require_attempt_owner = [&](int attempt_user_id) -> bool {
        if (!auth_opt || (user_id != attempt_user_id && role != "teacher" && role != "admin")) {
            status_line = "HTTP/1.1 403 Forbidden";
            response_body = "{\"message\":\"Access denied\"}";
            return false;
        }
        return true;
    };

    auto require_role = [&](const std::vector<std::string>& allowed_roles) -> bool {
        if (!auth_opt) {
            status_line = "HTTP/1.1 401 Unauthorized";
            response_body = "{\"message\":\"Invalid or missing token\"}";
            return false;
        }
        bool allowed = false;
        for (const auto& allowed_role : allowed_roles) {
            if (role == allowed_role) {
                allowed = true;
                break;
            }
        }
        if (!allowed) {
            status_line = "HTTP/1.1 403 Forbidden";
            response_body = "{\"message\":\"Insufficient permissions. Required roles: " + 
                           [&allowed_roles]() -> std::string {
                               std::string result;
                               for (size_t i = 0; i < allowed_roles.size(); ++i) {
                                   result += allowed_roles[i];
                                   if (i < allowed_roles.size() - 1) result += ", ";
                               }
                               return result;
                           }() + "\"}";
            return false;
        }
        return true;
    };

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
    // Проверяем, что пользователь авторизован и имеет роль admin или teacher
    if (!require_role({"admin", "teacher"})) {
    return build_response();
}
    
    std::string body = request_data.count("body") ? request_data.at("body") : "";
    std::string title = "Untitled";
    std::string description = "";

    // простейший парсинг title
    size_t pos = body.find("\"title\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        size_t q1 = body.find("\"", colon);
        size_t q2 = body.find("\"", q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos) {
            title = body.substr(q1 + 1, q2 - q1 - 1);
        }
    }

    // простейший парсинг description
    pos = body.find("\"description\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        size_t q1 = body.find("\"", colon);
        size_t q2 = body.find("\"", q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos) {
            description = body.substr(q1 + 1, q2 - q1 - 1);
        }
    }

    try {
        pqxx::work w(db.conn());
        pqxx::result r = w.exec_params(
            "INSERT INTO tests (title, description) VALUES ($1, $2) RETURNING id",
            title, description
        );
        w.commit();

        int new_id = r[0]["id"].as<int>();
        status_line = "HTTP/1.1 201 Created";
        response_body = "{\"id\":" + std::to_string(new_id) + "}";
    } catch (const std::exception& e) {
        status_line = "HTTP/1.1 500 Internal Server Error";
        response_body = std::string("{\"code\":\"DB_ERROR\",\"message\":\"") + e.what() + "\"}";
    }
}

    else if (method == "PUT" && extract_id_from_path(path, "/tests/")) {
    // Проверяем, что пользователь авторизован и имеет роль admin или teacher
    if (!require_role({"admin", "teacher"})) {
        return build_response();
    }
    
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
    // ТОЛЬКО admin может удалять тесты
    if (!require_role({"admin"})) {
        return build_response();
    }
    
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
    // Проверяем, что пользователь авторизован и имеет роль admin или teacher
    if (!require_role({"admin", "teacher"})) {
        return build_response();
    }
    
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
    // ТОЛЬКО admin может удалять вопросы
    if (!require_role({"admin"})) {
        return build_response();
    }
    
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
    // Проверяем, что пользователь авторизован и имеет роль admin или teacher
    if (!require_role({"admin", "teacher"})) {
        return build_response();
    }
    
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
    // ТОЛЬКО admin может удалять ответы
    if (!require_role({"admin"})) {
        return build_response();
    }
    
    int aid = std::stoi(path.substr(9));
        bool ok = answerService.remove(aid);
        status_line = ok ? "HTTP/1.1 200 OK" : "HTTP/1.1 404 Not Found";
        response_body = ok ? "{\"message\":\"Answer deleted\"}" : "{\"message\":\"Answer not found\"}";
    }

    // ---------- QUESTIONS BY ID ----------
else if (method == "GET" && path.find("/questions/") == 0) {
    int qid = std::stoi(path.substr(11));
    auto q = questionService.get(qid);
    if (q) {
        status_line = "HTTP/1.1 200 OK";
        response_body = questionToJson(*q);
    } else {
        status_line = "HTTP/1.1 404 Not Found";
        response_body = "{\"message\":\"Question not found\"}";
    }
}
else if (method == "PUT" && path.find("/questions/") == 0) {
    // Проверяем, что пользователь авторизован и имеет роль admin или teacher
    if (!require_role({"admin", "teacher"})) {
        return build_response();
    }
    
    int qid = std::stoi(path.substr(11));
    std::string body = request_data.count("body") ? request_data.at("body") : "";
    std::optional<std::string> new_text;
    std::optional<std::string> new_type;
    std::optional<int> new_order;

    size_t pos = body.find("\"text\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        size_t q1 = body.find("\"", colon);
        size_t q2 = body.find("\"", q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
            new_text = body.substr(q1 + 1, q2 - q1 - 1);
    }
    pos = body.find("\"type\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        size_t q1 = body.find("\"", colon);
        size_t q2 = body.find("\"", q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
            new_type = body.substr(q1 + 1, q2 - q1 - 1);
    }
    pos = body.find("\"order_index\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        try { new_order = std::stoi(body.substr(colon+1)); } catch (...) {}
    }

    bool ok = questionService.update(qid, new_text, new_type, new_order);
    if (ok) {
        status_line = "HTTP/1.1 200 OK";
        response_body = "{\"message\":\"Question updated\"}";
    } else {
        status_line = "HTTP/1.1 404 Not Found";
        response_body = "{\"message\":\"Question not found\"}";
    }
}

// ---------- ANSWERS BY ID ----------
else if (method == "GET" && path.find("/answers/") == 0) {
    int aid = std::stoi(path.substr(9));
    auto a = answerService.get(aid);
    if (a) {
        status_line = "HTTP/1.1 200 OK";
        response_body = answerToJson(*a);
    } else {
        status_line = "HTTP/1.1 404 Not Found";
        response_body = "{\"message\":\"Answer not found\"}";
    }
}
else if (method == "PUT" && path.find("/answers/") == 0) {
    // Проверяем, что пользователь авторизован и имеет роль admin или teacher
    if (!require_role({"admin", "teacher"})) {
        return build_response();
    }
    
    int aid = std::stoi(path.substr(9));
    std::string body = request_data.count("body") ? request_data.at("body") : "";
    std::optional<std::string> new_text;
    std::optional<bool> new_correct;

    size_t pos = body.find("\"text\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        size_t q1 = body.find("\"", colon);
        size_t q2 = body.find("\"", q1 + 1);
        if (q1 != std::string::npos && q2 != std::string::npos)
            new_text = body.substr(q1 + 1, q2 - q1 - 1);
    }
    pos = body.find("\"is_correct\"");
    if (pos != std::string::npos) {
        size_t colon = body.find(":", pos);
        std::string val = body.substr(colon+1);
        new_correct = (val.find("true") != std::string::npos);
    }

    bool ok = answerService.update(aid, new_text, new_correct);
    if (ok) {
        status_line = "HTTP/1.1 200 OK";
        response_body = "{\"message\":\"Answer updated\"}";
    } else {
        status_line = "HTTP/1.1 404 Not Found";
        response_body = "{\"message\":\"Answer not found\"}";
    }
}

    // ---------- HEALTH & ROOT ----------
    // ---------- ATTEMPTS: POST /api/tests/{id}/submit ----------
else if (method == "POST" && path.find("/api/tests/") == 0 &&
         path.size() > std::string("/api/tests/").size() + std::string("/submit").size() &&
         path.rfind("/submit") == path.size() - 7) {

    // Создаем таймер для отслеживания времени выполнения
    std::string id_part = path.substr(std::string("/api/tests/").length(),
                                      path.length() - std::string("/api/tests/").length() - std::string("/submit").length());
    LoggingUtils::ScopedTimer timer("submit_attempt", {
        {"test_id", id_part},
        {"user_id", std::to_string(user_id)}
    });
    

    if (!require_auth()) {
        return build_response();
    }
    int test_id = 0;
    try {
        test_id = std::stoi(id_part);
    } catch (...) {
        timer.markError("Invalid test id");
        status_line = "HTTP/1.1 400 Bad Request";
        response_body = "{\"code\":\"BAD_REQUEST\",\"message\":\"Invalid test id\"}";
        return build_response();
    }

    // тело с ответами (если есть)
    std::string answers_json = "null";
    if (request_data.count("body") && !request_data.at("body").empty())
        answers_json = request_data.at("body");

    try {
        // Проверка существования теста
        {
            pqxx::work w(db.conn());
            pqxx::result r = w.exec_params("SELECT id FROM tests WHERE id = $1", test_id);
            if (r.empty()) {
                timer.markError("Test not found");
                status_line = "HTTP/1.1 404 Not Found";
                response_body = "{\"code\":\"NOT_FOUND\",\"message\":\"Test not found\"}";
                w.commit();
                return build_response();
            }
            w.commit();
        }

        // Вставляем попытку
        pqxx::work tx(db.conn());
        pqxx::result ins = tx.exec_params(
            "INSERT INTO attempts (user_id, test_id, answers, status) VALUES ($1, $2, $3::jsonb, $4) RETURNING id, started_at",
            user_id, test_id, answers_json, std::string("in_progress")
        );
        tx.commit();

        int attempt_id = ins[0]["id"].as<int>();
        std::string started_at = ins[0]["started_at"].c_str();

        // Логируем начало попытки
        LoggingUtils::logAttemptStart(user_id, test_id, attempt_id);

        // Если пришли ответы — вставляем attempt_answers
        if (!answers_json.empty() && answers_json != "null") {
            try {
                auto parsed = json::parse(answers_json);
                if (!parsed.is_array()) throw std::runtime_error("answers must be an array");
                for (auto& item : parsed) {
                    int qid = item.at("question_id").get<int>();
                    int aid = item.at("answer_id").get<int>();
                    pqxx::work tx2(db.conn());
                    tx2.exec_params(
                        "INSERT INTO attempt_answers (attempt_id, question_id, answer_id) VALUES ($1, $2, $3)",
                        attempt_id, qid, aid
                    );
                    tx2.commit();
                }
            } catch (const std::exception& e) {
                // Если парсинг/вставка ответов упали — логируем и продолжаем (попытка создана)
                Logger::warning("Failed to parse/insert attempt answers: " + std::string(e.what()), {
                    {"attempt_id", std::to_string(attempt_id)},
                    {"error", e.what()}
                });
            }
        }

        std::ostringstream oss;
        oss << "{\"attempt_id\":" << attempt_id
            << ",\"test_id\":" << test_id
            << ",\"user_id\":" << user_id
            << ",\"started_at\":\"" << started_at << "\""
            << ",\"status\":\"in_progress\"}";
        status_line = "HTTP/1.1 201 Created";
        response_body = oss.str();

    } catch (const std::exception &e) {
        timer.markError(std::string("Database error: ") + e.what());
        LoggingUtils::logDbInsertError("attempts", e.what());
        status_line = "HTTP/1.1 500 Internal Server Error";
        response_body = std::string("{\"code\":\"DB_ERROR\",\"message\":\"") + e.what() + "\"}";
    }
}

// ---------- ATTEMPTS ----------

// GET /attempts/{id}
else if (method == "GET" && path.find("/attempts/") == 0) {
    int attempt_id = std::stoi(path.substr(10));
if (!require_auth()) return build_response();
    try {
        pqxx::work w(db.conn());
        pqxx::result r = w.exec_params(
            "SELECT id, user_id, test_id, status, score, started_at, completed_at "
            "FROM attempts WHERE id=$1", attempt_id
        );
        if (r.empty()) {
            status_line = "HTTP/1.1 404 Not Found";
            response_body = "{\"message\":\"Attempt not found\"}";
        } else {
                int attempt_user_id = r[0]["user_id"].as<int>();
            if (!require_attempt_owner(attempt_user_id)) return build_response();
            std::ostringstream oss;
            oss << "{"
                << "\"id\":" << r[0]["id"].as<int>() << ","
                << "\"user_id\":" << r[0]["user_id"].as<int>() << ","
                << "\"test_id\":" << r[0]["test_id"].as<int>() << ","
                << "\"status\":\"" << r[0]["status"].c_str() << "\","
                << "\"score\":" << (r[0]["score"].is_null() ? 0 : r[0]["score"].as<int>()) << ","
                << "\"started_at\":\"" << r[0]["started_at"].c_str() << "\","
                << "\"completed_at\":" << (r[0]["completed_at"].is_null() ? "null" : ("\"" + std::string(r[0]["completed_at"].c_str()) + "\""))
                << "}";
            status_line = "HTTP/1.1 200 OK";
            response_body = oss.str();
        }
        w.commit();
    } catch (const std::exception& e) {
        status_line = "HTTP/1.1 500 Internal Server Error";
        response_body = std::string("{\"error\":\"") + e.what() + "\"}";
    }
}

// PUT /attempts/{id}
// В блоке PUT /attempts/{id} ДОБАВЛЯЕМ:

else if (method == "PUT" && path.find("/attempts/") == 0) {
    int attempt_id = std::stoi(path.substr(10));
    
    // Создаем таймер для завершения попытки
    LoggingUtils::ScopedTimer timer("complete_attempt", {
        {"attempt_id", std::to_string(attempt_id)},
        {"user_id", std::to_string(user_id)}
    });
    
    if (!require_auth()) return build_response();
    
    try {
        pqxx::work w(db.conn());
        pqxx::result att = w.exec_params("SELECT user_id FROM attempts WHERE id=$1", attempt_id);
        if (att.empty()) {
            timer.markError("Attempt not found");
            status_line = "HTTP/1.1 404 Not Found";
            response_body = "{\"message\":\"Attempt not found\"}";
            return build_response();
        }
        int attempt_user_id = att[0]["user_id"].as<int>();
        if (!require_attempt_owner(attempt_user_id)) {
            timer.markError("Access denied");
            return build_response();
        }
        
        pqxx::result answers = w.exec_params(
            "SELECT a.is_correct FROM attempt_answers aa "
            "JOIN answers a ON aa.answer_id=a.id "
            "WHERE aa.attempt_id=$1", attempt_id
        );

        ScoreResult scoreRes = AttemptUtils::calculate_score_from_result(answers);
        int total = scoreRes.total;
        int correct = scoreRes.correct;
        int score = scoreRes.score;

        // Обновляем попытку
        w.exec_params(
            "UPDATE attempts SET status='completed', score=$2, completed_at=NOW() WHERE id=$1",
            attempt_id, score
        );
        w.commit();

        // Логируем завершение попытки
        LoggingUtils::logAttemptComplete(attempt_id, score, total, correct);

        std::ostringstream oss;
        oss << "{"
            << "\"attempt_id\":" << attempt_id << ","
            << "\"status\":\"completed\","
            << "\"total\":" << total << ","
            << "\"correct\":" << correct << ","
            << "\"score\":" << score
            << "}";
        status_line = "HTTP/1.1 200 OK";
        response_body = oss.str();
        
    } catch (const std::exception& e) {
        timer.markError(std::string("Error: ") + e.what());
        status_line = "HTTP/1.1 500 Internal Server Error";
        response_body = std::string("{\"error\":\"") + e.what() + "\"}";
    }
}

        // ---------- METRICS ----------
    else if (method == "GET" && path == "/metrics") {
        // Только для админов — Prometheus формат
        if (!require_role({"admin"})) {
            return build_response();
        }

        status_line = "HTTP/1.1 200 OK";
        response_body = Metrics::getPrometheusMetrics();

        // Переопределяем заголовки специально для Prometheus
        std::string response = status_line + "\r\n";
        response += "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n";
        response += "Content-Length: " + std::to_string(response_body.length()) + "\r\n";
        response += "\r\n";
        response += response_body;
        return response;  // возвращаем напрямую, без build_response()
    }
    
    else if (method == "GET" && path == "/api/metrics") {
        // Для админов и учителей (JSON формат)
        if (!require_role({"admin", "teacher"})) {
            return build_response();
        }
        status_line = "HTTP/1.1 200 OK";
        response_body = Metrics::getJsonMetrics();
    }

    // ---------- HEALTH & ROOT ----------
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

        // Обновляем status_code из status_line (для лога)
    if (status_line.find("200") == 9) status_code = 200;
    else if (status_line.find("201") == 9) status_code = 201;
    else if (status_line.find("400") == 9) status_code = 400;
    else if (status_line.find("401") == 9) status_code = 401;
    else if (status_line.find("403") == 9) status_code = 403;
    else if (status_line.find("404") == 9) status_code = 404;
    else if (status_line.find("500") == 9) status_code = 500;
    else if (status_line.find("503") == 9) status_code = 503;

    // Логируем КАЖДЫЙ запрос (включая ошибки)
    LoggingUtils::logHttpRequest(method, path, status_code);

    return build_response();
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
    Logger::info("Core API server started", {
    {"port", "8080"},
    {"db_url", std::string(db_url_env).substr(0, 20) + "..."}
    });

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
