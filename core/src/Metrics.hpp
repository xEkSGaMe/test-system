// Metrics.hpp
#ifndef METRICS_HPP
#define METRICS_HPP

#include <atomic>
#include <map>
#include <string>
#include <chrono>
#include <mutex>
#include <sstream>

class Metrics {
private:
    // Счетчики
    static std::atomic<int> attempts_started;
    static std::atomic<int> attempts_completed;
    static std::atomic<int> attempts_failed;
    static std::atomic<int> db_insert_errors;
    static std::atomic<int> auth_failures;
    static std::atomic<int> requests_total;
    static std::atomic<int> requests_by_method[10]; // GET, POST, PUT, DELETE и т.д.
    
    // Время выполнения операций
    static std::mutex timing_mutex;
    static std::map<std::string, std::chrono::microseconds> operation_timings;
    static std::map<std::string, int> operation_counts;
    
public:
    // Методы для инкрементации счетчиков
    static void incrementAttemptsStarted() { attempts_started++; requests_total++; }
    static void incrementAttemptsCompleted() { attempts_completed++; }
    static void incrementAttemptsFailed() { attempts_failed++; }
    static void incrementDbInsertErrors() { db_insert_errors++; }
    static void incrementAuthFailures() { auth_failures++; }
    
    static void incrementRequestByMethod(const std::string& method) {
        if (method == "GET") requests_by_method[0]++;
        else if (method == "POST") requests_by_method[1]++;
        else if (method == "PUT") requests_by_method[2]++;
        else if (method == "DELETE") requests_by_method[3]++;
        else if (method == "PATCH") requests_by_method[4]++;
        else requests_by_method[5]++;
    }
    
    // Метод для записи времени выполнения операции
    static void recordOperationTime(const std::string& operation_name, 
                                   std::chrono::microseconds duration) {
        std::lock_guard<std::mutex> lock(timing_mutex);
        operation_timings[operation_name] += duration;
        operation_counts[operation_name]++;
    }
    
    // Получение всех метрик в формате Prometheus
    static std::string getPrometheusMetrics() {
        std::stringstream ss;
        
        ss << "# HELP test_system_attempts_started Total number of test attempts started\n";
        ss << "# TYPE test_system_attempts_started counter\n";
        ss << "test_system_attempts_started " << attempts_started.load() << "\n\n";
        
        ss << "# HELP test_system_attempts_completed Total number of test attempts completed\n";
        ss << "# TYPE test_system_attempts_completed counter\n";
        ss << "test_system_attempts_completed " << attempts_completed.load() << "\n\n";
        
        ss << "# HELP test_system_attempts_failed Total number of failed test attempts\n";
        ss << "# TYPE test_system_attempts_failed counter\n";
        ss << "test_system_attempts_failed " << attempts_failed.load() << "\n\n";
        
        ss << "# HELP test_system_db_insert_errors Total number of database insertion errors\n";
        ss << "# TYPE test_system_db_insert_errors counter\n";
        ss << "test_system_db_insert_errors " << db_insert_errors.load() << "\n\n";
        
        ss << "# HELP test_system_auth_failures Total number of authentication failures\n";
        ss << "# TYPE test_system_auth_failures counter\n";
        ss << "test_system_auth_failures " << auth_failures.load() << "\n\n";
        
        ss << "# HELP test_system_requests_total Total number of HTTP requests\n";
        ss << "# TYPE test_system_requests_total counter\n";
        ss << "test_system_requests_total " << requests_total.load() << "\n\n";
        
        // Метрики по методам
        ss << "# HELP test_system_requests_by_method HTTP requests by method\n";
        ss << "# TYPE test_system_requests_by_method counter\n";
        ss << "test_system_requests_by_method{method=\"GET\"} " << requests_by_method[0].load() << "\n";
        ss << "test_system_requests_by_method{method=\"POST\"} " << requests_by_method[1].load() << "\n";
        ss << "test_system_requests_by_method{method=\"PUT\"} " << requests_by_method[2].load() << "\n";
        ss << "test_system_requests_by_method{method=\"DELETE\"} " << requests_by_method[3].load() << "\n";
        ss << "test_system_requests_by_method{method=\"PATCH\"} " << requests_by_method[4].load() << "\n";
        ss << "test_system_requests_by_method{method=\"OTHER\"} " << requests_by_method[5].load() << "\n\n";
        
        // Среднее время выполнения операций
        {
            std::lock_guard<std::mutex> lock(timing_mutex);
            for (const auto& [op_name, total_time] : operation_timings) {
                int count = operation_counts[op_name];
                if (count > 0) {
                    double avg_ms = total_time.count() / 1000.0 / count;
                    ss << "# HELP test_system_operation_duration_seconds Average duration of operation\n";
                    ss << "# TYPE test_system_operation_duration_seconds gauge\n";
                    ss << "test_system_operation_duration_seconds{operation=\"" 
                       << op_name << "\"} " << avg_ms << "\n\n";
                }
            }
        }
        
        return ss.str();
    }
    
    // Получение метрик в формате JSON (для API)
    static std::string getJsonMetrics() {
        std::stringstream ss;
        ss << "{";
        ss << "\"attempts_started\":" << attempts_started.load() << ",";
        ss << "\"attempts_completed\":" << attempts_completed.load() << ",";
        ss << "\"attempts_failed\":" << attempts_failed.load() << ",";
        ss << "\"db_insert_errors\":" << db_insert_errors.load() << ",";
        ss << "\"auth_failures\":" << auth_failures.load() << ",";
        ss << "\"requests_total\":" << requests_total.load() << ",";
        
        ss << "\"requests_by_method\":{";
        ss << "\"GET\":" << requests_by_method[0].load() << ",";
        ss << "\"POST\":" << requests_by_method[1].load() << ",";
        ss << "\"PUT\":" << requests_by_method[2].load() << ",";
        ss << "\"DELETE\":" << requests_by_method[3].load() << ",";
        ss << "\"PATCH\":" << requests_by_method[4].load() << ",";
        ss << "\"OTHER\":" << requests_by_method[5].load();
        ss << "}";
        
        ss << "}";
        return ss.str();
    }
    
    // Сброс метрик (для тестирования)
    static void reset() {
        attempts_started = 0;
        attempts_completed = 0;
        attempts_failed = 0;
        db_insert_errors = 0;
        auth_failures = 0;
        requests_total = 0;
        
        for (int i = 0; i < 10; i++) {
            requests_by_method[i] = 0;
        }
        
        std::lock_guard<std::mutex> lock(timing_mutex);
        operation_timings.clear();
        operation_counts.clear();
    }
};

// Инициализация статических членов
std::atomic<int> Metrics::attempts_started(0);
std::atomic<int> Metrics::attempts_completed(0);
std::atomic<int> Metrics::attempts_failed(0);
std::atomic<int> Metrics::db_insert_errors(0);
std::atomic<int> Metrics::auth_failures(0);
std::atomic<int> Metrics::requests_total(0);
std::atomic<int> Metrics::requests_by_method[10] = {0};

std::mutex Metrics::timing_mutex;
std::map<std::string, std::chrono::microseconds> Metrics::operation_timings;
std::map<std::string, int> Metrics::operation_counts;

#endif