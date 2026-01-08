package storage

import (
	"context"
	"crypto/sha256"
	"fmt"
	"time"

	"github.com/redis/go-redis/v9"
)

type RedisManager struct {
	client *redis.Client
}

func NewRedisManager(addr, password string, db int) (*RedisManager, error) {
	client := redis.NewClient(&redis.Options{
		Addr:     addr,
		Password: password,
		DB:       db,
	})

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	if err := client.Ping(ctx).Err(); err != nil {
		return nil, fmt.Errorf("failed to connect to redis: %w", err)
	}

	return &RedisManager{client: client}, nil
}

// BlacklistToken добавляет хеш токена в черный список
// Принимает ctx из сервиса для корректного управления жизненным циклом запроса
func (rm *RedisManager) BlacklistToken(ctx context.Context, token string, ttl time.Duration) error {
	tokenHash := fmt.Sprintf("%x", sha256.Sum256([]byte(token)))
	key := fmt.Sprintf("blacklist:%s", tokenHash)
	
	// Устанавливаем значение "1" с временем жизни токена
	return rm.client.Set(ctx, key, "1", ttl).Err()
}

// IsTokenBlacklisted проверяет наличие токена в черном списке
func (rm *RedisManager) IsTokenBlacklisted(ctx context.Context, token string) (bool, error) {
	tokenHash := fmt.Sprintf("%x", sha256.Sum256([]byte(token)))
	key := fmt.Sprintf("blacklist:%s", tokenHash)

	val, err := rm.client.Get(ctx, key).Result()
	if err == redis.Nil {
		return false, nil
	}
	if err != nil {
		return false, fmt.Errorf("redis error: %w", err)
	}
	
	return val == "1", nil
}

func (rm *RedisManager) Close() error {
	return rm.client.Close()
}

// Client возвращает прямой доступ к redis.Client для использования в сервисах
func (rm *RedisManager) Client() *redis.Client {
    return rm.client
}