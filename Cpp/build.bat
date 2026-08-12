@echo off
REM =========================================================================
REM ImageCut C++ Project Build Script (MSVC + CMake)
REM =========================================================================

echo [1/3] Setting up C++ CMake Build Directory...
if not exist build mkdir build

echo [2/3] Configuring CMake Project...
REM Set your Qt6 and OpenCV installation paths below if not in standard PATH:
REM set CMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2019_64;C:\opencv\build

"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed. Please ensure Qt6 and OpenCV C++ packages are installed.
    exit /b %ERRORLEVEL%
)

echo [3/3] Building Executable (Release Configuration)...
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release

if %ERRORLEVEL% EQ 0 (
    echo =========================================================================
    echo [SUCCESS] Executable built successfully: Cpp/build/Release/ImageCut.exe
    echo =========================================================================
) else (
    echo [ERROR] Compilation failed.
)
