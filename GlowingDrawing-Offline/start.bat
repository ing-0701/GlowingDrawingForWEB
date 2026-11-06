@echo off
cd /d "%~dp0"

if not exist "dist\index.html" (
    echo echo ERROR: dist\index.html was not found. Extract the entire ZIP first.
    pause
    exit /b 1
)

start "GlowingDrawing Server" /min caddy.exe run --config Caddyfile

timeout /t 2 /nobreak >nul

start "" "http://127.0.0.1:8765/"