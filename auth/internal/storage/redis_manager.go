package storage

import (
    "context"
    "encoding/json"
    "errors"
    "fmt"
    "time"

    "github.com/redis/go-redis/v9"
)

var (
    ErrSessionNotFound   = errors.New("сессия не найдена")
    ErrTokenNotFound     = errors.New("токен не найден")
    ErrStateNotFound     = errors.New("state не найден")
    ErrInvalidSession    = errors.New("невалидная сессия")
    ErrRedisConnection   = errors.New("ошибка подключения к Redis")
)

type Session struct {
    Type          string            `json:"type"`
    Identifier    string            `json:"identifier"`
    Status        string            `json:"status"`
    LoginToken    string            `json:"login_token,omitempty"`
    UserID        string            `json:"user_id,omitempty"`
    JWTTokens     map[string]string `json:"jwt_tokens,omitempty"`
    CreatedAt     time.Time         `json:"created_at"`
    LastActivity  time.Time         `json:"last_activity"`
    ExpiresAt     time.Time         `json:"expires_at"`
}

type LoginTokenData struct {
    Token     string      `json:"token"`
    Data      interface{} `json:"data"`
    Status    string      `json:"status"`
    UserData  interface{} `json:"user_data,omitempty"`
    CreatedAt time.Time   `json:"created_at"`
    ExpiresAt time.Time   `json:"expires_at"`
}

type OAuthStateData struct {
    State     string    `json:"state"`
    Data      string    `json:"data"`
    CreatedAt time.Time `json:"created_at"`
    ExpiresAt time.Time `json:"expires_at"`
}

type RedisManager struct {
    client *redis.Client
    ctx    context.Context
}

func NewRedisManager(addr, password string, db int) (*RedisManager, error) {
    client := redis.NewClient(&redis.Options{
        Addr:     addr,
        Password: password,
        DB:       db,
        PoolSize: 10,
    })

    ctx := context.Background()
    
    _, err := client.Ping(ctx).Result()
    if err != nil {
        return nil, fmt.Errorf("%w: %v", ErrRedisConnection, err)
    }

    return &RedisManager{
        client: client,
        ctx:    ctx,
    }, nil
}

func (rm *RedisManager) CreateSession(sessionType, identifier, loginToken string) error {
    session := &Session{
        Type:         sessionType,
        Identifier:   identifier,
        Status:       "unknown",
        LoginToken:   loginToken,
        CreatedAt:    time.Now(),
        LastActivity: time.Now(),
        ExpiresAt:    time.Now().Add(24 * time.Hour),
    }

    key := fmt.Sprintf("session:%s:%s", sessionType, identifier)
    data, err := json.Marshal(session)
    if err != nil {
        return err
    }

    return rm.client.Set(rm.ctx, key, data, 24*time.Hour).Err()
}

func (rm *RedisManager) GetSession(sessionType, identifier string) (*Session, error) {
    key := fmt.Sprintf("session:%s:%s", sessionType, identifier)
    
    data, err := rm.client.Get(rm.ctx, key).Bytes()
    if err != nil {
        if err == redis.Nil {
            return nil, ErrSessionNotFound
        }
        return nil, err
    }

    var session Session
    if err := json.Unmarshal(data, &session); err != nil {
        return nil, err
    }

    return &session, nil
}

func (rm *RedisManager) UpdateSessionStatus(sessionType, identifier, status string, userData interface{}) error {
    session, err := rm.GetSession(sessionType, identifier)
    if err != nil {
        return err
    }

    session.Status = status
    session.LastActivity = time.Now()
    
    if status == "authorized" && userData != nil {
        if userID, ok := userData.(string); ok {
            session.UserID = userID
        }
    }

    key := fmt.Sprintf("session:%s:%s", sessionType, identifier)
    data, err := json.Marshal(session)
    if err != nil {
        return err
    }

    ttl := time.Until(session.ExpiresAt)
    return rm.client.Set(rm.ctx, key, data, ttl).Err()
}

func (rm *RedisManager) DeleteSession(sessionType, identifier string) error {
    key := fmt.Sprintf("session:%s:%s", sessionType, identifier)
    return rm.client.Del(rm.ctx, key).Err()
}

func (rm *RedisManager) UpdateLastActivity(sessionType, identifier string) error {
    session, err := rm.GetSession(sessionType, identifier)
    if err != nil {
        return err
    }

    session.LastActivity = time.Now()
    
    key := fmt.Sprintf("session:%s:%s", sessionType, identifier)
    data, err := json.Marshal(session)
    if err != nil {
        return err
    }

    ttl := time.Until(session.ExpiresAt)
    return rm.client.Set(rm.ctx, key, data, ttl).Err()
}

func (rm *RedisManager) SaveLoginToken(token string, data interface{}, ttl time.Duration) error {
    if ttl == 0 {
        ttl = 10 * time.Minute
    }

    tokenData := &LoginTokenData{
        Token:     token,
        Data:      data,
        Status:    "pending",
        CreatedAt: time.Now(),
        ExpiresAt: time.Now().Add(ttl),
    }

    key := fmt.Sprintf("login_token:%s", token)
    jsonData, err := json.Marshal(tokenData)
    if err != nil {
        return err
    }

    return rm.client.Set(rm.ctx, key, jsonData, ttl).Err()
}

func (rm *RedisManager) GetLoginToken(token string) (*LoginTokenData, error) {
    key := fmt.Sprintf("login_token:%s", token)
    
    data, err := rm.client.Get(rm.ctx, key).Bytes()
    if err != nil {
        if err == redis.Nil {
            return nil, ErrTokenNotFound
        }
        return nil, err
    }

    var tokenData LoginTokenData
    if err := json.Unmarshal(data, &tokenData); err != nil {
        return nil, err
    }

    return &tokenData, nil
}

func (rm *RedisManager) UpdateLoginTokenStatus(token, status string, userData interface{}) error {
    tokenData, err := rm.GetLoginToken(token)
    if err != nil {
        return err
    }

    tokenData.Status = status
    if userData != nil {
        tokenData.UserData = userData
    }

    key := fmt.Sprintf("login_token:%s", token)
    jsonData, err := json.Marshal(tokenData)
    if err != nil {
        return err
    }

    ttl := time.Until(tokenData.ExpiresAt)
    return rm.client.Set(rm.ctx, key, jsonData, ttl).Err()
}

func (rm *RedisManager) DeleteLoginToken(token string) error {
    key := fmt.Sprintf("login_token:%s", token)
    return rm.client.Del(rm.ctx, key).Err()
}

func (rm *RedisManager) SaveOAuthState(state string, data interface{}, ttl time.Duration) error {
    if ttl == 0 {
        ttl = 10 * time.Minute
    }

    stateData := &OAuthStateData{
        State:     state,
        Data:      data.(string),
        CreatedAt: time.Now(),
        ExpiresAt: time.Now().Add(ttl),
    }

    key := fmt.Sprintf("oauth_state:%s", state)
    jsonData, err := json.Marshal(stateData)
    if err != nil {
        return err
    }

    return rm.client.Set(rm.ctx, key, jsonData, ttl).Err()
}

func (rm *RedisManager) GetOAuthState(state string) (*OAuthStateData, error) {
    key := fmt.Sprintf("oauth_state:%s", state)
    
    data, err := rm.client.Get(rm.ctx, key).Bytes()
    if err != nil {
        if err == redis.Nil {
            return nil, ErrStateNotFound
        }
        return nil, err
    }

    var stateData OAuthStateData
    if err := json.Unmarshal(data, &stateData); err != nil {
        return nil, err
    }

    return &stateData, nil
}

func (rm *RedisManager) DeleteOAuthState(state string) error {
    key := fmt.Sprintf("oauth_state:%s", state)
    return rm.client.Del(rm.ctx, key).Err()
}

func (rm *RedisManager) SaveJWTBlacklist(jti string, ttl time.Duration) error {
    key := fmt.Sprintf("jwt_blacklist:%s", jti)
    return rm.client.Set(rm.ctx, key, "blacklisted", ttl).Err()
}

func (rm *RedisManager) IsJWTBlacklisted(jti string) (bool, error) {
    key := fmt.Sprintf("jwt_blacklist:%s", jti)
    
    _, err := rm.client.Get(rm.ctx, key).Result()
    if err != nil {
        if err == redis.Nil {
            return false, nil
        }
        return false, err
    }

    return true, nil
}

func (rm *RedisManager) HealthCheck() error {
    _, err := rm.client.Ping(rm.ctx).Result()
    return err
}

func (rm *RedisManager) GetStats() (map[string]interface{}, error) {
    info, err := rm.client.Info(rm.ctx).Result()
    if err != nil {
        return nil, err
    }

    stats := map[string]interface{}{
        "connected_clients": "unknown",
        "used_memory":       "unknown",
        "total_connections": "unknown",
    }

    return stats, nil
}