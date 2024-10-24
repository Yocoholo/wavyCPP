@REM  rebuild.bat rebuilds BGFX for MINGW and also recompiles shaders.

cd submodules\bgfx
call make mingw-gcc-release64
call make mingw-gcc-debug64
cd ..\..

call compile_shaders.bat
echo Press enter to continue...
pause
