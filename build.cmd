@echo off
setlocal
if defined VSCMD_VER goto tools

set "SIGS_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%SIGS_VSWHERE%" goto missing_compiler
for /f "usebackq delims=" %%I in (`"%SIGS_VSWHERE%" -all -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "SIGS_VS_PATH=%%I"
if not defined SIGS_VS_PATH goto missing_compiler
if not exist "%SIGS_VS_PATH%\Common7\Tools\VsDevCmd.bat" goto missing_compiler
call "%SIGS_VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

:tools
if exist "%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "PATH=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"
if exist "%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" set "PATH=%VSINSTALLDIR%Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
where cl.exe >nul 2>nul
if errorlevel 1 goto missing_compiler
where cmake.exe >nul 2>nul
if errorlevel 1 goto missing_tools
where ninja.exe >nul 2>nul
if errorlevel 1 goto missing_tools
where ctest.exe >nul 2>nul
if errorlevel 1 goto missing_tools

cd /d "%~dp0"
if errorlevel 1 exit /b 1
cmake -P tools/prepare-build.cmake
if errorlevel 1 exit /b 1
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 goto configure_failed
cmake --build build
if errorlevel 1 exit /b 1
ctest --test-dir build --output-on-failure
exit /b %errorlevel%

:missing_compiler
echo ERROR: No usable Visual Studio C++ toolchain was found.
echo Install Visual Studio or Build Tools with Desktop development with C++.
echo You can also run this script from an x64 Native Tools command prompt.
exit /b 1

:missing_tools
echo ERROR: CMake, CTest and Ninja must be available.
echo Install the C++ CMake tools for Windows component in Visual Studio,
echo or install CMake and Ninja and add them to PATH.
exit /b 1

:configure_failed
echo ERROR: CMake configuration failed. See the error above.
echo If you changed toolchains, rename the build folder and run this script again.
exit /b 1
