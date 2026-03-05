@echo off
setlocal

cd /d "%~dp0"

set "QT_PREFIX=C:\Qt\6.10.2\msvc2022_64"
if not "%~1"=="" set "QT_PREFIX=%~1"

if not exist "%QT_PREFIX%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [ERROR] Qt6Config.cmake not found under "%QT_PREFIX%".
    echo [HINT] Pass your Qt kit path as the first argument.
    echo        Example: build_and_launch.bat "C:\Qt\6.10.2\msvc2022_64"
    exit /b 1
)

echo [1/3] Configuring project...
cmake -S . -B build-msvc -DCMAKE_PREFIX_PATH="%QT_PREFIX%"
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b %errorlevel%
)

echo [2/3] Building Release...
cmake --build build-msvc --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b %errorlevel%
)

set "EXE=%CD%\build-msvc\Release\MinecraftLegacyLauncher.exe"
if not exist "%EXE%" (
    echo [ERROR] Executable not found: "%EXE%"
    exit /b 1
)

echo [3/3] Launching...
start "Minecraft Legacy Launcher" "%EXE%"
echo [OK] Launcher started.

endlocal
exit /b 0
