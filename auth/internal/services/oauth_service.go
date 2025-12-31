package services

import (
	"context"
	"encoding/json"
	"fmt"
	"time"
	"test-system/auth/internal/config"
	"test-system/auth/internal/models"
	"golang.org/x/oauth2"
)

type OAuthService struct {
	authService *AuthService
	yandexCfg   *oauth2.Config
}

func NewOAuthService(cfg *config.Config, authService *AuthService) *OAuthService {
	return &OAuthService{
		authService: authService,
		yandexCfg: &oauth2.Config{
			// Эти данные подтянутся из твоего обновленного .env
			ClientID:     cfg.Yandex.ClientID, 
			ClientSecret: cfg.Yandex.ClientSecret,
			RedirectURL:  cfg.Yandex.RedirectURL,
			Endpoint: oauth2.Endpoint{
				AuthURL:  "https://oauth.yandex.ru/authorize",
				TokenURL: "https://oauth.yandex.ru/token",
			},
			Scopes: []string{"login:email", "login:info"},
		},
	}
}

func (s *OAuthService) GetYandexAuthURL() string {
	// Формируем ссылку вручную, чтобы точно контролировать параметры
	return fmt.Sprintf(
		"https://oauth.yandex.ru/authorize?response_type=code&client_id=%s&redirect_uri=%s",
		s.yandexCfg.ClientID,
		s.yandexCfg.RedirectURL,
	)
}

// Обработка ответа от Яндекса
func (s *OAuthService) HandleYandexCallback(ctx context.Context, code string) (*models.User, *TokenPair, error) {
	// 1. Обмениваем временный код на токен доступа
	token, err := s.yandexCfg.Exchange(ctx, code)
	if err != nil {
		return nil, nil, err
	}

	// 2. Запрашиваем данные профиля у Яндекса
	client := s.yandexCfg.Client(ctx, token)
	resp, err := client.Get("https://login.yandex.ru/info?format=json")
	if err != nil {
		return nil, nil, err
	}
	defer resp.Body.Close()

	// 3. Разбираем JSON от Яндекса
	var userInfo struct {
		DefaultEmail string `json:"default_email"`
		FirstName    string `json:"first_name"`
		LastName     string `json:"last_name"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&userInfo); err != nil {
		return nil, nil, err
	}

	fullName := fmt.Sprintf("%s %s", userInfo.FirstName, userInfo.LastName)
	
	// 4. Маппинг: логиним или регистрируем в нашей базе
	return s.loginOrRegisterOAuthUser(ctx, userInfo.DefaultEmail, fullName)
}

func (s *OAuthService) loginOrRegisterOAuthUser(ctx context.Context, email, name string) (*models.User, *TokenPair, error) {
	user, err := s.authService.userRepo.FindByEmail(ctx, email)
	if err != nil {
		return nil, nil, err
	}

	// Если пользователя нет в PostgreSQL — создаем его с ролью student
	if user == nil {
		req := &models.CreateUserRequest{
			Email:    email,
			FullName: name,
			Password: "oauth_generated_password_" + email,
		}
		// По умолчанию в Register должна стоять роль "student"
		return s.authService.Register(ctx, req)
	}

	// Генерируем JWT для нашего Core API (C++)
	accessToken, expiresIn, err := s.authService.jwtMgr.GenerateToken(user.ID, user.Email, user.Role)
	if err != nil {
		return nil, nil, err
	}

	refreshToken := fmt.Sprintf("rt_%d_%s", time.Now().UnixNano(), email)
	expiresAt := time.Now().Add(24 * time.Hour) 
	
	err = s.authService.refreshRepo.Create(ctx, user.ID, refreshToken, expiresAt)
	if err != nil {
		return nil, nil, err
	}

	return user, &TokenPair{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		TokenType:    "Bearer",
		ExpiresIn:    expiresIn,
	}, nil
}