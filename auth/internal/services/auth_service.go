package services

import (
	"context"
	"errors"
	"time"
	"crypto/rand"
	"encoding/hex"

	"test-system/auth/internal/auth/jwt"
	"test-system/auth/internal/models"
	"test-system/auth/internal/repositories"
	"test-system/auth/internal/storage"
	"test-system/auth/internal/utils"
)

type AuthService struct {
	userRepo    *repositories.UserRepository
	jwtMgr      *jwt.Manager
	redisMgr    *storage.RedisManager
	refreshRepo *repositories.RefreshTokenRepository
}

func NewAuthService(
	userRepo *repositories.UserRepository,
	jwtMgr *jwt.Manager,
	redisMgr *storage.RedisManager,
	refreshRepo *repositories.RefreshTokenRepository,
) *AuthService {
	return &AuthService{
		userRepo:    userRepo,
		jwtMgr:      jwtMgr,
		redisMgr:    redisMgr,
		refreshRepo: refreshRepo,
	}
}

// Используем только эту структуру для всех ответов с токенами
type TokenPair struct {
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
	TokenType    string `json:"token_type"`
	ExpiresIn    int    `json:"expires_in"`
}

func (s *AuthService) Register(ctx context.Context, req *models.CreateUserRequest) (*models.User, *TokenPair, error) {
	existing, _ := s.userRepo.FindByEmail(ctx, req.Email)
	if existing != nil {
		return nil, nil, errors.New("user already exists")
	}

	hashedPassword, err := utils.HashPassword(req.Password)
	if err != nil {
		return nil, nil, err
	}

	user := &models.User{
		Email:        req.Email,
		PasswordHash: hashedPassword,
		FullName:     req.FullName,
		Role:         "student",
		IsBlocked:    false,
		CreatedAt:    time.Now(),
		UpdatedAt:    time.Now(),
	}

	if err := s.userRepo.Create(ctx, user); err != nil {
		return nil, nil, err
	}

	accessToken, expiresIn, err := s.jwtMgr.GenerateToken(user.ID, user.Email, user.Role)
	if err != nil {
		return nil, nil, err
	}

	// Генерируем refresh токен при регистрации
	refresh := utils.GenerateSecureToken(32)
	_ = s.refreshRepo.Create(ctx, user.ID, refresh, time.Now().Add(7*24*time.Hour))

	return user, &TokenPair{
		AccessToken:  accessToken,
		RefreshToken: refresh,
		TokenType:    "Bearer",
		ExpiresIn:    expiresIn,
	}, nil
}

func (s *AuthService) Login(ctx context.Context, req *models.LoginRequest) (*models.User, *TokenPair, error) {
	user, err := s.userRepo.FindByEmail(ctx, req.Email)
	if err != nil || user == nil {
		return nil, nil, errors.New("invalid credentials")
	}

	if user.IsBlocked {
		return nil, nil, errors.New("account is blocked")
	}

	if !utils.CheckPasswordHash(req.Password, user.PasswordHash) {
		return nil, nil, errors.New("invalid credentials")
	}

	accessToken, expiresIn, err := s.jwtMgr.GenerateToken(user.ID, user.Email, user.Role)
	if err != nil {
		return nil, nil, err
	}

	refresh := utils.GenerateSecureToken(32)
	_ = s.refreshRepo.Create(ctx, user.ID, refresh, time.Now().Add(7*24*time.Hour))

	return user, &TokenPair{
		AccessToken:  accessToken,
		RefreshToken: refresh,
		TokenType:    "Bearer",
		ExpiresIn:    expiresIn,
	}, nil
}

func (s *AuthService) ValidateToken(ctx context.Context, token string) (*jwt.Claims, error) {
	isBlacklisted, _ := s.redisMgr.IsTokenBlacklisted(ctx, token)
	if isBlacklisted {
		return nil, errors.New("token has been invalidated (logged out)")
	}

	claims, err := s.jwtMgr.ValidateToken(token)
	if err != nil {
		return nil, err
	}

	user, err := s.userRepo.FindByID(ctx, claims.UserID)
	if err != nil || user == nil || user.IsBlocked {
		return nil, errors.New("user unavailable")
	}

	return claims, nil
}

func (s *AuthService) Logout(ctx context.Context, token string) error {
	claims, err := s.jwtMgr.ValidateToken(token)
	if err != nil {
		return nil
	}

	ttl := time.Until(claims.ExpiresAt.Time)
	if ttl > 0 {
		return s.redisMgr.BlacklistToken(ctx, token, ttl)
	}
	return nil
}

// StoreTicket сохраняет JWT access token в Redis с привязкой к короткому тикету
func (s *AuthService) StoreTicket(ctx context.Context, ticket string, token string) error {
    // Используем redisMgr, который уже есть в AuthService
    // Устанавливаем время жизни 2 минуты (этого хватит, чтобы бот успел забрать токен)
    return s.redisMgr.Client().Set(ctx, "ticket:"+ticket, token, 2*time.Minute).Err()
}

// GetTokenByTicket получает токен из Redis и сразу его удаляет
func (s *AuthService) GetTokenByTicket(ctx context.Context, ticket string) (string, error) {
    key := "ticket:" + ticket
    
    // Получаем токен
    token, err := s.redisMgr.Client().Get(ctx, key).Result()
    if err != nil {
        return "", err
    }

    // Удаляем тикет сразу после использования (одноразовый код)
    s.redisMgr.Client().Del(ctx, key)
    
    return token, nil
}

func (s *AuthService) IsTokenBlacklisted(ctx context.Context, token string) (bool, error) {
	return s.redisMgr.IsTokenBlacklisted(ctx, token)
}

func (s *AuthService) RefreshToken(ctx context.Context, rawRefresh string) (*TokenPair, error) {
	rt, err := s.refreshRepo.FindValid(ctx, rawRefresh)
	if err != nil || rt == nil {
		return nil, errors.New("invalid or expired refresh token")
	}

	user, err := s.userRepo.FindByID(ctx, rt.UserID)
	if err != nil || user == nil || user.IsBlocked {
		return nil, errors.New("user unavailable")
	}

	accessToken, expiresIn, err := s.jwtMgr.GenerateToken(user.ID, user.Email, user.Role)
	if err != nil {
		return nil, err
	}

	// Генерация нового рефреш-токена
	b := make([]byte, 32)
	if _, err := rand.Read(b); err != nil {
		return nil, err
	}
	newRefreshToken := hex.EncodeToString(b)

	_ = s.refreshRepo.Delete(ctx, rawRefresh) 

	err = s.refreshRepo.Create(ctx, user.ID, newRefreshToken, time.Now().Add(24*time.Hour*7))
	if err != nil {
		return nil, err
	}

	return &TokenPair{
		AccessToken:  accessToken,
		RefreshToken: newRefreshToken,
		TokenType:    "Bearer",
		ExpiresIn:    expiresIn,
	}, nil
}