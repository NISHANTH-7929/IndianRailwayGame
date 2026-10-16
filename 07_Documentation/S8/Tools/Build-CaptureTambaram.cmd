@echo off
call D:\DeveloperTools\VS2022BuildTools\VC\Auxiliary\Build\vcvars64.bat >nul
cd /d D:\Project\IndianRailwayGame\07_Documentation\S8\Tools
cl /nologo /EHsc CaptureTambaramMain.cpp /Fe:CaptureTambaram.exe
exit /b %errorlevel%
