// Инициализация MongoDB для системы тестирования
db = db.getSiblingDB('test_system');

// Создаем коллекцию users с валидацией
db.createCollection('users', {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["email", "fullName", "roles", "createdAt", "updatedAt"],
      properties: {
        email: {
          bsonType: "string",
          pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$",
          description: "Email пользователя (уникальный)"
        },
        fullName: {
          bsonType: "string",
          minLength: 2,
          maxLength: 100,
          description: "Полное имя пользователя"
        },
        roles: {
          bsonType: "array",
          items: {
            bsonType: "string",
            enum: ["student", "teacher", "admin"]
          },
          minItems: 1,
          description: "Роли пользователя в системе"
        }
      }
    }
  }
});

// Создаем индексы
db.users.createIndex({ "email": 1 }, { unique: true, name: "email_unique" });
db.users.createIndex({ "roles": 1 }, { name: "roles_index" });
db.users.createIndex({ "createdAt": -1 }, { name: "createdAt_desc" });

// Создаем начальных пользователей
const users = [
  {
    email: "student@example.com",
    fullName: "Иван Студентов",
    roles: ["student"],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date(),
    lastLogin: new Date()
  },
  {
    email: "teacher@example.com",
    fullName: "Петр Преподавателев",
    roles: ["teacher"],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date(),
    lastLogin: new Date()
  },
  {
    email: "admin@example.com",
    fullName: "Админ Админов",
    roles: ["admin", "teacher"],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date(),
    lastLogin: new Date()
  }
];

// Вставляем пользователей (если их еще нет)
users.forEach(user => {
  db.users.updateOne(
    { email: user.email },
    { $setOnInsert: user },
    { upsert: true }
  );
});

print("✅ MongoDB инициализирована успешно!");


db.createCollection('sessions', {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["sessionToken", "userId", "status", "createdAt"],
      properties: {
        sessionToken: {
          bsonType: "string",
          description: "Токен сессии (уникальный)"
        },
        userId: {
          bsonType: "string",
          description: "ID пользователя (может быть null для анонимных)"
        },
        chatId: {
          bsonType: "string",
          description: "Chat ID для Telegram (уникальный для Telegram сессий)"
        },
        status: {
          bsonType: "string",
          enum: ["anonymous", "authorized"],
          description: "Статус сессии"
        },
        loginToken: {
          bsonType: "string",
          description: "Токен для входа (для анонимных сессий)"
        },
        accessToken: {
          bsonType: "string",
          description: "JWT access token"
        },
        refreshToken: {
          bsonType: "string",
          description: "JWT refresh token"
        },
        userAgent: {
          bsonType: "string",
          description: "User-Agent браузера"
        },
        ipAddress: {
          bsonType: "string",
          description: "IP адрес клиента"
        },
        lastActivity: {
          bsonType: "date",
          description: "Время последней активности"
        },
        expiresAt: {
          bsonType: "date",
          description: "Время истечения сессии"
        },
        createdAt: {
          bsonType: "date",
          description: "Время создания сессии"
        },
        updatedAt: {
          bsonType: "date",
          description: "Время обновления сессии"
        }
      }
    }
  }
});

// Индексы для sessions
db.sessions.createIndex({ "sessionToken": 1 }, { unique: true, name: "sessionToken_unique" });
db.sessions.createIndex({ "userId": 1 }, { name: "userId_index" });
db.sessions.createIndex({ "chatId": 1 }, { unique: true, sparse: true, name: "chatId_unique" });
db.sessions.createIndex({ "status": 1 }, { name: "status_index" });
db.sessions.createIndex({ "expiresAt": 1 }, { expireAfterSeconds: 0, name: "expiresAt_ttl" });
db.sessions.createIndex({ "lastActivity": 1 }, { name: "lastActivity_index" });