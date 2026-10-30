@echo off
if "%~1"=="" exit /b 2
if exist "%~1" exit /b 3
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
set RAILSRC=%~dp0..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation
set RAILOBJ=%~1-compiler
if exist "%RAILOBJ%" exit /b 3
mkdir "%RAILOBJ%"
cd /d "%RAILOBJ%"
cl /nologo /std:c++17 /EHsc /O2 /I "%RAILSRC%" "%RAILSRC%\RailKernel.cpp" "%RAILSRC%\RailKernelTests.cpp" "%RAILSRC%\RailHandling.cpp" "%RAILSRC%\RailOperations.cpp" "%RAILSRC%\RailAIDriver.cpp" "%RAILSRC%\RailOperationsAI.cpp" "%RAILSRC%\RailContent.cpp" "%RAILSRC%\RailSnapshotRecord.cpp" "%RAILSRC%\RailWorldSnapshot.cpp" "%RAILSRC%\RailCheckpointStore.cpp" "%RAILSRC%\RailService.cpp" "%~dp0Export-SceneryReview.cpp" /Fe:ExportScenery.exe >compile.log 2>&1
if errorlevel 1 exit /b 1
ExportScenery.exe "%~1" >export.log 2>&1
exit /b %errorlevel%
