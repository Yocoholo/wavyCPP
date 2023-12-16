cd submodules\bgfx
call make mingw-gcc-release64
cd ..\..

call compile_shaders.bat
echo Press enter to continue...
pause
