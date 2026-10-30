@echo off
if "%~1"=="" exit /b 2
if exist "%~1" exit /b 3
mkdir "%~1"
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d "%~1"
set RAILSRC=..\..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation
cl /nologo /std:c++17 /EHsc /O2 /I %RAILSRC% %RAILSRC%\RailKernel.cpp ..\..\Tools\Export-DrivingProfile.cpp /Fe:DrivingProfile.exe >compile.log 2>&1
if errorlevel 1 exit /b 1
DrivingProfile.exe >profile.json
exit /b %errorlevel%
