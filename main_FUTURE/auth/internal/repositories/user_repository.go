package repositories

import (
    "context"
    "fmt"
    "time"

    "auth-service/internal/models"
    "go.mongodb.org/mongo-driver/bson"
    "go.mongodb.org/mongo-driver/bson/primitive"
    "go.mongodb.org/mongo-driver/mongo"
    "go.mongodb.org/mongo-driver/mongo/options"
)

type UserRepository interface {
    FindByEmail(ctx context.Context, email string) (*models.User, error)
    FindByID(ctx context.Context, id string) (*models.User, error)
    FindByExternalAuth(ctx context.Context, provider, externalID string) (*models.User, error)
    Create(ctx context.Context, user *models.User) error
    Update(ctx context.Context, user *models.User) error
    AddRefreshToken(ctx context.Context, userID, token string) error
    RemoveRefreshToken(ctx context.Context, userID, token string) error
    RemoveAllRefreshTokens(ctx context.Context, userID string) error
    BlockUser(ctx context.Context, userID string) error
    UnblockUser(ctx context.Context, userID string) error
    GetUserPermissions(ctx context.Context, userID string) ([]string, error)
}

type userRepository struct {
    collection *mongo.Collection
}

func NewUserRepository(database *mongo.Database) UserRepository {
    collection := database.Collection("users")
    return &userRepository{collection: collection}
}

func (r *userRepository) FindByEmail(ctx context.Context, email string) (*models.User, error) {
    var user models.User
    err := r.collection.FindOne(ctx, bson.M{"email": email}).Decode(&user)
    if err != nil {
        if err == mongo.ErrNoDocuments {
            return nil, nil
        }
        return nil, err
    }
    return &user, nil
}

func (r *userRepository) FindByID(ctx context.Context, id string) (*models.User, error) {
    objectID, err := primitive.ObjectIDFromHex(id)
    if err != nil {
        return nil, err
    }

    var user models.User
    err = r.collection.FindOne(ctx, bson.M{"_id": objectID}).Decode(&user)
    if err != nil {
        if err == mongo.ErrNoDocuments {
            return nil, nil
        }
        return nil, err
    }
    return &user, nil
}

func (r *userRepository) FindByExternalAuth(ctx context.Context, provider, externalID string) (*models.User, error) {
    var filter bson.M
    switch provider {
    case "github":
        filter = bson.M{"externalAuth.github.id": externalID}
    case "yandex":
        filter = bson.M{"externalAuth.yandex.id": externalID}
    default:
        return nil, fmt.Errorf("unknown provider: %s", provider)
    }

    var user models.User
    err := r.collection.FindOne(ctx, filter).Decode(&user)
    if err != nil {
        if err == mongo.ErrNoDocuments {
            return nil, nil
        }
        return nil, err
    }
    return &user, nil
}

func (r *userRepository) Create(ctx context.Context, user *models.User) error {
    user.ID = primitive.NewObjectID()
    user.CreatedAt = time.Now()
    user.UpdatedAt = time.Now()

    _, err := r.collection.InsertOne(ctx, user)
    return err
}

func (r *userRepository) Update(ctx context.Context, user *models.User) error {
    user.UpdatedAt = time.Now()

    _, err := r.collection.UpdateOne(
        ctx,
        bson.M{"_id": user.ID},
        bson.M{"$set": user},
    )
    return err
}

func (r *userRepository) AddRefreshToken(ctx context.Context, userID, token string) error {
    objectID, err := primitive.ObjectIDFromHex(userID)
    if err != nil {
        return err
    }

    _, err = r.collection.UpdateOne(
        ctx,
        bson.M{"_id": objectID},
        bson.M{"$push": bson.M{"refreshTokens": token}},
    )
    return err
}

func (r *userRepository) RemoveRefreshToken(ctx context.Context, userID, token string) error {
    objectID, err := primitive.ObjectIDFromHex(userID)
    if err != nil {
        return err
    }

    _, err = r.collection.UpdateOne(
        ctx,
        bson.M{"_id": objectID},
        bson.M{"$pull": bson.M{"refreshTokens": token}},
    )
    return err
}

func (r *userRepository) RemoveAllRefreshTokens(ctx context.Context, userID string) error {
    objectID, err := primitive.ObjectIDFromHex(userID)
    if err != nil {
        return err
    }

    _, err = r.collection.UpdateOne(
        ctx,
        bson.M{"_id": objectID},
        bson.M{"$set": bson.M{"refreshTokens": []string{}}},
    )
    return err
}

func (r *userRepository) BlockUser(ctx context.Context, userID string) error {
    objectID, err := primitive.ObjectIDFromHex(userID)
    if err != nil {
        return err
    }

    _, err = r.collection.UpdateOne(
        ctx,
        bson.M{"_id": objectID},
        bson.M{"$set": bson.M{"isBlocked": true, "updatedAt": time.Now()}},
    )
    return err
}

func (r *userRepository) UnblockUser(ctx context.Context, userID string) error {
    objectID, err := primitive.ObjectIDFromHex(userID)
    if err != nil {
        return err
    }

    _, err = r.collection.UpdateOne(
        ctx,
        bson.M{"_id": objectID},
        bson.M{"$set": bson.M{"isBlocked": false, "updatedAt": time.Now()}},
    )
    return err
}

func (r *userRepository) GetUserPermissions(ctx context.Context, userID string) ([]string, error) {
    user, err := r.FindByID(ctx, userID)
    if err != nil {
        return nil, err
    }
    if user == nil {
        return nil, fmt.Errorf("user not found")
    }
    return user.GetPermissions(), nil
}

func (r *userRepository) EnsureIndexes(ctx context.Context) error {
    emailIndex := mongo.IndexModel{
        Keys:    bson.M{"email": 1},
        Options: options.Index().SetUnique(true),
    }
    rolesIndex := mongo.IndexModel{
        Keys: bson.M{"roles": 1},
    }
    externalAuthIndex := mongo.IndexModel{
        Keys: bson.D{
            {Key: "externalAuth.github.id", Value: 1},
            {Key: "externalAuth.yandex.id", Value: 1},
        },
    }

    _, err := r.collection.Indexes().CreateMany(ctx, []mongo.IndexModel{emailIndex, rolesIndex, externalAuthIndex})
    return err
}