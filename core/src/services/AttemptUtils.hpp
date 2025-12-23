#pragma once
#include <pqxx/pqxx>
#include <vector>
#include "../database/Database.hpp"

struct ScoreResult {
    int total;
    int correct;
    int score; // 0..100
};

class AttemptUtils {
public:
    // Главный метод: считает баллы в БД и закрывает попытку
    static ScoreResult finalizeAttempt(Database& db, int attempt_id);
    
    // Оставляем твои старые методы для совместимости
    static ScoreResult calculate_score_from_result(const pqxx::result& answers);
};