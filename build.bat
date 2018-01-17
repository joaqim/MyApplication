@echo off
setlocal
cd /D %0\..
set MSBUILD_EXE="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\amd64\MSBuild.exe"
set path=P:\sys\bin;%path%
@echo off

if "%~1"=="run" (
	pushd bin\Debug
	@echo on
	echo Starting magnum-shadows.exe
    magnum-shadows.exe scene2.blend
	@echo off
	popd
	GOTO :EXIT
)

if not defined DEV_ENV_DIR (
	REM call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
	REM call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
	REM p:
	REM cd /D %0\..
)
set DEV_ENV_DIR=???
IF NOT EXIST build mkdir build
pushd build

set BUILD_TYPE=Debug

set CMAKE_FLAGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if "%~1"=="" (
	@echo on
	cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX=p:/sys -T v141 .. %CMAKE_FLAGS%	
)
REM %MSBUILD_EXE% MyApplication.sln /nologo /v:q /property:GenerateFullPaths=true /property:Configuration=%BUILD_TYPE% /p:PlatformToolset=v141
@echo on
cmake --build .
REM %MSBUILD_EXE% MyApplication.sln /nologo /v:q /property:GenerateFullPaths=true /property:Configuration=%BUILD_TYPE% /p:PlatformToolset=v141
@echo off
set BUILD_STATUS=%ERRORLEVEL%
REM devenv vorp.vcxproj /build Debug
REM MSBuild.exe vorp.vcxproj /property:Configuration=Debug

GOTO :POPD
pushd ..\bin\Debug
@echo off
if %BUILD_STATUS% == 0 (
    echo Starting magnum-shadows.exe
    magnum-shadows.exe scene2.blend
)

popd
:POPD
popd
:EXIT
endlocal
EXIT /B %ERRORLEVEL%
