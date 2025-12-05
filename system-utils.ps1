# Системные утилиты для тестовой системы
function Show-SystemStatus {
    Write-Host "=== СТАТУС ТЕСТОВОЙ СИСТЕМЫ ===" -ForegroundColor Cyan
    
    # Контейнеры
    Write-Host "`nКонтейнеры:" -ForegroundColor Yellow
    docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
    
    # Проверка доступности
    Write-Host "`nПроверка доступности:" -ForegroundColor Yellow
    $services = @(
        @{Name="PostgreSQL"; Port=5432; Url="postgres://localhost:5432"},
        @{Name="MongoDB"; Port=27017; Url="mongodb://localhost:27017"},
        @{Name="Redis"; Port=6379; Url="redis://localhost:6379"},
        @{Name="Web Client"; Port=3000; Url="http://localhost:3000"},
        @{Name="Auth Service"; Port=8081; Url="http://localhost:8081"},
        @{Name="Core Service"; Port=8082; Url="http://localhost:8082"},
        @{Name="Telegram Bot"; Port=8083; Url="http://localhost:8083"}
    )
    
    foreach ($service in $services) {
        try {
            $result = Test-NetConnection -ComputerName localhost -Port $service.Port -WarningAction SilentlyContinue
            if ($result.TcpTestSucceeded) {
                Write-Host "  ✅ $($service.Name) (:$($service.Port))" -ForegroundColor Green
            } else {
                Write-Host "  ❌ $($service.Name) (:$($service.Port))" -ForegroundColor Red
            }
        } catch {
            Write-Host "  ❓ $($service.Name) (:$($service.Port)) - ошибка проверки" -ForegroundColor Yellow
        }
    }
}

function Restart-System {
    Write-Host "Перезапуск системы..." -ForegroundColor Yellow
    
    # Остановка сервисов
    docker stop web-client auth-service core-service telegram-bot 2>$null
    docker rm web-client auth-service core-service telegram-bot 2>$null
    
    # Запуск сервисов
    docker run -d --name web-client --network test-system_test-network -p 3000:80 nginx:alpine
    docker run -d --name auth-service --network test-system_test-network -p 8081:80 nginx:alpine
    docker run -d --name core-service --network test-system_test-network -p 8082:80 nginx:alpine
    docker run -d --name telegram-bot --network test-system_test-network -p 8083:80 nginx:alpine
    
    Write-Host "Система перезапущена!" -ForegroundColor Green
    Show-SystemStatus
}

function Check-Databases {
    Write-Host "Проверка баз данных..." -ForegroundColor Cyan
    
    Write-Host "`nPostgreSQL:" -ForegroundColor Yellow
    docker exec test-system-postgres psql -U admin -d test_system -c "SELECT version();" 2>&1 | ForEach-Object { "  $_" }
    
    Write-Host "`nMongoDB:" -ForegroundColor Yellow
    docker exec test-system-mongodb mongo --eval "db.version()" -u admin -p admin123 2>&1 | ForEach-Object { "  $_" }
    
    Write-Host "`nRedis:" -ForegroundColor Yellow
    docker exec test-system-redis redis-cli ping 2>&1 | ForEach-Object { "  $_" }
}

function Show-Logs {
    param(
        [string]$ServiceName
    )
    
    if ($ServiceName) {
        Write-Host "Логи $ServiceName :" -ForegroundColor Cyan
        docker logs $ServiceName --tail 20
    } else {
        Write-Host "Доступные сервисы:" -ForegroundColor Cyan
        docker ps --format "{{.Names}}" | ForEach-Object { "  - $_" }
        Write-Host "`nИспользование: Show-Logs <имя_сервиса>" -ForegroundColor Yellow
    }
}

# Экспорт функций
Export-ModuleMember -Function Show-SystemStatus, Restart-System, Check-Databases, Show-Logs