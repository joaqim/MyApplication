@echo off
pushd ..
@echo on
call build.bat %1
@echo off
popd
EXIT /B %ERRORLEVEL%