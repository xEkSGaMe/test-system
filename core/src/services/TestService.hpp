#pragma once
#include "../database/Database.hpp"
#include "../models/Test.hpp"
#include <vector>
#include <optional>
#include <string>

class TestService {
public:
  explicit TestService(Database& db);

  std::vector<Test> list();
  std::optional<Test> get(int id);
  int create(const std::string& title, const std::optional<std::string>& description);
  bool update(int id, const std::optional<std::string>& title,
              const std::optional<std::string>& description,
              const std::optional<bool>& is_published);
  bool remove(int id);

private:
  Database& db_;
};
