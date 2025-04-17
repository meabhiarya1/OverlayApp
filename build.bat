@echo off
setlocal

:: Set compiler
set COMPILER=cl

:: Set source files
set SOURCES=main.cpp imgui\*.cpp imgui\backends\imgui_impl_win32.cpp imgui\backends\imgui_impl_dx11.cpp

:: Set include paths
set INCLUDES=/Iimgui /Iimgui\backends

:: Set compiler flags
set CXXFLAGS=/TP /std:c++17

:: Set libraries
set LIBS=dwmapi.lib d3d11.lib

:: Compile
%COMPILER% %CXXFLAGS% %SOURCES% %INCLUDES% /link %LIBS%

pause
