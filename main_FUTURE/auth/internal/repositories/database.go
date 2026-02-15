package repositories

import (
    "context"
    "time"

    "github.com/go-redis/redis/v9"
    "go.mongodb.org/mongo-driver/mongo"
    "go.mongodb.org/mongo-driver/mongo/options"
    "go.uber.org/zap"
)

// NewMongoClient создает подключение к MongoDB
func NewMongoClient(uri string, logger *zap.Logger) (*mongo.Client, error) {
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()

    clientOptions := options.Client().ApplyURI(uri)
    client, err := mongo.Connect(ctx, clientOptions)
    if err != nil {
        logger.Error("Failed to connect to MongoDB", zap.Error(err))
        return nil, err
    }

    // Проверка подключения
    err = client.Ping(ctx, nil)
    if err != nil {
        logger.Error("Failed to ping MongoDB", zap.Error(err))
        return nil, err
    }

    logger.Info("Successfully connected to MongoDB")
    return client, nil
}

// NewRedisClient создает подключение к Redis
func NewRedisClient(uri string, logger *zap.Logger) *redis.Client {
    opt, err := redis.ParseURL(uri)
    if err != nil {
        logger.Fatal("Failed to parse Redis URL", zap.Error(err))
    }

    client := redis.NewClient(opt)

    // Проверка подключения
    ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
    defer cancel()

    if err := client.Ping(ctx).Err(); err != nil {
        logger.Fatal("Failed to connect to Redis", zap.Error(err))
    }

    logger.Info("Successfully connected to Redis")
    return client
}
