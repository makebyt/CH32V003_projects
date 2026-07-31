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
for /r build %%f in (ch32_flasher_flashdrive.uf2) do copy /Y "%%f" . >nul
if not exist ch32_flasher_flashdrive.uf2 goto :error

echo.
echo Done: ch32_flasher_flashdrive.uf2 готов в корне проекта.
echo.
pause
exit /b 0

:error
echo.
echo *** BUILD FAILED ***
echo.
pause
exit /b 1
