# Исправьте кодировку start.bat
Set-Content -Path "start.bat" -Value @'
@echo off
chcp 65001 > nul
echo ====================================
echo Запуск системы тестирования
echo ====================================

if not exist "docker-compose.yml" (
    echo ERROR: docker-compose.yml not found!
    echo Run this script from test-system folder
    pause
    exit /b 1
)

echo 1. Stopping old containers...
docker-compose down

echo 2. Starting containers...
docker-compose up -d

echo 3. Waiting for databases...
timeout /t 10 /nobreak

echo 4. Checking status...
docker-compose ps

echo 5. Checking PostgreSQL...
docker-compose logs postgres --tail=2

echo.
echo ====================================
echo Access URLs:
echo Web interface: http://localhost
echo PostgreSQL: localhost:5432
echo MongoDB: localhost:27017
echo Redis: localhost:6379
echo ====================================
pause
'@ -Encoding ASCII