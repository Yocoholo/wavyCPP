<p align="center">
<img src="Screenshot.png">
</p>

[Wavy](https://github.com/Yocoholo/wavyCPP) - A BGFX marching squares implementation
============================

<p align="center">
    <a href="#what-is-it">What is it?</a> -
    <a href="#building">Building</a> -
    <a href="#references">References</a>
</p>

What is it?
============================
Wavy is a C++ program that implements a marching squares algorithm for generating waves and makes use of the the [BGFX](https://github.com/bkaradzic/bgfx) graphics library.

This was a project to learn how graphics are made. To gain an appreciation for rendering, graphics libraries, and engines.

Building
============================

### Prerequisites

- **CMake** ≥ 3.16
- **Make** and a **C++ compiler** (GCC)
- **Git** (for submodule checkout)
- **Linux build deps:** `libx11-dev`, `libgl-dev` (for native Linux builds)
- **MinGW-w64** with the **posix** threading model (for Windows cross-compile only):
  ```bash
  sudo apt install mingw-w64 g++-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix
  sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
  sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
  ```

### Clone

```bash
git clone --recursive https://github.com/Yocoholo/wavyCPP.git
cd wavyCPP
```

---

### Linux (native)

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Run:
```bash
./wavy --type opengl    # or --type vulkan
```

### Windows (cross-compile from Linux)

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Run (on Windows or WSL):
```bash
./wavy.exe              # DX11 by default, or --type vulkan / --type opengl
```

---

### How it works

The first `cmake --build .` will automatically:
1. **Build bgfx** static libraries for your platform (stamp-file cached — only runs once)
2. **Build a native Linux shaderc** if cross-compiling (for SPIR-V / GLSL shaders)
3. **Compile shaders** for all available backends (DX11, SPIR-V, GLSL)
4. **Build wavy**

Subsequent builds only recompile what changed. Editing `.sc` shader files triggers automatic shader recompilation.

> **Note:** The posix threading model is required for MinGW — the win32 model lacks `std::mutex` support needed by bimg's ASTC encoder.

### Debug build

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug          # add toolchain file for Windows target
cmake --build .
```

### Force-rebuild bgfx

```bash
cmake --build build --target build_bgfx
```

References
============================
[BGFX](https://github.com/bkaradzic/bgfx) - The graphics library used in this project.

[BGFX starter template](https://github.com/codetechandtutorials/bgfx_starter_template) - While not entirely used, was useful when starting this project.
