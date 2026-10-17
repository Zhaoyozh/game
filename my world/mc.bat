@echo off
chcp 65001 > nul
cls
cd /d %~dp0

echo ================================
echo   Compiling...
echo ================================
echo.

W:\RedPanda-CPP\mingw64\bin\g++.exe -O2 -std=c++17 ^
    -static -static-libgcc -static-libstdc++ ^
    console.cpp ^
    blocks.cpp ^
    world.cpp ^
    ui.cpp ^
    save.cpp ^
    game.cpp ^
    multiplayer.cpp ^
    main.cpp ^
    -o mc.exe ^
    -luser32 -lws2_32

if errorlevel 1 (
    echo.
    echo FAILED - see errors above
) else (
    echo.
    echo OK - run mc.exe
)
echo.
pause
