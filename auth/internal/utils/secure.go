package utils

import (
    "crypto/rand"
    "encoding/hex"
)

// GenerateSecureToken генерирует криптографически безопасный токен длиной n байт.
// Возвращает hex-строку длиной 2*n символов.
func GenerateSecureToken(n int) string {
    b := make([]byte, n)
    _, err := rand.Read(b)
    if err != nil {
        // если rand.Read вернул ошибку — возвращаем пустую строку
        return ""
    }
    return hex.EncodeToString(b)
}
