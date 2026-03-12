@echo off
REM ============================================================================
REM  MouseDebouncer - Install/Uninstall Shortcuts
REM  Run as Administrator for Start Menu shortcuts.
REM  Startup folder shortcuts do not require elevation.
REM ============================================================================
setlocal enabledelayedexpansion

set "EXE_DIR=%~dp0"
set "EXE_PATH=%EXE_DIR%MouseDebouncer.exe"
set "APP_NAME=Mouse Debouncer"
set "STARTUP_FOLDER=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
set "STARTMENU_FOLDER=%APPDATA%\Microsoft\Windows\Start Menu\Programs"

if not exist "%EXE_PATH%" (
    echo ERROR: MouseDebouncer.exe not found in %EXE_DIR%
    echo        Run this script from the same directory as the executable.
    pause
    exit /b 1
)

echo.
echo  MouseDebouncer Shortcut Manager
echo  ================================
echo.
echo  1) Create Start Menu shortcut
echo  2) Add to Startup (launches on login with 10s delay)
echo  3) Both (Start Menu + Startup)
echo  4) Remove all shortcuts
echo  5) Exit
echo.
set /p CHOICE="  Select option [1-5]: "

if "%CHOICE%"=="1" goto :startmenu
if "%CHOICE%"=="2" goto :startup
if "%CHOICE%"=="3" goto :both
if "%CHOICE%"=="4" goto :remove
if "%CHOICE%"=="5" goto :eof
echo Invalid choice.
pause
goto :eof

:both
call :startmenu_impl
call :startup_impl
echo.
echo Done. Start Menu and Startup shortcuts created.
pause
goto :eof

:startmenu
call :startmenu_impl
echo.
echo Done. Start Menu shortcut created.
pause
goto :eof

:startup
call :startup_impl
echo.
echo Done. Startup shortcut created (10 second delay on login).
pause
goto :eof

:remove
if exist "%STARTMENU_FOLDER%\%APP_NAME%.lnk" (
    del "%STARTMENU_FOLDER%\%APP_NAME%.lnk"
    echo Removed Start Menu shortcut.
) else (
    echo No Start Menu shortcut found.
)
if exist "%STARTUP_FOLDER%\%APP_NAME%.lnk" (
    del "%STARTUP_FOLDER%\%APP_NAME%.lnk"
    echo Removed Startup shortcut.
) else (
    echo No Startup shortcut found.
)
REM Also remove the scheduled task if it exists
schtasks /query /tn "MouseDebouncer" >nul 2>&1
if !errorlevel! equ 0 (
    schtasks /delete /tn "MouseDebouncer" /f >nul 2>&1
    echo Removed scheduled task.
)
echo.
echo Done.
pause
goto :eof

REM --- Implementation subroutines ---

:startmenu_impl
echo Creating Start Menu shortcut...
powershell -NoProfile -Command ^
    "$ws = New-Object -ComObject WScript.Shell; ^
     $sc = $ws.CreateShortcut('%STARTMENU_FOLDER%\%APP_NAME%.lnk'); ^
     $sc.TargetPath = '%EXE_PATH%'; ^
     $sc.WorkingDirectory = '%EXE_DIR%'; ^
     $sc.Description = 'Mouse Debouncer - Filters phantom double-clicks'; ^
     $sc.Save()"
goto :eof

:startup_impl
echo Creating Startup entry with 10 second delay...
REM Use Task Scheduler for delayed startup (cleaner than a shortcut with sleep)
schtasks /create /tn "MouseDebouncer" ^
    /tr "\"%EXE_PATH%\"" ^
    /sc onlogon ^
    /delay 0000:10 ^
    /rl limited ^
    /f >nul 2>&1
if !errorlevel! equ 0 (
    echo Created scheduled task "MouseDebouncer" (runs on logon, 10s delay^).
) else (
    echo WARNING: Could not create scheduled task. Falling back to Startup folder shortcut.
    powershell -NoProfile -Command ^
        "$ws = New-Object -ComObject WScript.Shell; ^
         $sc = $ws.CreateShortcut('%STARTUP_FOLDER%\%APP_NAME%.lnk'); ^
         $sc.TargetPath = '%EXE_PATH%'; ^
         $sc.WorkingDirectory = '%EXE_DIR%'; ^
         $sc.Description = 'Mouse Debouncer - Filters phantom double-clicks'; ^
         $sc.Save()"
    echo Created Startup folder shortcut (no delay^).
)
goto :eof
