#include "QuestionService.hpp"
#include <pqxx/pqxx>

QuestionService::QuestionService(Database& db) : db_(db) {}

std::vector<Question> QuestionService::list_by_test(int test_id) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "SELECT id, test_id, text, type, order_index FROM questions WHERE test_id=$1 ORDER BY order_index ASC, id ASC",
    test_id
  );
  std::vector<Question> out;
  out.reserve(r.size());
  for (const auto& row : r) {
    Question q {
      row["id"].as<int>(),
      row["test_id"].as<int>(),
      row["text"].as<std::string>(),
      row["type"].as<std::string>(),
      row["order_index"].as<int>()
    };
    out.push_back(std::move(q));
  }
  tx.commit();
  return out;
}

std::optional<Question> QuestionService::get(int id) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "SELECT id, test_id, text, type, order_index FROM questions WHERE id=$1 LIMIT 1",
    id
  );
  if (r.empty()) return std::nullopt;
  const auto& row = r[0];
  Question q {
    row["id"].as<int>(),
    row["test_id"].as<int>(),
    row["text"].as<std::string>(),
    row["type"].as<std::string>(),
    row["order_index"].as<int>()
  };
  tx.commit();
  return q;
}

int QuestionService::create(int test_id, const std::string& text, const std::string& type, int order_index) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "INSERT INTO questions (test_id, text, type, order_index) VALUES ($1,$2,$3,$4) RETURNING id",
    test_id, text, type, order_index
  );
  int id = r[0]["id"].as<int>();
  tx.commit();
  return id;
}

bool QuestionService::update(int id,
                             const std::optional<std::string>& text,
                             const std::optional<std::string>& type,
                             const std::optional<int>& order_index) {
  pqxx::work tx{db_.connection()};
  std::string q = "UPDATE questions SET ";
  bool first = true;
  auto addField = [&](const std::string& f) {
    if (!first) q += ", ";
    q += f;
    first = false;
  };

  if (text.has_value())        addField("text = " + tx.quote(*text));
  if (type.has_value())        addField("type = " + tx.quote(*type));
  if (order_index.has_value()) addField("order_index = " + tx.quote(*order_index));
  q += " WHERE id = " + tx.quote(id);

  if (first) return false; // ничего не обновили
  auto res = tx.exec(q);
  tx.commit();
  return res.affected_rows() > 0;
}

bool QuestionService::remove(int id) {
  pqxx::work tx{db_.connection()};
  auto res = tx.exec_params("DELETE FROM questions WHERE id=$1", id);
  tx.commit();
  return res.affected_rows() > 0;
}
