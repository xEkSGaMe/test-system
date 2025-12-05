// database/mongo-init.js
db = db.getSiblingDB('test_system');

// Создаем коллекцию пользователей
db.createCollection('users', {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["email", "fullName", "createdAt"],
      properties: {
        _id: { bsonType: "objectId" },
        email: {
          bsonType: "string",
          pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$",
          description: "Валидный email адрес"
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
          default: ["student"],
          description: "Роли пользователя"
        },
        refreshTokens: {
          bsonType: "array",
          items: { bsonType: "string" },
          default: [],
          description: "Refresh tokens для выхода на всех устройствах"
        },
        isBlocked: {
          bsonType: "bool",
          default: false,
          description: "Заблокирован ли пользователь"
        },
        externalAuth: {
          bsonType: "object",
          properties: {
            github: {
              bsonType: "object",
              properties: {
                id: { bsonType: "string" },
                username: { bsonType: "string" },
                avatarUrl: { bsonType: "string" }
              }
            },
            yandex: {
              bsonType: "object",
              properties: {
                id: { bsonType: "string" },
                login: { bsonType: "string" },
                avatarUrl: { bsonType: "string" }
              }
            }
          },
          description: "Данные внешней аутентификации"
        },
        createdAt: { bsonType: "date" },
        updatedAt: { bsonType: "date" }
      }
    }
  }
});

// Создаем индексы
db.users.createIndex({ email: 1 }, { unique: true, name: "email_unique" });
db.users.createIndex({ roles: 1 }, { name: "roles_index" });
db.users.createIndex({ "externalAuth.github.id": 1 }, { sparse: true, name: "github_id_index" });
db.users.createIndex({ "externalAuth.yandex.id": 1 }, { sparse: true, name: "yandex_id_index" });

// Коллекция для login tokens
db.createCollection('login_tokens', {
  timeseries: {
    timeField: 'createdAt',
    metaField: 'token',
    granularity: 'seconds'
  },
  expireAfterSeconds: 300 // Автоматическое удаление через 5 минут
});

db.login_tokens.createIndex({ token: 1 }, { unique: true, name: "token_unique" });
db.login_tokens.createIndex({ createdAt: 1 }, { name: "created_at_ttl", expireAfterSeconds: 300 });

// Коллекция для OAuth state
db.createCollection('oauth_states', {
  timeseries: {
    timeField: 'createdAt',
    metaField: 'state',
    granularity: 'seconds'
  },
  expireAfterSeconds: 300
});

db.oauth_states.createIndex({ state: 1 }, { unique: true, name: "state_unique" });

// Создаем тестовых пользователей
db.users.insertMany([
  {
    _id: ObjectId("111111111111111111111111"),
    email: "admin@test.com",
    fullName: "Администратор",
    roles: ["admin"],
    refreshTokens: [],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date()
  },
  {
    _id: ObjectId("222222222222222222222222"),
    email: "teacher@test.com",
    fullName: "Преподаватель",
    roles: ["teacher"],
    refreshTokens: [],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date()
  },
  {
    _id: ObjectId("333333333333333333333333"),
    email: "student@test.com",
    fullName: "Студент",
    roles: ["student"],
    refreshTokens: [],
    isBlocked: false,
    createdAt: new Date(),
    updatedAt: new Date()
  }
]);

print("MongoDB инициализирован успешно!");