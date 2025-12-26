# fix_imports.ps1
Write-Host "Fixing imports in Go files..." -ForegroundColor Green

# Список файлов для проверки/исправления
$files = Get-ChildItem -Path . -Recurse -Include "*.go" | Where-Object { $_.FullName -notlike "*\vendor\*" }

foreach ($file in $files) {
    Write-Host "Checking $($file.Name)..." -ForegroundColor Cyan
    
    $content = Get-Content $file.FullName -Raw
    
    # Заменяем старые импорты
    $content = $content -replace '"auth-service/', '"test-system/auth/'
    $content = $content -replace 'package auth-service', 'package auth'
    
    Set-Content $file.FullName $content -Encoding UTF8
}

Write-Host "Done! All imports fixed." -ForegroundColor Green