package models

import (
    "time"
    "regexp"
    "github.com/go-playground/validator/v10"
)

type User struct {
    ID            int       `json:"id" db:"id"`  // БЫЛО: primitive.ObjectID
    Email         string    `json:"email" db:"email" validate:"required,email"`
    PasswordHash  string    `json:"-" db:"password_hash"` // Нужно для логина!
    FullName      string    `json:"full_name" db:"full_name"`
    Role          string    `json:"role" db:"role"`       // admin, teacher, student
    CreatedAt     time.Time `json:"created_at" db:"created_at"`
}

// Конструктор
func NewUser(email, passwordHash, fullName string) *User {
    return &User{
        Email:        email,
        PasswordHash: passwordHash,
        FullName:     fullName,
        Role:         "student",
        CreatedAt:    time.Now(),
    }
}

// Валидация осталась прежней
func (u *User) Validate() error {
    validate := validator.New()
    return validate.Struct(u)
}