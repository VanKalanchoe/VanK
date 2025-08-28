@echo off
set SCRIPT_DIR=%~dp0
set SCRIPTCORE_PATH=%SCRIPT_DIR%..\build\win\VanK-Editor\Resources\Scripts\VanK-ScriptCore.dll

:: Set the path to vcpkg (EDIT this to your actual vcpkg path)
set VCPKG_ROOT=%SCRIPT_DIR%..\VanK\Vendor\vcpkg

:: Check if vcpkg.exe exists
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo vcpkg not found, cloning and bootstrapping...
    if not exist "%VCPKG_ROOT%" (
        mkdir "%VCPKG_ROOT%"
    )
    pushd "%VCPKG_ROOT%"
    git clone https://github.com/microsoft/vcpkg.git .
    call bootstrap-vcpkg.bat
    popd
) else (
    echo vcpkg found at "%VCPKG_ROOT%\vcpkg.exe"
)

set PATH=%VCPKG_ROOT%;%PATH%

:: Install freetype with vcpkg if missing
"%VCPKG_ROOT%\vcpkg.exe" install freetype:x64-windows --clean-after-build

"%VCPKG_ROOT%\vcpkg.exe" install skia:x64-windows --clean-after-build

cmake -S .. -B ..\build\win -G "Visual Studio 17 2022" -A x64 -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" -DSCRIPTCORE_PATH="%SCRIPTCORE_PATH%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DMSDFGEN_BUILD_STANDALONE=OFF

pause