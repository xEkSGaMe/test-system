package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"
	
	"github.com/gin-gonic/gin"
	
	"test-system/auth/internal/config"
	"test-system/auth/internal/repositories"
	"test-system/auth/internal/auth/jwt"
	"test-system/auth/internal/storage"
	"test-system/auth/internal/services"
	"test-system/auth/internal/handlers"
	"test-system/auth/internal/middleware"
	
	_ "github.com/lib/pq"
)

func main() {
	cfg := config.LoadConfig()
	
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.Printf("Starting auth service in %s mode", cfg.Server.Env)
	
	db, err := repositories.NewDatabase(cfg.Database.GetDSN())
	if err != nil {
		log.Fatalf("Failed to connect to database: %v", err)
	}
	defer db.Close()
	
	log.Println("✅ Successfully connected to PostgreSQL")
	
	redisMgr, err := storage.NewRedisManager(cfg.Redis.GetAddr(), cfg.Redis.Password, cfg.Redis.DB)
	if err != nil {
		log.Fatalf("Failed to connect to Redis: %v", err)
	}
	defer redisMgr.Close()
	
	log.Println("✅ Successfully connected to Redis")
	
	jwtMgr := jwt.NewManager(cfg.JWT.Secret, cfg.JWT.ExpiryTime)
	userRepo := repositories.NewUserRepository(db)
	refreshRepo := repositories.NewRefreshTokenRepository(db)
	authService := services.NewAuthService(userRepo, jwtMgr, redisMgr, refreshRepo)
	authHandler := handlers.NewAuthHandler(authService)

	
	if cfg.Server.Env == "production" {
		gin.SetMode(gin.ReleaseMode)
	}
	
	router := gin.Default()
	
	router.Use(func(c *gin.Context) {
		c.Writer.Header().Set("Access-Control-Allow-Origin", "*")
		c.Writer.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		c.Writer.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		
		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(204)
			return
		}
		
		c.Next()
	})
	
	router.GET("/health", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"status":  "ok",
			"service": "auth",
			"time":    time.Now().Format(time.RFC3339),
		})
	})
	
	router.POST("/auth/register", authHandler.Register)
	router.POST("/auth/login", authHandler.Login)
	router.POST("/auth/validate", authHandler.Validate)
	router.POST("/auth/refresh", authHandler.Refresh)

	
	protected := router.Group("/")
	protected.Use(middleware.AuthMiddleware(authService))
	{
		protected.POST("/auth/logout", authHandler.Logout)
		protected.GET("/auth/me", func(c *gin.Context) {
			userID, _ := c.Get("user_id")
			userEmail, _ := c.Get("user_email")
			userRole, _ := c.Get("user_role")
			
			c.JSON(http.StatusOK, gin.H{
				"success": true,
				"data": gin.H{
					"id":    userID,
					"email": userEmail,
					"role":  userRole,
				},
			})
		})
	}
	
	srv := &http.Server{
		Addr:    ":" + cfg.Server.Port,
		Handler: router,
	}
	
	go func() {
		log.Printf("🚀 Auth service starting on port %s", cfg.Server.Port)
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("Failed to start server: %v", err)
		}
	}()
	
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit
	log.Println("Shutting down server...")
	
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	
	if err := srv.Shutdown(ctx); err != nil {
		log.Fatal("Server forced to shutdown:", err)
	}
	
	log.Println("Server exited properly")
}