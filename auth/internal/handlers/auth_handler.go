package handlers

import (
	"crypto/rand"
	"encoding/hex"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"test-system/auth/internal/models"
	"test-system/auth/internal/services"
)

type AuthHandler struct {
	authService  *services.AuthService
	oauthService *services.OAuthService
}

func NewAuthHandler(authService *services.AuthService, oauthService *services.OAuthService) *AuthHandler {
	return &AuthHandler{
		authService:  authService,
		oauthService: oauthService,
	}
}

// Вспомогательная функция для генерации короткого кода
func generateTicket() string {
	b := make([]byte, 8)
	rand.Read(b)
	return "tk_" + hex.EncodeToString(b)
}

// YandexLogin
func (h *AuthHandler) YandexLogin(c *gin.Context) {
	url := h.oauthService.GetYandexAuthURL()
	c.Redirect(http.StatusTemporaryRedirect, url)
}

// YandexCallback
func (h *AuthHandler) YandexCallback(c *gin.Context) {
	code := c.Query("code")
	if code == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Code not provided"})
		return
	}

	_, tokens, err := h.oauthService.HandleYandexCallback(c.Request.Context(), code)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to authenticate: " + err.Error()})
		return
	}

	// Создаем тикет и сохраняем его в Redis на 2 минуты
	ticket := generateTicket()
	err = h.authService.StoreTicket(c.Request.Context(), ticket, tokens.AccessToken)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to store ticket"})
		return
	}

	// Отправляем в бота короткий код
	tgRedirect := "https://t.me/TestSystemDevBot?start=" + ticket
	c.Redirect(http.StatusFound, tgRedirect)
}

// GitHubLogin
func (h *AuthHandler) GitHubLogin(c *gin.Context) {
	url := h.oauthService.GetGitHubAuthURL()
	c.Redirect(http.StatusTemporaryRedirect, url)
}

// GitHubCallback
func (h *AuthHandler) GitHubCallback(c *gin.Context) {
	code := c.Query("code")
	if code == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Code is required"})
		return
	}

	_, pair, err := h.oauthService.HandleGitHubCallback(c.Request.Context(), code)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	// Создаем тикет
	ticket := generateTicket()
	err = h.authService.StoreTicket(c.Request.Context(), ticket, pair.AccessToken)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to store ticket"})
		return
	}

	c.Redirect(http.StatusFound, "https://t.me/TestSystemDevBot?start="+ticket)
}

// ExchangeTicket — новый метод, который вызывает бот
func (h *AuthHandler) ExchangeTicket(c *gin.Context) {
	ticket := c.Param("ticket")
	if ticket == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Ticket is required"})
		return
	}

	// Получаем токен из Redis и тут же удаляем его (одноразовое использование)
	token, err := h.authService.GetTokenByTicket(c.Request.Context(), ticket)
	if err != nil || token == "" {
		c.JSON(http.StatusNotFound, gin.H{"error": "Ticket expired or invalid"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"token": token})
}

// Register
func (h *AuthHandler) Register(c *gin.Context) {
	var req models.CreateUserRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	user, tokens, err := h.authService.Register(c.Request.Context(), &req)
	if err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusCreated, gin.H{"user": user, "auth": tokens})
}

// Login
func (h *AuthHandler) Login(c *gin.Context) {
	var req models.LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	user, tokens, err := h.authService.Login(c.Request.Context(), &req)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "Invalid email or password"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"user": user, "auth": tokens})
}

// Logout
func (h *AuthHandler) Logout(c *gin.Context) {
	token := extractToken(c)
	if token == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "No token provided"})
		return
	}

	_ = h.authService.Logout(c.Request.Context(), token)
	c.JSON(http.StatusOK, gin.H{"message": "Successfully logged out"})
}

// Validate
func (h *AuthHandler) Validate(c *gin.Context) {
	token := extractToken(c)
	claims, err := h.authService.ValidateToken(c.Request.Context(), token)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"valid": false, "error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{"valid": true, "claims": claims})
}

// Refresh
func (h *AuthHandler) Refresh(c *gin.Context) {
	var req RefreshRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	pair, err := h.authService.RefreshToken(c.Request.Context(), req.RefreshToken)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusOK, gin.H{"auth": pair})
}

// Me
func (h *AuthHandler) Me(c *gin.Context) {
	token := extractToken(c)
	claims, err := h.authService.ValidateToken(c.Request.Context(), token)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"success": false, "error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"success": true,
		"data": gin.H{
			"id":    claims.UserID,
			"email": claims.Email,
			"role":  claims.Role,
		},
	})
}

func extractToken(c *gin.Context) string {
	authHeader := c.GetHeader("Authorization")
	if authHeader == "" {
		return ""
	}
	parts := strings.Split(authHeader, " ")
	if len(parts) != 2 || parts[0] != "Bearer" {
		return ""
	}
	return parts[1]
}

type RefreshRequest struct {
	RefreshToken string `json:"refresh_token" binding:"required"`
}