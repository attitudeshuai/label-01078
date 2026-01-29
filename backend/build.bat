@echo off
echo Building GCD Solver Service...

if not exist build mkdir build
cd build

cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo CMake configuration failed. Trying with Visual Studio...
    cmake .. -DCMAKE_BUILD_TYPE=Release
)

cmake --build . --config Release

if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo Executable: build\gcd_server.exe
echo.
echo To run: build\gcd_server.exe [port]
echo Default port: 8080
