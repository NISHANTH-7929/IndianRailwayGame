@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S6\Evidence
rem Run Test-S6.cmd first to produce the matching pure simulation object files.
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /I ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation ..\Tools\Inspect-S6Checkpoint.cpp RailKernel.obj RailKernelTests.obj RailHandling.obj RailHandlingTests.obj RailOperations.obj RailOperationsTests.obj RailAIDriver.obj RailOperationsAI.obj RailAITests.obj RailContent.obj RailSnapshotRecord.obj RailWorldSnapshot.obj RailCheckpointStore.obj /Fe:S6Inspect.exe >inspector-build.log 2>&1
exit /b %errorlevel%
