@echo off
set SCRIPT_DIR=%~dp0
set SCRIPTCORE_PATH=%SCRIPT_DIR%..\build\win\VanK-Editor\Resources\Scripts\VanK-ScriptCore.dll

cmake -DUSE_SHADER_LANGUAGE=SLANG -S .. -B ..\build\win -G "Visual Studio 17 2022" -A x64 -DSCRIPTCORE_PATH="%SCRIPTCORE_PATH%"
pause