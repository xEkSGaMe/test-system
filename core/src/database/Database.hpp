#pragma once
#include <pqxx/pqxx>
#include <memory>
#include <string>

class Database {
public:
    explicit Database(const std::string& conn_str);

    // Доступ к соединению для сервисов
    pqxx::connection& connection() { return *connection_; }

    // Добавляем совместимость с вызовами db.conn()
    pqxx::connection& conn() { return *connection_; }

    // Для health-check
    std::string get_connection_string() const { return conn_str_; }

private:
    std::unique_ptr<pqxx::connection> connection_;
    std::string conn_str_;
};
