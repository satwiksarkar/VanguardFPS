@echo off
REM =========================================================================
REM VanguardFPS - Local Co-op Multiplayer Launch Script (Windows)
REM Launches two side-by-side instances of VanguardFPS for instant LAN/Local testing
REM =========================================================================

echo [VANGUARD CO-OP] Initializing dual local instances...

REM Ensure Qt6 and MinGW runtime binaries are available in PATH
set PATH=C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%

set EXE_DIR=%~dp0build
set EXE_PATH=%EXE_DIR%\VanguardFPS.exe
if not exist "%EXE_PATH%" (
    set EXE_DIR=%~dp0build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug
    set EXE_PATH=%EXE_DIR%\VanguardFPS.exe
)

if not exist "%EXE_PATH%" (
    echo [ERROR] Could not find VanguardFPS.exe in build\ or build\Desktop_Qt_6_11_1_MinGW_64_bit_Debug\
    echo Please build the project first.
    pause
    exit /b 1
)

echo [VANGUARD CO-OP] Launching Instance 1 (Squad Leader)...
start "" /D "%EXE_DIR%" "%EXE_PATH%"

timeout /t 1 /nobreak >nul

echo [VANGUARD CO-OP] Launching Instance 2 (Squad Operative)...
start "" /D "%EXE_DIR%" "%EXE_PATH%"

echo [VANGUARD CO-OP] Both instances launched successfully.
echo In Instance 1, click "HOST CO-OP SQUAD LOBBY".
echo In Instance 2, select the discovered lobby in the list and click "QUICK JOIN".

