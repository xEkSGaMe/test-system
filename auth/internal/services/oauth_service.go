package services

import (
	"context"
	"crypto/rand"
	"encoding/json"
	"fmt"
	"golang.org/x/oauth2"
	"golang.org/x/oauth2/github"
	"test-system/auth/internal/config"
	"test-system/auth/internal/models"
)

type OAuthService struct {
	authService *AuthService
	yandexCfg   *oauth2.Config
	githubCfg   *oauth2.Config
}

func NewOAuthService(cfg *config.Config, authService *AuthService) *OAuthService {
	return &OAuthService{
		authService: authService,
		yandexCfg: &oauth2.Config{
			ClientID:     cfg.Yandex.ClientID,
			ClientSecret: cfg.Yandex.ClientSecret,
			RedirectURL:  cfg.Yandex.RedirectURL,
			Endpoint: oauth2.Endpoint{
				AuthURL:  "https://oauth.yandex.ru/authorize",
				TokenURL: "https://oauth.yandex.ru/token",
			},
			Scopes: []string{"login:email", "login:info"},
		},
		githubCfg: &oauth2.Config{
			ClientID:     cfg.GitHub.ClientID,
			ClientSecret: cfg.GitHub.ClientSecret,
			RedirectURL:  cfg.GitHub.RedirectURL,
			Endpoint:     github.Endpoint,
			Scopes:       []string{"user:email", "read:user"},
		},
	}
}

func (s *OAuthService) GetYandexAuthURL() string {
	return s.yandexCfg.AuthCodeURL("state")
}

func (s *OAuthService) GetGitHubAuthURL() string {
	return s.githubCfg.AuthCodeURL("state")
}

func (s *OAuthService) HandleGitHubCallback(ctx context.Context, code string) (*models.User, *TokenPair, error) {
	token, err := s.githubCfg.Exchange(ctx, code)
	if err != nil {
		return nil, nil, err
	}

	client := s.githubCfg.Client(ctx, token)
	resp, err := client.Get("https://api.github.com/user")
	if err != nil {
		return nil, nil, err
	}
	defer resp.Body.Close()

	var userInfo struct {
		Login string `json:"login"`
		Email string `json:"email"`
		Name  string `json:"name"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&userInfo); err != nil {
		return nil, nil, err
	}

	email := userInfo.Email
	if email == "" {
		email = userInfo.Login + "@github.com"
	}

	return s.loginOrRegisterOAuthUser(ctx, email, userInfo.Name)
}

func (s *OAuthService) HandleYandexCallback(ctx context.Context, code string) (*models.User, *TokenPair, error) {
	token, err := s.yandexCfg.Exchange(ctx, code)
	if err != nil {
		return nil, nil, err
	}

	client := s.yandexCfg.Client(ctx, token)
	resp, err := client.Get("https://login.yandex.ru/info?format=json")
	if err != nil {
		return nil, nil, err
	}
	defer resp.Body.Close()

	var userInfo struct {
		DefaultEmail string `json:"default_email"`
		FirstName    string `json:"first_name"`
		LastName     string `json:"last_name"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&userInfo); err != nil {
		return nil, nil, err
	}

	fullName := fmt.Sprintf("%s %s", userInfo.FirstName, userInfo.LastName)
	return s.loginOrRegisterOAuthUser(ctx, userInfo.DefaultEmail, fullName)
}

func (s *OAuthService) loginOrRegisterOAuthUser(ctx context.Context, email, name string) (*models.User, *TokenPair, error) {
	user, err := s.authService.userRepo.FindByEmail(ctx, email)
	if err != nil {
		return nil, nil, err
	}

	if user == nil {
		req := &models.CreateUserRequest{
			Email:    email,
			FullName: name,
			Password: "oauth_generated_password_" + email,
		}
		return s.authService.Register(ctx, req)
	}

	accessToken, expiresIn, err := s.authService.jwtMgr.GenerateToken(user.ID, user.Email, user.Role)
	if err != nil {
		return nil, nil, err
	}

	// Генерируем Refresh Token здесь, внутри функции
	b := make([]byte, 32)
	rand.Read(b)
	refreshToken := fmt.Sprintf("%x", b)

	return user, &TokenPair{
		AccessToken:  accessToken,
		RefreshToken: refreshToken,
		TokenType:    "Bearer",
		ExpiresIn:    expiresIn,
	}, nil
}