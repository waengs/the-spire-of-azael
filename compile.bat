@echo off
setlocal EnableExtensions

echo Building The Spire of Azael (SDL2 + OpenGL + Dear ImGui + SQLite)...

rem %~dp0 ends with '\' — a trailing backslash before " escapes the closing quote and breaks cmake -S/-B
set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "BUILD_DIR=%PROJECT_ROOT%\build"

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

echo.
echo Build complete:
echo   Game:     build\Release\SpireOfAzael.exe
echo   Console:  build\Release\SpireOfAzaelConsole.exe
echo   Saves:    spire_save.db (created at runtime)
echo.

endlocal
