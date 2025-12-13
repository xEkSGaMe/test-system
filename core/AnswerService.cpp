#include "AnswerService.hpp"
#include <pqxx/pqxx>

AnswerService::AnswerService(Database& db) : db_(db) {}

std::vector<Answer> AnswerService::list_by_question(int question_id) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "SELECT id, question_id, text, is_correct FROM answers WHERE question_id=$1 ORDER BY id ASC",
    question_id
  );
  std::vector<Answer> out;
  out.reserve(r.size());
  for (const auto& row : r) {
    Answer a {
      row["id"].as<int>(),
      row["question_id"].as<int>(),
      row["text"].as<std::string>(),
      row["is_correct"].as<bool>()
    };
    out.push_back(std::move(a));
  }
  tx.commit();
  return out;
}

std::optional<Answer> AnswerService::get(int id) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "SELECT id, question_id, text, is_correct FROM answers WHERE id=$1 LIMIT 1",
    id
  );
  if (r.empty()) return std::nullopt;
  const auto& row = r[0];
  Answer a {
    row["id"].as<int>(),
    row["question_id"].as<int>(),
    row["text"].as<std::string>(),
    row["is_correct"].as<bool>()
  };
  tx.commit();
  return a;
}

int AnswerService::create(int question_id, const std::string& text, bool is_correct) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "INSERT INTO answers (question_id, text, is_correct) VALUES ($1,$2,$3) RETURNING id",
    question_id, text, is_correct
  );
  int id = r[0]["id"].as<int>();
  tx.commit();
  return id;
}

bool AnswerService::update(int id,
                           const std::optional<std::string>& text,
                           const std::optional<bool>& is_correct) {
  pqxx::work tx{db_.connection()};
  std::string q = "UPDATE answers SET ";
  bool first = true;
  auto addField = [&](const std::string& f) {
    if (!first) q += ", ";
    q += f;
    first = false;
  };

  if (text.has_value())       addField("text = " + tx.quote(*text));
  if (is_correct.has_value()) addField(std::string("is_correct = ") + (*is_correct ? "TRUE" : "FALSE"));
  q += " WHERE id = " + tx.quote(id);

  if (first) return false;
  auto res = tx.exec(q);
  tx.commit();
  return res.affected_rows() > 0;
}

bool AnswerService::remove(int id) {
  pqxx::work tx{db_.connection()};
  auto res = tx.exec_params("DELETE FROM answers WHERE id=$1", id);
  tx.commit();
  return res.affected_rows() > 0;
}
