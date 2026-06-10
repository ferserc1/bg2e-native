echo off

REM check if the first and second arguments are set
if "%2"=="" (
    echo "Usage: build.bat <shaders_dir> <out_dir>"
    exit /b 1
)

set SHADERS_DIR=%~f1
set OUT_DIR=%~f2
set GLSLANG=%VULKAN_SDK%\Bin\glslang.exe

pushd .
cd %SHADERS_DIR%

if not exist "%OUT_DIR%" (
    mkdir "%OUT_DIR%"
)

echo "Building shaders on directory %OUT_DIR%"
for /f "tokens=*" %%f in ('dir /b /a-d') do (
    if "%%~xf"==".glsl" (
        echo %GLSLANG% -V %%f -o %OUT_DIR%\%%~nf.spv
        %GLSLANG% -V %%f -o %OUT_DIR%\%%~nf.spv
    )
)

popd
