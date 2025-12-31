package handlers

import (
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

// YandexLogin godoc
// @Summary Login with Yandex
// @Description Redirects user to Yandex OAuth2 login page
// @Tags auth
// @Success 307
// @Router /auth/yandex/login [get]
func (h *AuthHandler) YandexLogin(c *gin.Context) {
	url := h.oauthService.GetYandexAuthURL()
	c.Redirect(http.StatusTemporaryRedirect, url)
}

// YandexCallback godoc
// @Summary Yandex OAuth2 Callback
// @Description Handles Yandex redirection, creates user if not exists and returns tokens
// @Tags auth
// @Param code query string true "OAuth2 Code"
// @Success 200 {object} map[string]interface{}
// @Router /auth/yandex/callback [get]
func (h *AuthHandler) YandexCallback(c *gin.Context) {
	code := c.Query("code")
	if code == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Code not provided"})
		return
	}

	user, tokens, err := h.oauthService.HandleYandexCallback(c.Request.Context(), code)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to authenticate with Yandex: " + err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"user": user,
		"auth": tokens,
	})
}

// Register godoc
// @Summary Register a new user
// @Description Creates a new user account and returns tokens
// @Tags auth
// @Accept json
// @Produce json
// @Param request body models.CreateUserRequest true "Registration Info"
// @Success 201 {object} map[string]interface{}
// @Failure 400 {object} map[string]string
// @Failure 409 {object} map[string]string
// @Router /auth/register [post]
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

// Login godoc
// @Summary User Login
// @Description Authenticates user and returns access and refresh tokens
// @Tags auth
// @Accept json
// @Produce json
// @Param request body models.LoginRequest true "Login Credentials"
// @Success 200 {object} map[string]interface{}
// @Failure 400 {object} map[string]string
// @Failure 401 {object} map[string]string
// @Router /auth/login [post]
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

// Logout godoc
// @Summary User Logout
// @Description Invalidates the current access token by adding it to blacklist
// @Tags auth
// @Security BearerAuth
// @Success 200 {object} map[string]string
// @Failure 400 {object} map[string]string
// @Router /auth/logout [post]
func (h *AuthHandler) Logout(c *gin.Context) {
	token := extractToken(c)
	if token == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "No token provided"})
		return
	}

	_ = h.authService.Logout(c.Request.Context(), token)
	c.JSON(http.StatusOK, gin.H{"message": "Successfully logged out"})
}

// Validate godoc
// @Summary Validate JWT Token
// @Description Checks if the provided token is valid and returns its claims
// @Tags auth
// @Param Authorization header string true "Bearer <token>"
// @Success 200 {object} map[string]interface{}
// @Failure 401 {object} map[string]interface{}
// @Router /auth/validate [get]
func (h *AuthHandler) Validate(c *gin.Context) {
	token := extractToken(c)
	claims, err := h.authService.ValidateToken(c.Request.Context(), token)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"valid": false, "error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{"valid": true, "claims": claims})
}

// Refresh godoc
// @Summary Refresh Access Token
// @Description Uses a refresh token to obtain a new pair of tokens
// @Tags auth
// @Accept json
// @Produce json
// @Param request body RefreshRequest true "Refresh Token"
// @Success 200 {object} map[string]interface{}
// @Failure 400 {object} map[string]string
// @Failure 401 {object} map[string]string
// @Router /auth/refresh [post]
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

// Me godoc
// @Summary Get Current User Info
// @Description Returns info about the user associated with the provided JWT
// @Tags auth
// @Security BearerAuth
// @Success 200 {object} map[string]interface{}
// @Failure 401 {object} map[string]interface{}
// @Router /auth/me [get]
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