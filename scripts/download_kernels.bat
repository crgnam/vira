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

set "OUTPUT_DIR=%BASE_DIR%\kernels"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"



echo Downloading generic kernels
if not exist "%OUTPUT_DIR%\generic\lsk" mkdir "%OUTPUT_DIR%\generic\lsk"
curl -L -o "%OUTPUT_DIR%\generic\lsk\naif0012.tls" https://naif.jpl.nasa.gov/pub/naif/generic_kernels/lsk/naif0012.tls

if not exist "%OUTPUT_DIR%\generic\pck" mkdir "%OUTPUT_DIR%\generic\pck"
curl -L -o "%OUTPUT_DIR%\generic\pck\pck00010.tpc" https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/pck00010.tpc



echo.
echo Downloading ORX kernels required by vira_orex example
if not exist "%OUTPUT_DIR%\orx\ck" mkdir "%OUTPUT_DIR%\orx\ck"
curl -L -o "%OUTPUT_DIR%\orx\ck\orx_r_190205_190207_v01.bc" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/ck/orx_r_190205_190207_v01.bc

if not exist "%OUTPUT_DIR%\orx\fk" mkdir "%OUTPUT_DIR%\orx\fk"
curl -L -o "%OUTPUT_DIR%\orx\fk\orx_v03.tf" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/fk/orx_v03.tf

if not exist "%OUTPUT_DIR%\orx\pck" mkdir "%OUTPUT_DIR%\orx\pck"
curl -L -o "%OUTPUT_DIR%\orx\pck\bennu_v17.tpc" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/pck/bennu_v17.tpc

if not exist "%OUTPUT_DIR%\orx\sclk" mkdir "%OUTPUT_DIR%\orx\sclk"
curl -L -o "%OUTPUT_DIR%\orx\sclk\ORX_SCLKSCET.00043.tsc" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/sclk/ORX_SCLKSCET.00043.tsc

if not exist "%OUTPUT_DIR%\orx\spk" mkdir "%OUTPUT_DIR%\orx\spk"
curl -L -o "%OUTPUT_DIR%\orx\spk\orx_struct_v03.bsp" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/spk/orx_struct_v03.bsp
curl -L -o "%OUTPUT_DIR%\orx\spk\orx_181231_190305_190215_od099-N-M0D-P-M1D_v1.bsp" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/spk/orx_181231_190305_190215_od099-N-M0D-P-M1D_v1.bsp



echo.
echo Downloading Bennu kernels required by vira_orex example
if not exist "%OUTPUT_DIR%\bennu\dsk" mkdir "%OUTPUT_DIR%\bennu\dsk"
curl -L -o "%OUTPUT_DIR%\bennu\dsk\bennu_g_01680mm_alt_obj_0000n00000_v021.bds" https://naif.jpl.nasa.gov/pub/naif/ORX/kernels/dsk/bennu_g_01680mm_alt_obj_0000n00000_v021.bds



echo.
echo All files downloaded successfully to: %OUTPUT_DIR%
endlocal