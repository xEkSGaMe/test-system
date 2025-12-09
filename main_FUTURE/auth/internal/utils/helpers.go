package utils

import (
    "crypto/rand"
    "encoding/hex"
    "fmt"
    "regexp"
    "strings"
    "time"

    "go.mongodb.org/mongo-driver/bson/primitive"
)

func GenerateRandomString(n int) (string, error) {
    bytes := make([]byte, n)
    if _, err := rand.Read(bytes); err != nil {
        return "", err
    }
    return hex.EncodeToString(bytes), nil
}

func GenerateLoginToken() string {
    return primitive.NewObjectID().Hex()
}

func HashString(input string) string {
    return primitive.NewObjectID().Hex()
}

func ValidateEmail(email string) bool {
    re := regexp.MustCompile(`^[a-z0-9._%+\-]+@[a-z0-9.\-]+\.[a-z]{2,4}$`)
    return re.MatchString(email)
}

func ParseObjectID(id string) (primitive.ObjectID, error) {
    return primitive.ObjectIDFromHex(id)
}

var anonymousCounter int = 0

func GenerateAnonymousName() string {
    anonymousCounter++
    return fmt.Sprintf("Аноним%d", anonymousCounter)
}

func ParseRoles(rolesStr string) ([]string, error) {
    roles := strings.Split(rolesStr, ",")
    for i, role := range roles {
        roles[i] = strings.TrimSpace(role)
    }
    return roles, nil
}

func ContainsString(slice []string, str string) bool {
    for _, s := range slice {
        if s == str {
            return true
        }
    }
    return false
}

func RemoveString(slice []string, str string) []string {
    for i, s := range slice {
        if s == str {
            return append(slice[:i], slice[i+1:]...)
        }
    }
    return slice
}