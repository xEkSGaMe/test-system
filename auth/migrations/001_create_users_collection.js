// Создание коллекции users и индексов
db.createCollection("users", {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["email", "roles", "createdAt", "updatedAt"],
      properties: {
        email: {
          bsonType: "string",
          pattern: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$",
          description: "must be a valid email and is required"
        },
        fullName: {
          bsonType: "string"
        },
        roles: {
          bsonType: "array",
          minItems: 1,
          items: {
            bsonType: "string",
            enum: ["student", "teacher", "admin"]
          }
        },
        refreshTokens: {
          bsonType: "array",
          items: {
            bsonType: "string"
          }
        },
        isBlocked: {
          bsonType: "bool"
        },
        externalAuth: {
          bsonType: "object",
          properties: {
            github: {
              bsonType: "object",
              properties: {
                id: { bsonType: "string" },
                email: { bsonType: "string" }
              }
            },
            yandex: {
              bsonType: "object",
              properties: {
                id: { bsonType: "string" },
                email: { bsonType: "string" }
              }
            }
          }
        },
        createdAt: {
          bsonType: "date"
        },
        updatedAt: {
          bsonType: "date"
        }
      }
    }
  }
});

// Создание индексов
db.users.createIndex({ email: 1 }, { unique: true });
db.users.createIndex({ roles: 1 });
db.users.createIndex({ "externalAuth.github.id": 1 });
db.users.createIndex({ "externalAuth.yandex.id": 1 });

// TTL индекс для автоматического удаления анонимных пользователей через 30 дней
db.users.createIndex(
  { createdAt: 1 },
  {
    expireAfterSeconds: 2592000, // 30 дней
    partialFilterExpression: {
      fullName: { $regex: /^Аноним\d+$/ }
    }
  }
);

// Вставка системного администратора (если нет)
var adminUser = {
  email: "admin@example.com",
  fullName: "System Administrator",
  roles: ["admin"],
  refreshTokens: [],
  isBlocked: false,
  externalAuth: {
    github: { id: null, email: null },
    yandex: { id: null, email: null }
  },
  createdAt: new Date(),
  updatedAt: new Date()
};

var existingAdmin = db.users.findOne({ email: "admin@example.com" });
if (!existingAdmin) {
  db.users.insertOne(adminUser);
  print("Admin user created");
} else {
  print("Admin user already exists");
}