package main

import (
    "context"
    "log"
    "net/http"
    "os"
    "os/signal"
    "syscall"
    "time"

    "github.com/gin-contrib/cors"
    "github.com/gin-gonic/gin"
    "github.com/redis/go-redis/v9"
    "go.mongodb.org/mongo-driver/mongo"
    "go.mongodb.org/mongo-driver/mongo/options"
    "go.uber.org/zap"

    "auth-service/internal/config"
    "auth-service/internal/handlers"
    "auth-service/internal/repositories"
    "auth-service/internal/services"
    "auth-service/pkg/auth"
    "auth-service/pkg/redisclient"
)

func main() {
    // Load configuration
    cfg, err := config.Load()
    if err != nil {
        log.Fatalf("Failed to load config: %v", err)
    }

    // Initialize logger
    var logger *zap.Logger
    if cfg.Env == "production" {
        logger, _ = zap.NewProduction()
    } else {
        logger, _ = zap.NewDevelopment()
    }
    defer logger.Sync()

    // Connect to MongoDB
    mongoCtx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()

    mongoClient, err := mongo.Connect(mongoCtx, options.Client().ApplyURI(cfg.MongoDBURI))
    if err != nil {
        logger.Fatal("Failed to connect to MongoDB", zap.Error(err))
    }
    defer func() {
        if err = mongoClient.Disconnect(context.Background()); err != nil {
            logger.Error("Failed to disconnect MongoDB", zap.Error(err))
        }
    }()

    // Connect to Redis
    redisClient := redisclient.New(cfg.RedisURL)
    defer redisClient.Close()

    // Initialize repositories
    userRepo := repositories.NewUserRepository(mongoClient.Database(cfg.MongoDBDatabase))

    // Initialize services
    authService := services.NewAuthService(userRepo, redisClient, cfg)

    // Initialize HTTP server
    router := gin.New()

    // Middleware
    router.Use(gin.Logger())
    router.Use(gin.Recovery())
    router.Use(cors.New(cors.Config{
        AllowOrigins:     cfg.CORSAllowedOrigins,
        AllowMethods:     []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
        AllowHeaders:     []string{"Origin", "Content-Type", "Authorization"},
        ExposeHeaders:    []string{"Content-Length"},
        AllowCredentials: true,
        MaxAge:           12 * time.Hour,
    }))

    // Routes
    handlers.RegisterRoutes(router, authService, logger)

    // Health check
    router.GET("/health", func(c *gin.Context) {
        c.JSON(http.StatusOK, gin.H{"status": "OK"})
    })

    // Version
    router.GET("/version", func(c *gin.Context) {
        c.JSON(http.StatusOK, gin.H{"version": "1.0.0"})
    })

    // Start server
    srv := &http.Server{
        Addr:         ":" + cfg.Port,
        Handler:      router,
        ReadTimeout:  10 * time.Second,
        WriteTimeout: 10 * time.Second,
    }

    go func() {
        if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            logger.Fatal("Failed to start server", zap.Error(err))
        }
    }()

    // Graceful shutdown
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
    <-quit

    logger.Info("Shutting down server...")

    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()

    if err := srv.Shutdown(ctx); err != nil {
        logger.Fatal("Server forced to shutdown", zap.Error(err))
    }

    logger.Info("Server exited")
}