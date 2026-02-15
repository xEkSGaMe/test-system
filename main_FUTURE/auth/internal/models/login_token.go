package models

import (
    "encoding/json"
    "time"

    "go.mongodb.org/mongo-driver/bson/primitive"
)

type LoginToken struct {
    Token       string             `bson:"token" json:"token"`
    SessionType string             `bson:"sessionType" json:"sessionType"`
    Identifier  string             `bson:"identifier" json:"identifier"`
    Status      string             `bson:"status" json:"status"`
    UserID      primitive.ObjectID `bson:"userId,omitempty" json:"userId,omitempty"`
    CreatedAt   time.Time          `bson:"createdAt" json:"createdAt"`
    ExpiresAt   time.Time          `bson:"expiresAt" json:"expiresAt"`
    UpdatedAt   time.Time          `bson:"updatedAt" json:"updatedAt"`
}

const (
    SessionTypeWeb      = "web"
    SessionTypeTelegram = "telegram"

    StatusPending    = "pending"
    StatusAuthorized = "authorized"
    StatusDenied     = "denied"
    StatusExpired    = "expired"
)

func NewLoginToken(sessionType, identifier string) *LoginToken {
    now := time.Now()
    expiresAt := now.Add(5 * time.Minute)

    return &LoginToken{
        Token:       generateToken(),
        SessionType: sessionType,
        Identifier:  identifier,
        Status:      StatusPending,
        CreatedAt:   now,
        ExpiresAt:   expiresAt,
        UpdatedAt:   now,
    }
}

func (lt *LoginToken) IsExpired() bool {
    return time.Now().After(lt.ExpiresAt)
}

func (lt *LoginToken) ToJSON() (string, error) {
    data, err := json.Marshal(lt)
    if err != nil {
        return "", err
    }
    return string(data), nil
}

func (lt *LoginToken) FromJSON(data string) error {
    return json.Unmarshal([]byte(data), lt)
}

func (lt *LoginToken) UpdateStatus(newStatus string) {
    lt.Status = newStatus
    lt.UpdatedAt = time.Now()
}

func (lt *LoginToken) IsPending() bool {
    return lt.Status == StatusPending
}

func generateToken() string {
    return primitive.NewObjectID().Hex()
}