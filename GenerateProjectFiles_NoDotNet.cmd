@echo off
setlocal

set "UE_ENGINE_DIR=D:\ue engine\UE_5.4\Engine"
set "PROJECT_FILE="

for %%f in ("%~dp0*.uproject") do (
    set "PROJECT_FILE=%%~ff"
    goto FoundProject
)

echo ERROR: No .uproject file found next to this script.
exit /b 1

:FoundProject

call "%UE_ENGINE_DIR%\Build\BatchFiles\GetDotnetPath.bat"
if errorlevel 1 exit /b 1

dotnet "%UE_ENGINE_DIR%\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" -projectfiles -project="%PROJECT_FILE%" -game -engine -NoDotNet
exit /b %ERRORLEVEL%
