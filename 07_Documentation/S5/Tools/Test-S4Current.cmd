@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S5\Evidence
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /I ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailKernel.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailKernelTests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailHandling.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailHandlingTests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailOperations.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailOperationsTests.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailAIDriver.cpp ..\..\..\03_UnrealProject\MyProject\Source\MyProject\Simulation\RailOperationsAI.cpp ..\..\S4\Tools\OperationsMain.cpp /Fe:S4CurrentTests.exe >s4-current-compile.log 2>&1
if errorlevel 1 exit /b 1
S4CurrentTests.exe >s4-current-tests.json
exit /b %errorlevel%




