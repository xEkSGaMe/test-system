package repositories

import (
    "database/sql"
    "fmt"
    "time"
    _ "github.com/lib/pq" // Драйвер Postgres
    "go.uber.org/zap"
)

func NewPostgresDB(dsn string, logger *zap.Logger) (*sql.DB, error) {
    db, err := sql.Open("postgres", dsn)
    if err != nil {
        return nil, fmt.Errorf("failed to open db: %w", err)
    }

    // Настройка пула соединений
    db.SetMaxOpenConns(25)
    db.SetMaxIdleConns(25)
    db.SetConnMaxLifetime(5 * time.Minute)

    // Проверка соединения
    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
    defer cancel()

    if err := db.PingContext(ctx); err != nil {
        return nil, fmt.Errorf("failed to ping db: %w", err)
    }

    logger.Info("Successfully connected to PostgreSQL")
    return db, nil
}