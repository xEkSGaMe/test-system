#include "Database.hpp"
#include <stdexcept>

Database::Database(const std::string& conn_str) : conn_str_(conn_str) {
    connection_ = std::make_unique<pqxx::connection>(conn_str);
    if (!connection_->is_open()) {
        throw std::runtime_error("Failed to open PostgreSQL connection");
    }
}
