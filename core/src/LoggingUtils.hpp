// LoggingUtils.hpp
#ifndef LOGGING_UTILS_HPP
#define LOGGING_UTILS_HPP

#include "Logger.hpp"
#include "Metrics.hpp"
#include <chrono>
#include <map>

class LoggingUtils {
public:
    // Обертка для логирования начала операции с таймингом
    class ScopedTimer {
    private:
        std::string operation_name;
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
        std::map<std::string, std::string> context;
        
    public:
        ScopedTimer(const std::string& op_name, const std::map<std::string, std::string>& ctx = {})
            : operation_name(op_name), context(ctx) {
            start_time = std::chrono::high_resolution_clock::now();
            context["operation"] = operation_name;
            context["status"] = "started";
            Logger::info("Operation started: " + operation_name, context);
        }
        
        ~ScopedTimer() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            
            context["duration_ms"] = std::to_string(duration.count() / 1000.0);
            context["status"] = "completed";
            
            Logger::info("Operation completed: " + operation_name + " (" + 
                        std::to_string(duration.count() / 1000.0) + " ms)", context);
            
            // Записываем метрику времени выполнения
            Metrics::recordOperationTime(operation_name, duration);
        }
        
        // Метод для отметки ошибки
        void markError(const std::string& error_msg) {
            context["error"] = error_msg;
            context["status"] = "failed";
            Logger::error("Operation failed: " + operation_name + " - " + error_msg, context);
        }
    };
    
    // Логирование попытки прохождения теста
    static void logAttemptStart(int user_id, int test_id, int attempt_id) {
        std::map<std::string, std::string> context = {
            {"user_id", std::to_string(user_id)},
            {"test_id", std::to_string(test_id)},
            {"attempt_id", std::to_string(attempt_id)},
            {"event", "attempt_started"}
        };
        
        Logger::info("Test attempt started", context);
        Metrics::incrementAttemptsStarted();
    }
    
    // Логирование завершения попытки
    static void logAttemptComplete(int attempt_id, int score, int total, int correct) {
        std::map<std::string, std::string> context = {
            {"attempt_id", std::to_string(attempt_id)},
            {"score", std::to_string(score)},
            {"total_questions", std::to_string(total)},
            {"correct_answers", std::to_string(correct)},
            {"event", "attempt_completed"}
        };
        
        Logger::info("Test attempt completed. Score: " + std::to_string(score) + 
                    "/" + std::to_string(total), context);
        Metrics::incrementAttemptsCompleted();
    }
    
    // Логирование ошибки вставки в БД
    static void logDbInsertError(const std::string& table, const std::string& error) {
        std::map<std::string, std::string> context = {
            {"table", table},
            {"error", error},
            {"event", "db_insert_error"}
        };
        
        Logger::error("Database insert error in table " + table + ": " + error, context);
        Metrics::incrementDbInsertErrors();
    }
    
    // Логирование ошибки авторизации
    static void logAuthFailure(const std::string& reason, const std::string& token = "") {
        std::map<std::string, std::string> context = {
            {"reason", reason},
            {"event", "auth_failure"}
        };
        
        if (!token.empty()) {
            // Не логируем полный токен из соображений безопасности
            context["token_length"] = std::to_string(token.length());
        }
        
        Logger::warning("Authentication failed: " + reason, context);
        Metrics::incrementAuthFailures();
    }
    
    // Логирование HTTP запроса
    static void logHttpRequest(const std::string& method, const std::string& path, 
                              int status_code, const std::string& client_ip = "") {
        std::map<std::string, std::string> context = {
            {"method", method},
            {"path", path},
            {"status_code", std::to_string(status_code)},
            {"event", "http_request"}
        };
        
        if (!client_ip.empty()) {
            context["client_ip"] = client_ip;
        }
        
        Logger::info(method + " " + path + " -> " + std::to_string(status_code), context);
        Metrics::incrementRequestByMethod(method);
    }
};

#endif