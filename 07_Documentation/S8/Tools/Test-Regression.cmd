@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S8\Evidence
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /I ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailKernel.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailKernelTests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailHandling.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailHandlingTests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailOperations.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailOperationsTests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailAIDriver.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailOperationsAI.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailAITests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailContent.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailSnapshotRecord.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailWorldSnapshot.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailCheckpointStore.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailService.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailServiceTests.cpp ..\..\S7\Tools\S7Main.cpp /Fe:S7Tests.exe >host-compile.log 2>&1
if errorlevel 1 exit /b 1
S7Tests.exe >host-results.txt
exit /b %errorlevel%









