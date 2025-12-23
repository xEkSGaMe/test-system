#include "AttemptUtils.hpp"
#include "../Logger.hpp"

ScoreResult AttemptUtils::finalizeAttempt(Database& db, int attempt_id) {
    pqxx::work tx{db.connection()};

    // 1. Считаем правильные ответы пользователя напрямую через SQL JOIN
    // Мы сравниваем id выбранного ответа пользователем с тем, где is_correct = true для этого вопроса
    std::string score_query = 
        "SELECT "
        "  COUNT(*) as total_answered, "
        "  SUM(CASE WHEN a.is_correct = TRUE THEN 1 ELSE 0 END) as correct_count "
        "FROM user_answers ua "
        "JOIN answers a ON ua.selected_answer_id = a.id "
        "WHERE ua.attempt_id = " + tx.quote(attempt_id) + ";";

    auto r = tx.exec(score_query);
    
    int answered = r[0]["total_answered"].as<int>();
    int correct = r[0]["correct_count"].is_null() ? 0 : r[0]["correct_count"].as<int>();
    
    // 2. Узнаем общее количество вопросов в тесте, чтобы расчет был честным
    auto r_total = tx.exec(
        "SELECT COUNT(*) FROM questions q "
        "JOIN attempts att ON q.test_id = att.test_id "
        "WHERE att.id = " + tx.quote(attempt_id)
    );
    int total_questions = r_total[0][0].as<int>();

    int final_score = (total_questions == 0) ? 0 : (100 * correct / total_questions);

    // 3. Записываем результат и закрываем попытку
    tx.exec0(
        "UPDATE attempts SET "
        "score = " + tx.quote(final_score) + ", "
        "is_finished = TRUE, "
        "finished_at = NOW() "
        "WHERE id = " + tx.quote(attempt_id)
    );

    tx.commit();
    
    Logger::info("Attempt finalized", {
        {"attempt_id", std::to_string(attempt_id)},
        {"score", std::to_string(final_score)}
    });

    return { total_questions, correct, final_score };
}

// Твоя старая функция (доработаем её, чтобы не падала)
ScoreResult AttemptUtils::calculate_score_from_result(const pqxx::result& answers) {
    int total = answers.size();
    int correct = 0;
    for (const auto& row : answers) {
        if (!row["is_correct"].is_null() && row["is_correct"].as<bool>()) 
            ++correct;
    }
    return { total, correct, (total == 0) ? 0 : (100 * correct / total) };
}