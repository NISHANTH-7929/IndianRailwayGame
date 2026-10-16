@echo off
if "%~1"=="" exit /b 2
if exist "%~1" exit /b 3
mkdir "%~1"
if errorlevel 1 exit /b 4
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d "%~1"
set RAILSRC=D:\Project\IndianRailwayGame\03_UnrealProject\MyProject\Source\MyProject\Simulation
cl /nologo /std:c++17 /EHsc /O2 /I "%RAILSRC%" "%RAILSRC%\RailKernel.cpp" "%RAILSRC%\RailKernelTests.cpp" "%RAILSRC%\RailHandling.cpp" "%RAILSRC%\RailOperations.cpp" "%RAILSRC%\RailAIDriver.cpp" "%RAILSRC%\RailOperationsAI.cpp" "%RAILSRC%\RailDispatchTests.cpp" D:\Project\IndianRailwayGame\07_Documentation\S9\Tools\DispatchMain.cpp /Fe:DispatchTests.exe >compile.log 2>&1
if errorlevel 1 exit /b 1
DispatchTests.exe >results.json
exit /b %errorlevel%

