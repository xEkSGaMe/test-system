package repositories

import (
    "context"
    "database/sql"
    "errors"
    "auth-service/internal/models"
)

type UserRepository struct {
    db *sql.DB
}

func NewUserRepository(db *sql.DB) *UserRepository {
    return &UserRepository{db: db}
}

func (r *UserRepository) Create(ctx context.Context, user *models.User) error {
    query := `
        INSERT INTO users (email, password_hash, full_name, role, created_at) 
        VALUES ($1, $2, $3, $4, $5) 
        RETURNING id`
    
    // Postgres возвращает ID созданной записи
    err := r.db.QueryRowContext(ctx, query, 
        user.Email, user.PasswordHash, user.FullName, user.Role, user.CreatedAt,
    ).Scan(&user.ID)

    return err
}

func (r *UserRepository) FindByEmail(ctx context.Context, email string) (*models.User, error) {
    u := &models.User{}
    query := `SELECT id, email, password_hash, full_name, role, created_at FROM users WHERE email = $1`
    
    err := r.db.QueryRowContext(ctx, query, email).Scan(
        &u.ID, &u.Email, &u.PasswordHash, &u.FullName, &u.Role, &u.CreatedAt,
    )
    
    if err == sql.ErrNoRows {
        return nil, errors.New("user not found")
    }
    return u, err
}