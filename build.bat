@echo off
echo Compiling OverlayApp...

:: Set up MSVC environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: Build command
cl /EHsc main.cpp user32.lib gdi32.lib

pause
