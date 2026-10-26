@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake was not found. Install Visual Studio 2022 with Desktop development with C++ and CMake tools.
  echo Open an x64 Native Tools Command Prompt for VS 2022, then run this file again.
  pause
  exit /b 1
)
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto failed
cmake --build build-vs --config Release --parallel
if errorlevel 1 goto failed
ctest --test-dir build-vs -C Release --output-on-failure
if errorlevel 1 goto failed
echo.
echo Build succeeded. Starting Lumenfall...
"%~dp0build-vs\bin\Release\Lumenfall.exe"
exit /b 0
:failed
echo.
echo Build failed. See the messages above and README.md.
pause
exit /b 1
