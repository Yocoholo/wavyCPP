@echo off

@REM  compile_shaders.bat makes use of the shaderc executable from BGFX
@REM    and compiles the src/shaders/ into bin files for loading into the BGFX program

set PATH=.\build\shaders\dx11
set FRAGMENT_SHADER=fs_wavy.bin
set VERTEX_SHADER=vs_wavy.bin
set FRAGMENT_SHADER_SRC=.\src\shaders\fs_wavy.sc
set VERTEX_SHADER_SRC=.\src\shaders\vs_wavy.sc
set VARYING_DEF_SRC=.\src\shaders\varying.def.sc

:loop
set /p message="Enter to build | Any key to quit"

if "%message%"=="" (
  submodules\bgfx\.build\win64_mingw-gcc\bin\shadercRelease.exe ^
      -f %VERTEX_SHADER_SRC% ^
      -o %PATH%\%VERTEX_SHADER% ^
      --platform windows ^
      --type vertex ^
      --verbose ^
      -i "submodules\bgfx\src" ^
      -i "submodules\bgfx\examples\common" ^
      --varyingdef %VARYING_DEF_SRC% ^
      -p s_5_0 ^
      -O 3
  submodules\bgfx\.build\win64_mingw-gcc\bin\shadercRelease.exe ^
      -f %FRAGMENT_SHADER_SRC% ^
      -o %PATH%\%FRAGMENT_SHADER% ^
      --platform windows ^
      --type fragment ^
      --verbose ^
      -i "submodules\bgfx\src" ^
      -i "submodules\bgfx\examples\common" ^
      --varyingdef %VARYING_DEF_SRC% ^
      -p s_5_0 ^
      -O 3
  goto loop
) else (
  exit
)
