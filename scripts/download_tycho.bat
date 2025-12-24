@echo off
setlocal

REM Check that user provided output directory
if "%~1"=="" (
    echo Error: Output directory is required.
    echo Usage: %~nx0 ^<output_directory^>
    echo Example: %~nx0 C:\Users\user\data_sets\
    exit /b 1
)

set "BASE_DIR=%~1"
if "%BASE_DIR:~-1%"=="\" set "BASE_DIR=%BASE_DIR:~0,-1%"

set "OUTPUT_DIR=%BASE_DIR%\tycho2"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"


REM List of files
set files=tyc2.dat.00 tyc2.dat.01 tyc2.dat.02 tyc2.dat.03 tyc2.dat.04^
 tyc2.dat.05 tyc2.dat.06 tyc2.dat.07 tyc2.dat.08 tyc2.dat.09 tyc2.dat.10^
 tyc2.dat.11 tyc2.dat.12 tyc2.dat.13 tyc2.dat.14 tyc2.dat.15 tyc2.dat.16^
 tyc2.dat.17 tyc2.dat.18 tyc2.dat.19

REM Base URL
set base_url=https://cdsarc.cds.unistra.fr/viz-bin/nph-Cat/txt?I/259

REM Loop through each file
for %%f in (%files%) do (
    echo Downloading %%f...
    curl -L "%base_url%/%%f.gz" -o "%OUTPUT_DIR%\%%f"
)

echo.
echo All files downloaded successfully to: %OUTPUT_DIR%
endlocal