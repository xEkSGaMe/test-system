package config

import (
	"fmt"
	"os"
	"strconv"
	"time"
)

type OAuthProvider struct {
	ClientID     string
	ClientSecret string
	RedirectURL  string
}

type Config struct {
	Server   ServerConfig
	Database DatabaseConfig
	Redis    RedisConfig
	JWT      JWTConfig
	Yandex   OAuthProvider
	GitHub   OAuthProvider
}

type ServerConfig struct {
	Port string
	Env  string
}

type DatabaseConfig struct {
	Host     string
	Port     string
	Name     string
	User     string
	Password string
	SSLMode  string
}

type RedisConfig struct {
	Host     string
	Port     string
	Password string
	DB       int
}

type JWTConfig struct {
	Secret     string
	ExpiryTime time.Duration
}

func LoadConfig() *Config {
	return &Config{
		Server: ServerConfig{
			Port: getEnv("PORT", "8081"),
			Env:  getEnv("ENV", "development"),
		},
		Database: DatabaseConfig{
			Host:     getEnv("DB_HOST", "postgres"),
			Port:     getEnv("DB_PORT", "5432"),
			Name:     getEnv("DB_NAME", "test_system"),
			User:     getEnv("DB_USER", "admin"),
			Password: getEnv("DB_PASSWORD", "admin123"),
			SSLMode:  getEnv("DB_SSL_MODE", "disable"),
		},
		Redis: RedisConfig{
			Host:     getEnv("REDIS_HOST", "redis"),
			Port:     getEnv("REDIS_PORT", "6379"),
			Password: getEnv("REDIS_PASSWORD", ""),
			DB:       getEnvAsInt("REDIS_DB", 0),
		},
		JWT: JWTConfig{
			Secret:     getEnv("JWT_SECRET", "your-super-secret-jwt-key-change-this-in-production"),
			ExpiryTime: 15 * time.Minute,
		},
		Yandex: OAuthProvider{
			ClientID:     getEnv("YANDEX_CLIENT_ID", ""),
			ClientSecret: getEnv("YANDEX_CLIENT_SECRET", ""),
			RedirectURL:  getEnv("YANDEX_REDIRECT_URL", "http://localhost:8081/auth/yandex/callback"),
		},
		GitHub: OAuthProvider{
			ClientID:     getEnv("GITHUB_CLIENT_ID", ""),
			ClientSecret: getEnv("GITHUB_CLIENT_SECRET", ""),
			RedirectURL:  getEnv("GITHUB_REDIRECT_URL", "http://localhost:8081/auth/github/callback"),
		},
	}
}

func (db *DatabaseConfig) GetDSN() string {
	return fmt.Sprintf(
		"host=%s port=%s user=%s password=%s dbname=%s sslmode=%s",
		db.Host, db.Port, db.User, db.Password, db.Name, db.SSLMode,
	)
}

func (redis *RedisConfig) GetAddr() string {
	return fmt.Sprintf("%s:%s", redis.Host, redis.Port)
}

func getEnv(key, defaultValue string) string {
	if value, exists := os.LookupEnv(key); exists {
		return value
	}
	return defaultValue
}

func getEnvAsInt(key string, defaultValue int) int {
	if value, exists := os.LookupEnv(key); exists {
		if intValue, err := strconv.Atoi(value); err == nil {
			return intValue
		}
	}
	return defaultValue
}