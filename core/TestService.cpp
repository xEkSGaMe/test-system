#include "TestService.hpp"
#include <pqxx/pqxx>

TestService::TestService(Database& db) : db_(db) {}

std::vector<Test> TestService::list() {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec("SELECT id, title, description, author_id, is_published FROM tests ORDER BY id ASC");
  std::vector<Test> out;
  out.reserve(r.size());
  for (const auto& row : r) {
    Test t;
    t.id = row["id"].as<int>();
    t.title = row["title"].as<std::string>();
    t.description = row["description"].is_null() ? std::optional<std::string>{} : std::make_optional(row["description"].c_str());
    t.author_id = row["author_id"].is_null() ? std::optional<int>{} : std::make_optional(row["author_id"].as<int>());
    t.is_published = row["is_published"].as<bool>();
    out.push_back(std::move(t));
  }
  tx.commit();
  return out;
}

std::optional<Test> TestService::get(int id) {
  pqxx::work tx{db_.connection()};
  auto r = tx.exec_params(
    "SELECT id, title, description, author_id, is_published FROM tests WHERE id = $1 LIMIT 1", id
  );
  if (r.empty()) return std::nullopt;
  const auto& row = r[0];
  Test t;
  t.id = row["id"].as<int>();
  t.title = row["title"].as<std::string>();
  t.description = row["description"].is_null() ? std::optional<std::string>{} : std::make_optional(row["description"].c_str());
  t.author_id = row["author_id"].is_null() ? std::optional<int>{} : std::make_optional(row["author_id"].as<int>());
  t.is_published = row["is_published"].as<bool>();
  tx.commit();
  return t;
}

int TestService::create(const std::string& title, const std::optional<std::string>& description) {
  pqxx::work tx{db_.connection()};
  // Для NULL используем nullptr во втором параметре
  auto r = tx.exec_params(
    "INSERT INTO tests (title, description) VALUES ($1, $2) RETURNING id",
    title,
    description.has_value() ? *description : nullptr
  );
  int id = r[0]["id"].as<int>();
  tx.commit();
  return id;
}

bool TestService::update(int id, const std::optional<std::string>& title,
                         const std::optional<std::string>& description,
                         const std::optional<bool>& is_published) {
  pqxx::work tx{db_.connection()};
  std::string q = "UPDATE tests SET ";
  bool first = true;
  auto addField = [&](const std::string& f) {
    if (!first) q += ", ";
    q += f;
    first = false;
  };

  if (title.has_value())        addField("title = " + tx.quote(*title));
  if (description.has_value())  addField("description = " + tx.quote(*description));
  if (is_published.has_value()) addField("is_published = " + std::string(*is_published ? "TRUE" : "FALSE"));
  q += " WHERE id = " + tx.quote(id);

  if (first) return false; // ничего не обновили
  auto res = tx.exec(q);
  tx.commit();
  return res.affected_rows() > 0;
}

bool TestService::remove(int id) {
  pqxx::work tx{db_.connection()};
  auto res = tx.exec_params("DELETE FROM tests WHERE id = $1", id);
  tx.commit();
  return res.affected_rows() > 0;
}
