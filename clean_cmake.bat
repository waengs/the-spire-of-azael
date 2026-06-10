@echo off
setlocal
echo Removing stale CMake build folders (fixes path / rename errors)...

set "ROOT=%~dp0"
cd /d "%ROOT%"

if exist "build" (
    echo   deleting build\
    rmdir /s /q "build"
)
if exist "out" (
    echo   deleting out\
    rmdir /s /q "out"
)
if exist "CMakeCache.txt" del /q "CMakeCache.txt"
if exist "CMakeFiles" rmdir /s /q "CMakeFiles"

echo.
echo Done. Reconfigure from ONE folder only:
echo   %ROOT%
echo.
echo Visual Studio: Project -^> Delete Cache and Reconfigure
echo Or run: compile.bat
echo.
endlocal
