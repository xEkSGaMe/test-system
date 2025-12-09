package config

import (
    "fmt"
    "strings"
    "time"

    "github.com/spf13/viper"
)

type Config struct {
    Env        string `mapstructure:"ENV"`
    Port       string `mapstructure:"PORT"`

    JWTSecret       string        `mapstructure:"JWT_SECRET"`
    JWTAccessExpire time.Duration `mapstructure:"JWT_ACCESS_EXPIRE"`
    JWTRefreshExpire time.Duration `mapstructure:"JWT_REFRESH_EXPIRE"`

    MongoDBURI      string `mapstructure:"MONGODB_URI"`
    MongoDBDatabase string `mapstructure:"MONGODB_DB_NAME"`

    RedisURL string `mapstructure:"REDIS_URI"`

    GitHubClientID     string `mapstructure:"GITHUB_CLIENT_ID"`
    GitHubClientSecret string `mapstructure:"GITHUB_CLIENT_SECRET"`

    YandexClientID     string `mapstructure:"YANDEX_CLIENT_ID"`
    YandexClientSecret string `mapstructure:"YANDEX_CLIENT_SECRET"`

    CORSAllowedOrigins []string `mapstructure:"CORS_ALLOWED_ORIGINS"`

    RateLimitRequests int           `mapstructure:"RATE_LIMIT_REQUESTS"`
    RateLimitWindow   time.Duration `mapstructure:"RATE_LIMIT_WINDOW"`
}

func Load() (*Config, error) {
    viper.SetConfigFile(".env")
    viper.AutomaticEnv()

    // Set defaults
    viper.SetDefault("ENV", "development")
    viper.SetDefault("PORT", "8080")
    viper.SetDefault("JWT_ACCESS_EXPIRE", 15*time.Minute)
    viper.SetDefault("JWT_REFRESH_EXPIRE", 7*24*time.Hour)
    viper.SetDefault("MONGODB_DB_NAME", "test_system")
    viper.SetDefault("CORS_ALLOWED_ORIGINS", []string{"http://localhost:3000"})
    viper.SetDefault("RATE_LIMIT_REQUESTS", 100)
    viper.SetDefault("RATE_LIMIT_WINDOW", 1*time.Minute)

    // Read config
    if err := viper.ReadInConfig(); err != nil {
        if _, ok := err.(viper.ConfigFileNotFoundError); !ok {
            return nil, fmt.Errorf("error reading config file: %w", err)
        }
    }

    var config Config
    if err := viper.Unmarshal(&config); err != nil {
        return nil, fmt.Errorf("unable to decode into struct: %w", err)
    }

    // Validate
    if err := config.Validate(); err != nil {
        return nil, err
    }

    return &config, nil
}

func (c *Config) Validate() error {
    var missing []string

    if c.JWTSecret == "" {
        missing = append(missing, "JWT_SECRET")
    }
    if c.MongoDBURI == "" {
        missing = append(missing, "MONGODB_URI")
    }
    if c.RedisURL == "" {
        missing = append(missing, "REDIS_URI")
    }

    if len(missing) > 0 {
        return fmt.Errorf("missing required environment variables: %s", strings.Join(missing, ", "))
    }

    return nil
}