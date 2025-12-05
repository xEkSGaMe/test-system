Write-Host "🔍 Проверка работы системы тестирования" -ForegroundColor Cyan
Write-Host "==========================================="

# Проверка контейнеров
Write-Host "`n1. Статус контейнеров:" -ForegroundColor Yellow
docker-compose ps

# Проверка PostgreSQL
Write-Host "`n2. Проверка PostgreSQL:" -ForegroundColor Yellow
docker exec test-system-postgres psql -U admin -d test_system -c "SELECT version();" 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ PostgreSQL работает" -ForegroundColor Green
} else {
    Write-Host "❌ PostgreSQL не отвечает" -ForegroundColor Red
}

# Проверка MongoDB
Write-Host "`n3. Проверка MongoDB:" -ForegroundColor Yellow
docker exec test-system-mongodb mongosh --eval "db.version()" --quiet 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ MongoDB работает" -ForegroundColor Green
} else {
    Write-Host "❌ MongoDB не отвечает" -ForegroundColor Red
}

# Проверка Redis
Write-Host "`n4. Проверка Redis:" -ForegroundColor Yellow
docker exec test-system-redis redis-cli ping 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Redis работает" -ForegroundColor Green
} else {
    Write-Host "❌ Redis не отвечает" -ForegroundColor Red
}

Write-Host "`n===========================================" -ForegroundColor Cyan
Write-Host "✅ Проверка завершена" -ForegroundColor Green