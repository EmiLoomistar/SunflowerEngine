@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
pushd ..\
cl.exe -O2 ".\Sunflower Engine\*.c" /arch:AVX /Fe:sunflower.exe user32.lib gdi32.lib Winmm.lib
del *.obj
if %errorlevel%==0 (
sunflower.exe
)
popd

