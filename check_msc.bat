@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /nologo /EHsc D:\xieyi\check_msc.cpp
D:\xieyi\check_msc.exe
del D:\xieyi\check_msc.obj D:\xieyi\check_msc.exe 2>/dev/null
