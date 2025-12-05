# MongoDB Схема для системы тестирования

## Коллекция: `users`

### JSON Schema валидация

```json
{
  "$jsonSchema": {
    "bsonType": "object",
    "required": ["email", "fullName", "roles", "createdAt", "updatedAt"],
    "properties": {
      "_id": {
        "bsonType": "objectId",
        "description": "Уникальный идентификатор пользователя"
      },
      "email": {
        "bsonType": "string",
        "pattern": "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$",
        "description": "Email пользователя (уникальный)"
      },
      "fullName": {
        "bsonType": "string",
        "minLength": 2,
        "maxLength": 100,
        "description": "Полное имя пользователя"
      },
      "roles": {
        "bsonType": "array",
        "items": {
          "bsonType": "string",
          "enum": ["student", "teacher", "admin"]
        },
        "minItems": 1,
        "description": "Роли пользователя в системе"
      },
      "refreshTokens": {
        "bsonType": "array",
        "items": {
          "bsonType": "string",
          "pattern": "^[A-Za-z0-9-_]+\\.[A-Za-z0-9-_]+\\.[A-Za-z0-9-_]+$"
        },
        "description": "Список активных refresh токенов"
      },
      "isBlocked": {
        "bsonType": "bool",
        "description": "Заблокирован ли пользователь"
      },
      "externalAuth": {
        "bsonType": "object",
        "properties": {
          "github": {
            "bsonType": "object",
            "properties": {
              "id": {
                "bsonType": "string",
                "description": "GitHub ID пользователя"
              },
              "username": {
                "bsonType": "string",
                "description": "GitHub username"
              },
              "email": {
                "bsonType": "string",
                "description": "Email из GitHub"
              }
            }
          },
          "yandex": {
            "bsonType": "object",
            "properties": {
              "id": {
                "bsonType": "string",
                "description": "Yandex ID пользователя"
              },
              "email": {
                "bsonType": "string",
                "description": "Email из Yandex"
              },
              "login": {
                "bsonType": "string",
                "description": "Логин Yandex"
              }
            }
          }
        },
        "description": "Данные внешней аутентификации"
      },
      "createdAt": {
        "bsonType": "date",
        "description": "Дата создания записи"
      },
      "updatedAt": {
        "bsonType": "date",
        "description": "Дата последнего обновления"
      },
      "lastLogin": {
        "bsonType": "date",
        "description": "Дата последнего входа"
      },
      "loginAttempts": {
        "bsonType": "int",
        "minimum": 0,
        "description": "Количество неудачных попыток входа"
      },
      "lockUntil": {
        "bsonType": "date",
        "description": "Время блокировки (если есть)"
      }
    }
  }
}

// Создаем индексы
db.users.createIndex({ "email": 1 }, { unique: true, name: "email_unique" });
db.users.createIndex({ "roles": 1 }, { name: "roles_index" });
db.users.createIndex({ "isBlocked": 1 }, { name: "isBlocked_index" });
db.users.createIndex({ "createdAt": -1 }, { name: "createdAt_desc" });
db.users.createIndex({ "externalAuth.github.id": 1 }, { name: "github_id_index" });
db.users.createIndex({ "externalAuth.yandex.id": 1 }, { name: "yandex_id_index" });
db.users.createIndex({ "lastLogin": -1 }, { name: "lastLogin_desc" });

// TTL индекс для автоматического удаления старых refresh токенов
db.users.createIndex(
  { "refreshTokens.expiresAt": 1 },
  { 
    name: "refreshTokens_ttl",
    expireAfterSeconds: 0,
    partialFilterExpression: { "refreshTokens.expiresAt": { $exists: true } }
  }
);


// Метод проверки роли
db.users.helperMethods = {
  hasRole: function(userId, role) {
    const user = db.users.findOne({ _id: userId });
    return user && user.roles.includes(role);
  },
  
  addRefreshToken: function(userId, token, expiresAt) {
    return db.users.updateOne(
      { _id: userId },
      { 
        $push: { 
          refreshTokens: {
            token: token,
            expiresAt: expiresAt,
            createdAt: new Date()
          }
        },
        $set: { updatedAt: new Date() }
      }
    );
  },
  
  removeRefreshToken: function(userId, token) {
    return db.users.updateOne(
      { _id: userId },
      { 
        $pull: { 
          refreshTokens: { token: token }
        },
        $set: { updatedAt: new Date() }
      }
    );
  },
  
  incrementLoginAttempts: function(userId) {
    return db.users.updateOne(
      { _id: userId },
      { 
        $inc: { loginAttempts: 1 },
        $set: { updatedAt: new Date() }
      }
    );
  },
  
  resetLoginAttempts: function(userId) {
    return db.users.updateOne(
      { _id: userId },
      { 
        $set: { 
          loginAttempts: 0,
          lockUntil: null,
          lastLogin: new Date(),
          updatedAt: new Date()
        }
      }
    );
  },
  
  blockUser: function(userId, reason) {
    return db.users.updateOne(
      { _id: userId },
      { 
        $set: { 
          isBlocked: true,
          blockReason: reason,
          blockedAt: new Date(),
          updatedAt: new Date()
        }
      }
    );
  },
  
  unblockUser: function(userId) {
    return db.users.updateOne(
      { _id: userId },
      { 
        $set: { 
          isBlocked: false,
          blockReason: null,
          blockedAt: null,
          loginAttempts: 0,
          lockUntil: null,
          updatedAt: new Date()
        }
      }
    );
  }
};


