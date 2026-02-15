Write-Host "🚀 Запуск тестовой системы..." -ForegroundColor Green

# 1. Остановите и удалите все существующие контейнеры
Write-Host "1. Очистка старых контейнеров..." -ForegroundColor Yellow
docker-compose down 2>$null
docker rm -f $(docker ps -aq) 2>$null

# 2. Запустите базы данных
Write-Host "2. Запуск баз данных..." -ForegroundColor Yellow
docker-compose up -d postgres mongodb redis

# Ждем запуска баз данных
Write-Host "   Ожидание запуска баз данных (15 секунд)..." -ForegroundColor Gray
Start-Sleep -Seconds 15

# 3. Убедитесь, что web-client готов
Write-Host "3. Подготовка web-client..." -ForegroundColor Yellow
if (Test-Path "web-client\Dockerfile") {
    Write-Host "   Сборка web-client..." -ForegroundColor Gray
    docker-compose -f docker-compose.dev.yml build web-client 2>$null
    docker-compose -f docker-compose.dev.yml up -d web-client 2>$null
} else {
    Write-Host "   Использую временный web-client..." -ForegroundColor Gray
    docker run -d --name web-temp -p 3000:80 --network test-system_test-network nginx:alpine 2>$null
}

# 4. Создайте простой auth-service
Write-Host "4. Запуск auth-service..." -ForegroundColor Yellow
docker run -d --name auth-simple `
  --network test-system_test-network `
  -p 8081:80 `
  nginx:alpine `
  sh -c "echo 'Auth Service' > /usr/share/nginx/html/index.html && nginx -g 'daemon off;'"

# 5. Создайте простой core-service
Write-Host "5. Запуск core-service..." -ForegroundColor Yellow
docker run -d --name core-simple `
  --network test-system_test-network `
  -p 8082:8082 `
  alpine:latest `
  sh -c "echo 'Core Service placeholder' && while true; do echo -e 'HTTP/1.1 200 OK\r\n\r\nCore Service' | nc -l -p 8082; sleep 1; done"

# 6. Создайте простой telegram-bot
Write-Host "6. Запуск telegram-bot..." -ForegroundColor Yellow
docker run -d --name telegram-simple `
  --network test-system_test-network `
  -p 8083:8083 `
  alpine:latest `
  sh -c "echo 'Telegram Bot placeholder' && while true; do echo -e 'HTTP/1.1 200 OK\r\n\r\nTelegram Bot' | nc -l -p 8083; sleep 1; done"

# 7. Проверка всех сервисов
Write-Host "`n✅ Тестовая система запущена!" -ForegroundColor Green
Write-Host ""
Write-Host "🌐 Доступные сервисы:" -ForegroundColor Cyan
Write-Host "   • Web Client:     http://localhost:3000" -ForegroundColor White
Write-Host "   • Auth Service:   http://localhost:8081" -ForegroundColor White
Write-Host "   • Core Service:   http://localhost:8082" -ForegroundColor White
Write-Host "   • Telegram Bot:   http://localhost:8083" -ForegroundColor White
Write-Host ""
Write-Host "🗄️  Базы данных:" -ForegroundColor Cyan
Write-Host "   • PostgreSQL:     localhost:5432" -ForegroundColor White
Write-Host "   • MongoDB:        localhost:27017" -ForegroundColor White
Write-Host "   • Redis:          localhost:6379" -ForegroundColor White
Write-Host ""
Write-Host "📊 Статус контейнеров:" -ForegroundColor Cyan
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
Write-Host ""
Write-Host "🔍 Проверка доступности..." -ForegroundColor Cyan

# Функция для проверки HTTP сервисов
function Test-Service {
    param($Name, $Url)
    
    try {
        $response = Invoke-WebRequest -Uri $Url -TimeoutSec 3 -ErrorAction SilentlyContinue
        if ($response.StatusCode -eq 200) {
            Write-Host "   ✅ $Name доступен" -ForegroundColor Green
            return $true
        }
    } catch {
        try {
            # Попробуем через Test-NetConnection для портов баз данных
            $port = ($Url -split ':')[-1]
            $result = Test-NetConnection -ComputerName localhost -Port $port -WarningAction SilentlyContinue -ErrorAction SilentlyContinue
            if ($result.TcpTestSucceeded) {
                Write-Host "   ✅ $Name доступен (порт $port)" -ForegroundColor Green
                return $true
            }
        } catch {
            Write-Host "   ❌ $Name недоступен" -ForegroundColor Red
            return $false
        }
    }
    Write-Host "   ❌ $Name недоступен" -ForegroundColor Red
    return $false
}

# Проверка
Write-Host "   Проверка сервисов..." -ForegroundColor Gray
Test-Service "PostgreSQL" "localhost:5432"
Test-Service "MongoDB" "localhost:27017"
Test-Service "Redis" "localhost:6379"
Test-Service "Web Client" "http://localhost:3000"
Test-Service "Auth Service" "http://localhost:8081"
Test-Service "Core Service" "http://localhost:8082"
Test-Service "Telegram Bot" "http://localhost:8083"

Write-Host ""
Write-Host "📝 Дальнейшие шаги:" -ForegroundColor Yellow
Write-Host "   1. Проверьте логи контейнеров: docker logs <имя_контейнера>" -ForegroundColor Gray
Write-Host "   2. Для разработки: добавьте реальные Dockerfile в папки сервисов" -ForegroundColor Gray
Write-Host "   3. Для остановки: docker-compose down" -ForegroundColor Gray