package models

import (
    "regexp"
    "time"

    "github.com/go-playground/validator/v10"
    "go.mongodb.org/mongo-driver/bson/primitive"
)

type User struct {
    ID             primitive.ObjectID `bson:"_id,omitempty" json:"id"`
    Email          string             `bson:"email" json:"email" validate:"required,email"`
    FullName       string             `bson:"fullName" json:"fullName"`
    Roles          []string           `bson:"roles" json:"roles"`
    RefreshTokens  []string           `bson:"refreshTokens" json:"-"`
    IsBlocked      bool               `bson:"isBlocked" json:"isBlocked"`
    ExternalAuth   ExternalAuth       `bson:"externalAuth" json:"-"`
    CreatedAt      time.Time          `bson:"createdAt" json:"createdAt"`
    UpdatedAt      time.Time          `bson:"updatedAt" json:"updatedAt"`
}

type ExternalAuth struct {
    GitHub GitHubAuth `bson:"github" json:"github"`
    Yandex YandexAuth `bson:"yandex" json:"yandex"`
}

type GitHubAuth struct {
    ID    string `bson:"id" json:"id"`
    Email string `bson:"email" json:"email"`
}

type YandexAuth struct {
    ID    string `bson:"id" json:"id"`
    Email string `bson:"email" json:"email"`
}

const (
    RoleStudent = "student"
    RoleTeacher = "teacher"
    RoleAdmin   = "admin"
)

func NewUser(email, fullName string) *User {
    return &User{
        Email:         email,
        FullName:      fullName,
        Roles:         []string{RoleStudent},
        RefreshTokens: []string{},
        IsBlocked:     false,
        CreatedAt:     time.Now(),
        UpdatedAt:     time.Now(),
    }
}

func (u *User) Validate() error {
    validate := validator.New()
    validate.RegisterValidation("email", func(fl validator.FieldLevel) bool {
        email := fl.Field().String()
        re := regexp.MustCompile(`^[a-z0-9._%+\-]+@[a-z0-9.\-]+\.[a-z]{2,4}$`)
        return re.MatchString(email)
    })
    return validate.Struct(u)
}

func (u *User) HasRole(role string) bool {
    for _, r := range u.Roles {
        if r == role {
            return true
        }
    }
    return false
}

func (u *User) AddRefreshToken(token string) {
    u.RefreshTokens = append(u.RefreshTokens, token)
}

func (u *User) RemoveRefreshToken(token string) {
    for i, t := range u.RefreshTokens {
        if t == token {
            u.RefreshTokens = append(u.RefreshTokens[:i], u.RefreshTokens[i+1:]...)
            break
        }
    }
}

func (u *User) GetPermissions() []string {
    permissions := []string{}
    for _, role := range u.Roles {
        switch role {
        case RoleAdmin:
            permissions = append(permissions, "admin:*")
        case RoleTeacher:
            permissions = append(permissions, "teacher:*")
        case RoleStudent:
            permissions = append(permissions, "student:*")
        }
    }
    return permissions
}

func (u *User) IsAnonymous() bool {
    matched, _ := regexp.MatchString(`^Аноним\d+$`, u.FullName)
    return matched
}