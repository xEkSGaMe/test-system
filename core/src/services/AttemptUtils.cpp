#include "AttemptUtils.hpp"

ScoreResult calculate_score_from_result(const pqxx::result& answers) {
    int total = answers.size();
    int correct = 0;
    for (const auto& row : answers) {
        try {
            if (row["is_correct"].as<bool>()) ++correct;
        } catch (...) { /* игнорируем некорректные строки */ }
    }
    int score = (total == 0) ? 0 : (100 * correct / total);
    return { total, correct, score };
}

ScoreResult calculate_score_from_vector(const std::vector<bool>& answers) {
    int total = static_cast<int>(answers.size());
    int correct = 0;
    for (bool v : answers) if (v) ++correct;
    int score = (total == 0) ? 0 : (100 * correct / total);
    return { total, correct, score };
}
