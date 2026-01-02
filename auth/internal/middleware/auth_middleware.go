package middleware

import (
	"net/http"
	"strings"
	
	"github.com/gin-gonic/gin"
	
	"test-system/auth/internal/services"
)

func AuthMiddleware(authService *services.AuthService) gin.HandlerFunc {
	return func(c *gin.Context) {
		authHeader := c.GetHeader("Authorization")
		if authHeader == "" {
			c.JSON(http.StatusUnauthorized, gin.H{
				"success": false,
				"error":   "Authorization header is required",
			})
			c.Abort()
			return
		}
		
		token := strings.TrimPrefix(authHeader, "Bearer ")
		blacklisted, _ := authService.IsTokenBlacklisted(c.Request.Context(), token)
        if blacklisted {
            c.JSON(http.StatusUnauthorized, gin.H{
                "success": false, 
                "error": "token is revoked (logged out)",
            })
            c.Abort()
            return
        }
		if token == authHeader {
			c.JSON(http.StatusUnauthorized, gin.H{
				"success": false,
				"error":   "Authorization header must start with 'Bearer '",
			})
			c.Abort()
			return
		}
		
		claims, err := authService.ValidateToken(c.Request.Context(), token)
		if err != nil {
			c.JSON(http.StatusUnauthorized, gin.H{
				"success": false,
				"error":   "Invalid or expired token: " + err.Error(),
			})
			c.Abort()
			return
		}
		
		c.Set("user_id", claims.UserID)
		c.Set("user_email", claims.Email)
		c.Set("user_role", claims.Role)
		
		c.Next()
	}
}