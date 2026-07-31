@echo off
setlocal
cd /d "%~dp0"

echo === Configuring and building (cmake) ===
if not exist build mkdir build
cd build
cmake .. -G "Ninja"
if errorlevel 1 goto :error
cmake --build .
if errorlevel 1 goto :error
cd ..

echo.
echo === Copying result ===
for /r build %%f in (ch32_flasher_demo.uf2) do copy /Y "%%f" . >nul
if not exist ch32_flasher_demo.uf2 goto :error

echo.
echo Done: ch32_flasher_demo.uf2 готов в корне проекта.
echo Залей его на Pico через BOOTSEL.
echo Прошивки (1.bin/2.bin/3.bin) и log.txt - на SD-карте, отдельно от этой сборки.
echo.
pause
exit /b 0

:error
echo.
echo *** BUILD FAILED - смотри текст ошибки выше ***
echo.
pause
exit /b 1
