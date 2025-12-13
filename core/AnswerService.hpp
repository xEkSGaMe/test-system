#pragma once
#include "../database/Database.hpp"
#include "../models/Answer.hpp"
#include <vector>
#include <optional>
#include <string>

class AnswerService {
public:
  explicit AnswerService(Database& db);

  // CRUD
  std::vector<Answer> list_by_question(int question_id);
  std::optional<Answer> get(int id);
  int create(int question_id, const std::string& text, bool is_correct);
  bool update(int id,
              const std::optional<std::string>& text,
              const std::optional<bool>& is_correct);
  bool remove(int id);

private:
  Database& db_;
};
