@echo off
rem ============================================================
rem PunkUI build: GacGen resource generation + MSBuild
rem Usage: build.bat [Release^|Debug]  (default Release)
rem ============================================================
setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Release

set GACGEN=%~dp0..\gacui\Tools\Executables\Release\GacGen.exe
set MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe

if not exist "%GACGEN%" (
  echo [ERROR] GacGen.exe not found. Build gacui\Tools\Executables\Executables.sln first:
  echo         msbuild Executables.sln /p:Configuration=Release /p:Platform=x86 /p:PlatformToolset=v143
  exit /b 1
)

echo === [1/3] Generating PunkSkin resource ===
pushd "%~dp0Skin"
if not exist Source mkdir Source
cmd /c ""%GACGEN%" /C64 Resource.xml"
if errorlevel 1 goto :fail
if exist "Resource.xml.log\x64\Errors.txt" (
  type "Resource.xml.log\x64\Errors.txt"
  goto :fail
)
popd

echo === [2/3] Generating app resource ===
pushd "%~dp0UI"
if not exist Source mkdir Source
if not exist ..\UIRes mkdir ..\UIRes
cmd /c ""%GACGEN%" /C64 Resource.xml"
if errorlevel 1 goto :fail
if exist "Resource.xml.log\x64\Errors.txt" (
  type "Resource.xml.log\x64\Errors.txt"
  goto :fail
)
popd

echo === [3/3] MSBuild %CONFIG% x64 ===
"%MSBUILD%" "%~dp0PunkUI.vcxproj" /p:Configuration=%CONFIG% /p:Platform=x64 /m /v:m /nologo
if errorlevel 1 goto :fail

echo.
echo BUILD OK: PunkUI\Bin\x64\%CONFIG%\PunkUI.exe
echo Run it with working directory = PunkUI (reads UIRes\PunkUI.bin)
exit /b 0

:fail
popd 2>nul
echo BUILD FAILED
exit /b 1
