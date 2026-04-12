@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d D:\xieyi\build_msvc
D:\Qt\5.15.2\msvc2019_64\bin\qmake.exe ..\GSK988Tool.pro -spec win32-msvc "CONFIG+=release"
if %ERRORLEVEL% NEQ 0 exit /b 1
nmake
