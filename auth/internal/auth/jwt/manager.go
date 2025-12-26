package jwt

import (
	"fmt"
	"time"
	
	"github.com/golang-jwt/jwt/v5"
)

type Manager struct {
	secret     []byte
	expiryTime time.Duration
}

type Claims struct {
	UserID int    `json:"user_id"`
	Email  string `json:"email"`
	Role   string `json:"role"`
	jwt.RegisteredClaims
}

func NewManager(secret string, expiryTime time.Duration) *Manager {
	return &Manager{
		secret:     []byte(secret),
		expiryTime: expiryTime,
	}
}

func (m *Manager) GenerateToken(userID int, email, role string) (string, int, error) {
	expiresAt := time.Now().Add(m.expiryTime)
	
	claims := &Claims{
		UserID: userID,
		Email:  email,
		Role:   role,
		RegisteredClaims: jwt.RegisteredClaims{
			ExpiresAt: jwt.NewNumericDate(expiresAt),
			IssuedAt:  jwt.NewNumericDate(time.Now()),
			Subject:   fmt.Sprintf("%d", userID),
			Issuer:    "auth-service",
		},
	}
	
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	tokenString, err := token.SignedString(m.secret)
	if err != nil {
		return "", 0, fmt.Errorf("failed to sign token: %w", err)
	}
	
	// Возвращаем время жизни в секундах
	expiresIn := int(m.expiryTime.Seconds())
	return tokenString, expiresIn, nil
}

func (m *Manager) ValidateToken(tokenString string) (*Claims, error) {
	token, err := jwt.ParseWithClaims(tokenString, &Claims{}, func(token *jwt.Token) (interface{}, error) {
		// Проверяем алгоритм подписи
		if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("unexpected signing method: %v", token.Header["alg"])
		}
		return m.secret, nil
	})
	
	if err != nil {
		return nil, fmt.Errorf("failed to parse token: %w", err)
	}
	
	if !token.Valid {
		return nil, fmt.Errorf("invalid token")
	}
	
	claims, ok := token.Claims.(*Claims)
	if !ok {
		return nil, fmt.Errorf("invalid token claims")
	}
	
	return claims, nil
}