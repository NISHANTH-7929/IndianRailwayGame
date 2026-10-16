@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S7\Evidence
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /I ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailService.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailServiceTests.cpp ..\Tools\S7Main.cpp RailKernel.obj RailKernelTests.obj RailHandling.obj RailHandlingTests.obj RailOperations.obj RailOperationsTests.obj RailAIDriver.obj RailOperationsAI.obj RailAITests.obj RailContent.obj RailSnapshotRecord.obj RailWorldSnapshot.obj RailCheckpointStore.obj /Fe:S7Tests.exe >incremental-compile.log 2>&1
if errorlevel 1 exit /b 1
S7Tests.exe >host-results.txt
exit /b %errorlevel%
