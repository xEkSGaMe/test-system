package repositories

import (
    "context"
    "crypto/sha256"
    "database/sql"
    "encoding/hex"
    "time"

    "test-system/auth/internal/models"
)

type RefreshTokenRepository struct {
    db *sql.DB   // <-- меняем на *sql.DB
}

func NewRefreshTokenRepository(db *sql.DB) *RefreshTokenRepository {
    return &RefreshTokenRepository{db: db}
}

func hashToken(token string) string {
    sum := sha256.Sum256([]byte(token))
    return hex.EncodeToString(sum[:])
}

func (r *RefreshTokenRepository) Create(ctx context.Context, userID int, rawToken string, expiresAt time.Time) error {
    _, err := r.db.ExecContext(ctx,
        `INSERT INTO refresh_tokens (user_id, token_hash, expires_at, created_at, updated_at)
         VALUES ($1, $2, $3, NOW(), NOW())`,
        userID, hashToken(rawToken), expiresAt,
    )
    return err
}

func (r *RefreshTokenRepository) FindValid(ctx context.Context, rawToken string) (*models.RefreshToken, error) {
    var rt models.RefreshToken
    err := r.db.QueryRowContext(ctx,
        `SELECT id, user_id, token_hash, expires_at, created_at, updated_at
         FROM refresh_tokens
         WHERE token_hash = $1 AND expires_at > NOW()`,
        hashToken(rawToken),
    ).Scan(&rt.ID, &rt.UserID, &rt.TokenHash, &rt.ExpiresAt, &rt.CreatedAt, &rt.UpdatedAt)
    if err == sql.ErrNoRows {
        return nil, nil
    }
    if err != nil {
        return nil, err
    }
    return &rt, nil
}

func (r *RefreshTokenRepository) Invalidate(ctx context.Context, rawToken string) error {
    _, err := r.db.ExecContext(ctx,
        `DELETE FROM refresh_tokens WHERE token_hash = $1`,
        hashToken(rawToken),
    )
    return err
}
