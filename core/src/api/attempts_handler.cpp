// test-system/core-api/src/api/attempts_handler.cpp
#include "database/Database.h"
#include "middleware/JwtMiddleware.h"
#include <nlohmann/json.hpp>
#include <string>
#include <iostream>

// Адаптируй типы Request/Response под твой серверный код.
// Ниже используются псевдотипы: Request имеет методы getHeader, pathParam, body,
// Response имеет методы sendJson(status, string).
// Заменяй на реальные типы/вызовы в проекте.

struct Request {
  std::string getHeader(const std::string &name) const; // реализовано в сервере
  std::string pathParam(const std::string &name) const;
  std::string body() const;
};

struct Response {
  void sendJson(int status, const std::string &body) const;
};

void handleSubmitAttempt(const Request &req, Response &res, Database &db) {
  // 1. Авторизация
  std::string authHeader = req.getHeader("Authorization");
  if (authHeader.empty()) {
    res.sendJson(401, R"({"code":"UNAUTHORIZED","message":"Missing Authorization header"})");
    return;
  }
  std::string token = extractBearer(authHeader);
  if (token.empty()) {
    res.sendJson(401, R"({"code":"UNAUTHORIZED","message":"Invalid Authorization header"})");
    return;
  }
  auto claimsOpt = validateJwt(token);
  if (!claimsOpt) {
    res.sendJson(401, R"({"code":"UNAUTHORIZED","message":"Invalid or expired token"})");
    return;
  }
  JwtClaims claims = *claimsOpt;
  // Разрешаем студентам; при необходимости расширь роли
  if (claims.role != "student") {
    res.sendJson(403, R"({"code":"FORBIDDEN","message":"Role not allowed"})");
    return;
  }

  // 2. Параметр test_id
  int64_t test_id = 0;
  try {
    test_id = std::stoll(req.pathParam("id"));
  } catch (...) {
    res.sendJson(400, R"({"code":"BAD_REQUEST","message":"Invalid test id"})");
    return;
  }

  // 3. Проверка существования теста
  try {
    pqxx::work w(db.conn());
    pqxx::result r = w.prepared("select_test_exists")(test_id).exec();
    if (r.empty()) {
      res.sendJson(404, R"({"code":"NOT_FOUND","message":"Test not found"})");
      return;
    }
  } catch (const std::exception &e) {
    nlohmann::json err = { {"code","DB_ERROR"}, {"message", e.what()} };
    res.sendJson(500, err.dump());
    return;
  }

  // 4. Опциональная проверка лимита активных попыток (1 активная)
  try {
    pqxx::work w(db.conn());
    pqxx::result cnt = w.prepared("count_inprogress_attempts")(claims.user_id)(test_id).exec();
    int active = cnt[0][0].as<int>();
    if (active >= 1) {
      res.sendJson(429, R"({"code":"TOO_MANY_ATTEMPTS","message":"Active attempt already exists"})");
      return;
    }
  } catch (...) {
    // не фатально — логируем и продолжаем
  }

  // 5. Парсинг тела (опционально initial_answers, client_attempt_id)
  nlohmann::json bodyJson;
  std::string bodyStr = req.body();
  if (!bodyStr.empty()) {
    bodyJson = nlohmann::json::parse(bodyStr, nullptr, false);
    if (bodyJson.is_discarded()) {
      res.sendJson(400, R"({"code":"BAD_REQUEST","message":"Invalid JSON body"})");
      return;
    }
  }

  std::string answers_json = "null";
  std::string client_attempt_id = "";
  if (!bodyJson.is_null()) {
    if (bodyJson.contains("initial_answers")) answers_json = bodyJson["initial_answers"].dump();
    if (bodyJson.contains("client_attempt_id")) client_attempt_id = bodyJson["client_attempt_id"].get<std::string>();
  }

  // 6. Вставка попытки в транзакции
  try {
    pqxx::work tx(db.conn());
    pqxx::result ins = tx.prepared("insert_attempt")
      (claims.user_id)
      (test_id)
      (answers_json)
      ("in_progress")
      (client_attempt_id)
      .exec();

    int64_t attempt_id = ins[0][0].as<int64_t>();
    std::string started_at = ins[0][1].as<std::string>();
    tx.commit();

    nlohmann::json resp = {
      {"attempt_id", attempt_id},
      {"test_id", test_id},
      {"user_id", claims.user_id},
      {"started_at", started_at},
      {"status", "in_progress"}
    };
    res.sendJson(201, resp.dump());
    return;
  } catch (const std::exception &e) {
    nlohmann::json err = { {"code","INTERNAL_ERROR"}, {"message", e.what()} };
    res.sendJson(500, err.dump());
    return;
  }
}
