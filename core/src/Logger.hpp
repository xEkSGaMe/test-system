// Logger.hpp
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <map>

class Logger {
public:
    enum Level {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    static void log(Level level, const std::string& message, const std::map<std::string, std::string>& context = {}) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now;
        
        // Безопасное получение локального времени
        #ifdef _WIN32
            localtime_s(&tm_now, &time_t_now);
        #else
            localtime_r(&time_t_now, &tm_now);
        #endif
        
        std::ostringstream timestamp;
        timestamp << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
        
        // Цвета для терминала (работает в Linux/macOS, в Windows потребуется другая логика)
        const char* color = "";
        const char* level_str = "";
        switch(level) {
            case INFO: color = "\033[32m"; level_str = "INFO"; break;
            case WARNING: color = "\033[33m"; level_str = "WARNING"; break;
            case ERROR: color = "\033[31m"; level_str = "ERROR"; break;
            case DEBUG: color = "\033[36m"; level_str = "DEBUG"; break;
        }
        const char* reset = "\033[0m";
        
        // Выводим лог
        std::cout << color << "[" << timestamp.str() << "] [" << level_str << "] " 
                  << reset << message;
        
        if (!context.empty()) {
            std::cout << " {";
            bool first = true;
            for (const auto& [key, value] : context) {
                if (!first) std::cout << ", ";
                std::cout << "\"" << key << "\": \"" << value << "\"";
                first = false;
            }
            std::cout << "}";
        }
        
        std::cout << std::endl;
    }
    
    // Удобные методы для разных уровней
    static void info(const std::string& message, const std::map<std::string, std::string>& context = {}) {
        log(INFO, message, context);
    }
    
    static void warning(const std::string& message, const std::map<std::string, std::string>& context = {}) {
        log(WARNING, message, context);
    }
    
    static void error(const std::string& message, const std::map<std::string, std::string>& context = {}) {
        log(ERROR, message, context);
    }
    
    static void debug(const std::string& message, const std::map<std::string, std::string>& context = {}) {
        log(DEBUG, message, context);
    }
};

#endif