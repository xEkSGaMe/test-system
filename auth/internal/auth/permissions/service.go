package permissions

import (
    "strings"
)

type Permission struct {
    Name        string
    Description string
    Resource    string
    Action      string
}

type Role struct {
    Name        string
    Description string
    Permissions []string
}

type PermissionService struct {
    roles       map[string]*Role
    permissions map[string]*Permission
}

var (
    StudentPermissions = []string{
        "user:self:read",
        "user:fullName:self:write",
        "test:available:read",
        "test:attempt:create",
        "test:self:results:read",
        "attempt:self:read",
        "question:self:read",
    }

    TeacherPermissions = append(StudentPermissions,
        "user:list:read",
        "course:add",
        "course:del",
        "course:info:write",
        "course:participants:manage",
        "test:create",
        "test:update",
        "test:delete",
        "test:quest:add",
        "test:quest:update",
        "test:quest:del",
        "test:answer:read",
        "quest:create",
        "quest:update",
        "quest:del",
        "attempt:all:read",
        "attempt:results:read",
    )

    AdminPermissions = append(TeacherPermissions,
        "user:fullName:write",
        "user:roles:write",
        "user:delete",
        "system:settings:write",
    )
)

func NewPermissionService() *PermissionService {
    ps := &PermissionService{
        roles:       make(map[string]*Role),
        permissions: make(map[string]*Permission),
    }

    ps.initRoles()
    ps.initPermissions()

    return ps
}

func (ps *PermissionService) initRoles() {
    ps.roles["student"] = &Role{
        Name:        "student",
        Description: "Студент",
        Permissions: StudentPermissions,
    }

    ps.roles["teacher"] = &Role{
        Name:        "teacher",
        Description: "Преподаватель",
        Permissions: TeacherPermissions,
    }

    ps.roles["admin"] = &Role{
        Name:        "admin",
        Description: "Администратор",
        Permissions: AdminPermissions,
    }
}

func (ps *PermissionService) initPermissions() {
    allPermissions := make(map[string]bool)

    for _, role := range ps.roles {
        for _, permName := range role.Permissions {
            allPermissions[permName] = true
        }
    }

    for permName := range allPermissions {
        parts := strings.Split(permName, ":")
        if len(parts) >= 2 {
            ps.permissions[permName] = &Permission{
                Name:        permName,
                Resource:    parts[0],
                Action:      strings.Join(parts[1:], ":"),
                Description: ps.getPermissionDescription(permName),
            }
        }
    }
}

func (ps *PermissionService) getPermissionDescription(name string) string {
    descriptions := map[string]string{
        "user:self:read":              "Просмотр своих данных",
        "user:fullName:self:write":    "Изменение своего ФИО",
        "user:list:read":              "Просмотр списка пользователей",
        "user:fullName:write":         "Изменение ФИО любого пользователя",
        "user:roles:write":            "Изменение ролей пользователей",
        "user:delete":                 "Удаление пользователей",
        "course:add":                  "Добавление курса",
        "course:del":                  "Удаление курса",
        "course:info:write":           "Изменение информации о курсе",
        "course:participants:manage":  "Управление участниками курса",
        "test:available:read":         "Просмотр доступных тестов",
        "test:create":                 "Создание теста",
        "test:update":                 "Обновление теста",
        "test:delete":                 "Удаление теста",
        "test:quest:add":              "Добавление вопроса в тест",
        "test:quest:update":           "Обновление вопроса в тесте",
        "test:quest:del":              "Удаление вопроса из теста",
        "test:answer:read":            "Чтение ответов на тест",
        "test:attempt:create":         "Создание попытки прохождения теста",
        "test:self:results:read":      "Просмотр своих результатов теста",
        "quest:create":                "Создание вопроса",
        "quest:update":                "Обновление вопроса",
        "quest:del":                   "Удаление вопроса",
        "attempt:self:read":           "Просмотр своих попыток",
        "attempt:all:read":            "Просмотр всех попыток",
        "attempt:results:read":        "Чтение результатов попыток",
        "question:self:read":          "Чтение своих вопросов",
        "system:settings:write":       "Изменение системных настроек",
    }

    if desc, exists := descriptions[name]; exists {
        return desc
    }
    return "Разрешение: " + name
}

func (ps *PermissionService) GetPermissionsForRoles(roles []string) []string {
    result := make([]string, 0)
    seen := make(map[string]bool)

    for _, roleName := range roles {
        if role, exists := ps.roles[roleName]; exists {
            for _, perm := range role.Permissions {
                if !seen[perm] {
                    seen[perm] = true
                    result = append(result, perm)
                }
            }
        }
    }

    return result
}

func (ps *PermissionService) HasPermission(userRoles []string, permission string) bool {
    for _, roleName := range userRoles {
        if role, exists := ps.roles[roleName]; exists {
            for _, perm := range role.Permissions {
                if perm == permission {
                    return true
                }
            }
        }
    }
    return false
}

func (ps *PermissionService) ValidatePermission(permission string) bool {
    _, exists := ps.permissions[permission]
    return exists
}

func (ps *PermissionService) GetDefaultPermissions(role string) []string {
    if r, exists := ps.roles[role]; exists {
        return r.Permissions
    }
    return []string{}
}