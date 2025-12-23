package jwt

import (
    "errors"
    "fmt"
    "os"
    "time"

    "github.com/golang-jwt/jwt/v5"
)

var (
    ErrInvalidToken      = errors.New("невалидный токен")
    ErrTokenExpired      = errors.New("токен истек")
    ErrInvalidTokenType  = errors.New("неверный тип токена")
    ErrMissingSecret     = errors.New("отсутствует JWT_SECRET")
)

type Claims struct {
    UserID      string   `json:"user_id"`
    Email       string   `json:"email"`
    Permissions []string `json:"permissions,omitempty"`
    Roles       []string `json:"roles,omitempty"`
    Type        string   `json:"type"`
    jwt.RegisteredClaims
}

type User struct {
    ID          string
    Email       string
    Roles       []string
    Permissions []string
}

type JWTManager struct {
    secret               []byte
    accessTokenExpire    time.Duration
    refreshTokenExpire   time.Duration
}

func NewJWTManager() (*JWTManager, error) {
    secret := os.Getenv("JWT_SECRET")
    if len(secret) < 32 {
        return nil, ErrMissingSecret
    }

    accessExpire := 1 * time.Minute
    if envExpire := os.Getenv("ACCESS_TOKEN_EXPIRE"); envExpire != "" {
        if dur, err := time.ParseDuration(envExpire); err == nil {
            accessExpire = dur
        }
    }

    refreshExpire := 7 * 24 * time.Hour
    if envExpire := os.Getenv("REFRESH_TOKEN_EXPIRE"); envExpire != "" {
        if dur, err := time.ParseDuration(envExpire); err == nil {
            refreshExpire = dur
        }
    }

    return &JWTManager{
        secret:             []byte(secret),
        accessTokenExpire:  accessExpire,
        refreshTokenExpire: refreshExpire,
    }, nil
}

func (m *JWTManager) GenerateAccessToken(user *User) (string, error) {
    claims := &Claims{
        UserID:      user.ID,
        Email:       user.Email,
        Permissions: user.Permissions,
        Roles:       user.Roles,
        Type:        "access",
        RegisteredClaims: jwt.RegisteredClaims{
            ExpiresAt: jwt.NewNumericDate(time.Now().Add(m.accessTokenExpire)),
            IssuedAt:  jwt.NewNumericDate(time.Now()),
        },
    }

    token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
    return token.SignedString(m.secret)
}

func (m *JWTManager) GenerateRefreshToken(user *User) (string, error) {
    claims := &Claims{
        UserID: user.ID,
        Email:  user.Email,
        Type:   "refresh",
        RegisteredClaims: jwt.RegisteredClaims{
            ExpiresAt: jwt.NewNumericDate(time.Now().Add(m.refreshTokenExpire)),
            IssuedAt:  jwt.NewNumericDate(time.Now()),
        },
    }

    token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
    return token.SignedString(m.secret)
}

func (m *JWTManager) ParseToken(tokenString string) (*Claims, error) {
    token, err := jwt.ParseWithClaims(tokenString, &Claims{}, func(token *jwt.Token) (interface{}, error) {
        if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
            return nil, fmt.Errorf("неожиданный метод подписи: %v", token.Header["alg"])
        }
        return m.secret, nil
    })

    if err != nil {
        return nil, ErrInvalidToken
    }

    if claims, ok := token.Claims.(*Claims); ok && token.Valid {
        return claims, nil
    }

    return nil, ErrInvalidToken
}

func (m *JWTManager) ValidateToken(tokenString string) (bool, *Claims) {
    claims, err := m.ParseToken(tokenString)
    if err != nil {
        return false, nil
    }

    if time.Now().After(claims.ExpiresAt.Time) {
        return false, nil
    }

    return true, claims
}

func (m *JWTManager) RefreshTokens(refreshToken string) (string, string, error) {
    claims, err := m.ParseToken(refreshToken)
    if err != nil {
        return "", "", err
    }

    if claims.Type != "refresh" {
        return "", "", ErrInvalidTokenType
    }

    user := &User{
        ID:    claims.UserID,
        Email: claims.Email,
    }

    newAccessToken, err := m.GenerateAccessToken(user)
    if err != nil {
        return "", "", err
    }

    newRefreshToken, err := m.GenerateRefreshToken(user)
    if err != nil {
        return "", "", err
    }

    return newAccessToken, newRefreshToken, nil
}