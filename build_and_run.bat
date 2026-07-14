@echo off
setlocal enabledelayedexpansion

rem Build (MSYS2 MinGW64) and run media-player-cpp.exe with bin\ as its cwd.
rem OF_ROOT resolves to ..\..\.. (this script must stay in the project root).

set "PROJECT_DIR=%~dp0"
set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

if not defined MSYS2_ROOT set "MSYS2_ROOT=C:\msys64"

if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
    echo [build_and_run] MSYS2 not found at "%MSYS2_ROOT%".
    echo [build_and_run] Set MSYS2_ROOT to your MSYS2 install and re-run.
    exit /b 1
)

echo [build_and_run] Building with MSYS2 MinGW64 ^(make Release^)...
call "%MSYS2_ROOT%\msys2_shell.cmd" -mingw64 -defterm -no-start -here -c "cd '%PROJECT_DIR%' && make Release"
if errorlevel 1 (
    echo [build_and_run] Build failed.
    exit /b 1
)

set "EXE=%PROJECT_DIR%\bin\media-player-cpp.exe"
if not exist "%EXE%" (
    echo [build_and_run] Build succeeded but "%EXE%" was not found.
    exit /b 1
)

echo [build_and_run] Launching media-player-cpp.exe ^(cwd=bin, data in bin\data^)...
pushd "%PROJECT_DIR%\bin"
media-player-cpp.exe
set "EXIT_CODE=%errorlevel%"
popd

exit /b %EXIT_CODE%
