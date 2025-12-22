#pragma once
#include <pqxx/pqxx>
#include <vector>

struct ScoreResult {
    int total;
    int correct;
    int score; // 0..100
};

ScoreResult calculate_score_from_result(const pqxx::result& answers);
ScoreResult calculate_score_from_vector(const std::vector<bool>& answers);
