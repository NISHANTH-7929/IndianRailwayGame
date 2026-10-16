@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S8\Evidence\EgmoreTambaram
set RAILSRC=..\..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation
cl /nologo /std:c++17 /EHsc /O2 /I %RAILSRC% %RAILSRC%\RailKernel.cpp %RAILSRC%\RailKernelTests.cpp %RAILSRC%\RailHandling.cpp %RAILSRC%\RailOperations.cpp %RAILSRC%\RailAIDriver.cpp %RAILSRC%\RailOperationsAI.cpp %RAILSRC%\RailContent.cpp %RAILSRC%\RailSnapshotRecord.cpp %RAILSRC%\RailWorldSnapshot.cpp %RAILSRC%\RailCheckpointStore.cpp %RAILSRC%\RailService.cpp %RAILSRC%\RailTambaramTests.cpp ..\..\Tools\TambaramMain.cpp /Fe:TambaramJourney.exe >journey-compile.log 2>&1
if errorlevel 1 exit /b 1
TambaramJourney.exe >journey-results.json
exit /b %errorlevel%
