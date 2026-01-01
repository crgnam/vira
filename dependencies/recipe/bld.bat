@echo off

echo Building %PKG_NAME% version %PKG_VERSION%

cmake -B build %CMAKE_ARGS%
if errorlevel 1 exit 1

cmake --build build --config Release --parallel %CPU_COUNT%
if errorlevel 1 exit 1

cmake --install build --config Release
if errorlevel 1 exit 1