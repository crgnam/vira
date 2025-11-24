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

set "OUTPUT_DIR=%BASE_DIR%\albedo"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM List of files
set files=WAC_EMP_643NM_P900N0000_304P.IMG^
 WAC_EMP_643NM_P900S0000_304P.IMG^
 WAC_EMP_643NM_E300N2250_304P.IMG^
 WAC_EMP_643NM_E300N3150_304P.IMG^
 WAC_EMP_643NM_E300N0450_304P.IMG^
 WAC_EMP_643NM_E300N1350_304P.IMG^
 WAC_EMP_643NM_E300S2250_304P.IMG^
 WAC_EMP_643NM_E300S3150_304P.IMG^
 WAC_EMP_643NM_E300S0450_304P.IMG^
 WAC_EMP_643NM_E300S1350_304P.IMG

REM Base URL
set base_url=https://pds.lroc.im-ldi.com/data/LRO-L-LROC-5-RDR-V1.0/LROLRC_2001/DATA/MDR/WAC_EMP

for %%f in (%files%) do (
    echo Downloading %%f...
    curl -L "%base_url%/%%f" -o "%OUTPUT_DIR%\%%f"
)

echo.
echo All files downloaded successfully to: %OUTPUT_DIR%
endlocal