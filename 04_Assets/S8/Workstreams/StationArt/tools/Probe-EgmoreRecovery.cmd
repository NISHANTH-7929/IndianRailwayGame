@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S8\Workstreams\StationArt\Device\oct09-egmore-recovery-probe1
set "SIM=D:\Project\IndianRailwayGame\03_UnrealProject\MyProject\Source\MyProject\Simulation"
cl /nologo /std:c++17 /EHsc /W4 /O2 /I "%SIM%" "%SIM%\RailKernel.cpp" "%SIM%\RailHandling.cpp" "%SIM%\RailOperations.cpp" "%SIM%\RailAIDriver.cpp" "%SIM%\RailOperationsAI.cpp" "%SIM%\RailContent.cpp" "%SIM%\RailSnapshotRecord.cpp" "%SIM%\RailWorldSnapshot.cpp" "%SIM%\RailCheckpointStore.cpp" "%SIM%\RailService.cpp" D:\Project\IndianRailwayGame\04_Assets\S8\Workstreams\StationArt\tools\egmore_recovery_probe.cpp /Fe:probe.exe >compile.log 2>&1
if errorlevel 1 exit /b 1
probe.exe >result.txt
exit /b %errorlevel%
