
echo off

set OUT=..\build\shaders
set COMPILE=%VULKAN_SDK%\Bin\glslangValidator.exe

if not exist "%OUT%" mkdir %OUT%

for %%f in (shaders/*) do (
    %COMPILE% -V shaders/%%f -o %OUT%\%%f.spv
)
