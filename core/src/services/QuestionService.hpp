#pragma once
#include "../database/Database.hpp"
#include "../models/Question.hpp"
#include <vector>
#include <optional>
#include <string>

class QuestionService {
public:
  explicit QuestionService(Database& db);

  // CRUD
  std::vector<Question> list_by_test(int test_id);
  std::optional<Question> get(int id);
  int create(int test_id, const std::string& text, const std::string& type, int order_index);
  bool update(int id,
              const std::optional<std::string>& text,
              const std::optional<std::string>& type,
              const std::optional<int>& order_index);
  bool remove(int id);

private:
  Database& db_;
};
