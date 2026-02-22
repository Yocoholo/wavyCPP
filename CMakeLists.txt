cmake_minimum_required(VERSION 3.16)

project(wavy VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Options ──────────────────────────────────────────────────────────────────
option(GIT_SUBMODULE "Update submodules during configure" ON)

# ── CPU count for parallel builds ────────────────────────────────────────────
include(ProcessorCount)
ProcessorCount(NPROC)
if(NOT NPROC OR NPROC EQUAL 0)
  set(NPROC 4)
endif()

# ── Submodules ───────────────────────────────────────────────────────────────
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git" AND GIT_SUBMODULE)
  message(STATUS "Updating submodules...")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE GIT_SUBMOD_RESULT)
  if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    message(FATAL_ERROR "git submodule update failed (${GIT_SUBMOD_RESULT})")
  endif()
endif()

foreach(_sub bx bimg bgfx bnet)
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/${_sub}/makefile")
    message(FATAL_ERROR
      "Submodule '${_sub}' not found. Run:\n"
      "  git submodule update --init --recursive")
  endif()
endforeach()

# ═════════════════════════════════════════════════════════════════════════════
# Platform & build-type detection
# ═════════════════════════════════════════════════════════════════════════════
if(MINGW)
  set(WAVY_PLATFORM      "windows")
  set(BGFX_BUILD_SUBDIR  "win64_mingw-gcc")
  set(BGFX_MAKE_RELEASE  "mingw-gcc-release64")
  set(BGFX_MAKE_DEBUG    "mingw-gcc-debug64")
  set(BGFX_MAKE_ARGS     MINGW=/usr)
elseif(UNIX AND NOT APPLE)
  set(WAVY_PLATFORM      "linux")
  set(BGFX_BUILD_SUBDIR  "linux64_gcc")
  set(BGFX_MAKE_RELEASE  "linux-release64")
  set(BGFX_MAKE_DEBUG    "linux-debug64")
  set(BGFX_MAKE_ARGS     "")
else()
  message(FATAL_ERROR
    "Unsupported platform.\n"
    "Supported: Linux native, Windows (MinGW cross-compile from Linux)")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(BGFX_SUFFIX      "Debug")
  set(BGFX_MAKE_TARGET "${BGFX_MAKE_DEBUG}")
else()
  set(BGFX_SUFFIX      "Release")
  set(BGFX_MAKE_TARGET "${BGFX_MAKE_RELEASE}")
endif()

set(BGFX_DIR     "${PROJECT_SOURCE_DIR}/submodules/bgfx")
set(BGFX_BIN_DIR "${BGFX_DIR}/.build/${BGFX_BUILD_SUBDIR}/bin")

message(STATUS "─── wavy ───────────────────────────────────")
message(STATUS "  Platform:   ${WAVY_PLATFORM}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  bgfx libs:  ${BGFX_BIN_DIR}")
message(STATUS "  Jobs:       ${NPROC}")
message(STATUS "─────────────────────────────────────────────")

# ═════════════════════════════════════════════════════════════════════════════
# bgfx build  (automatic — builds once, then skips via stamp file)
#
#   Force rebuild:  cmake --build <dir> --target build_bgfx
# ═════════════════════════════════════════════════════════════════════════════
set(BGFX_STAMP "${CMAKE_BINARY_DIR}/.bgfx_${BGFX_SUFFIX}.stamp")

add_custom_command(
  OUTPUT  ${BGFX_STAMP}
  COMMAND make -C "${BGFX_DIR}" ${BGFX_MAKE_TARGET} ${BGFX_MAKE_ARGS} -j${NPROC}
  COMMAND ${CMAKE_COMMAND} -E touch ${BGFX_STAMP}
  COMMENT "Building bgfx (${BGFX_MAKE_TARGET})..."
  USES_TERMINAL
)
add_custom_target(build_bgfx DEPENDS ${BGFX_STAMP})

# When cross-compiling for Windows we also need a native Linux shaderc
# for SPIR-V / GLSL shader compilation.  Serialised after the MinGW build
# to prevent concurrent make runs in the same source tree.
if(MINGW)
  set(SHADERC_NATIVE_STAMP "${CMAKE_BINARY_DIR}/.shaderc_native.stamp")

  add_custom_command(
    OUTPUT  ${SHADERC_NATIVE_STAMP}
    COMMAND make -C "${BGFX_DIR}" linux-release64 -j${NPROC}
    COMMAND ${CMAKE_COMMAND} -E touch ${SHADERC_NATIVE_STAMP}
    DEPENDS ${BGFX_STAMP}
    COMMENT "Building native Linux shaderc for shader compilation..."
    USES_TERMINAL
  )
  add_custom_target(build_shaderc DEPENDS ${SHADERC_NATIVE_STAMP})
endif()

# ═════════════════════════════════════════════════════════════════════════════
# Wavy executable
# ═════════════════════════════════════════════════════════════════════════════
add_executable(wavy
  src/main.cpp
  src/grid.cpp
  src/renderer.cpp
  src/marching_squares.cpp
  src/helpers.cpp
  src/OpenSimplex.cpp
)

target_include_directories(wavy PRIVATE
  src
  ./include
  ${BGFX_DIR}/include
  ${PROJECT_SOURCE_DIR}/submodules/bx/include
  ${PROJECT_SOURCE_DIR}/submodules/bimg/include
  ${BGFX_DIR}/3rdparty
  ${BGFX_DIR}/examples/common
)

# Platform compat headers
if(MINGW)
  target_include_directories(wavy PRIVATE
    ${PROJECT_SOURCE_DIR}/submodules/bx/include/compat/mingw)
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  target_compile_definitions(wavy PRIVATE BX_CONFIG_DEBUG=1)
else()
  target_compile_definitions(wavy PRIVATE BX_CONFIG_DEBUG=0)
endif()

# ── Linking ──────────────────────────────────────────────────────────────────
target_link_directories(wavy PRIVATE ${BGFX_BIN_DIR})

# bgfx static libs — link order matters (most-dependent first)
target_link_libraries(wavy PRIVATE
  example-glue${BGFX_SUFFIX}
  example-common${BGFX_SUFFIX}
  bimg_decode${BGFX_SUFFIX}
  bgfx${BGFX_SUFFIX}
  bimg${BGFX_SUFFIX}
  bx${BGFX_SUFFIX}
)

# System libraries (platform-specific)
if(MINGW)
  target_link_libraries(wavy PRIVATE d3d11 dxgi d3dcompiler dxguid psapi)
elseif(UNIX)
  target_link_libraries(wavy PRIVATE X11 GL pthread dl rt)
endif()

# Ensure bgfx is built before linking
add_dependencies(wavy build_bgfx)

# ── Copy bgfx runtime assets into build dir ──────────────────────────────────
file(COPY ${BGFX_DIR}/examples/runtime/ DESTINATION ${CMAKE_BINARY_DIR})

# ═════════════════════════════════════════════════════════════════════════════
# Shader compilation  (automatic build dependency)
# ═════════════════════════════════════════════════════════════════════════════
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/src/shaders")
set(SHADER_INCLUDES
  -i "${BGFX_DIR}/src"
  -i "${BGFX_DIR}/examples/common"
  --varyingdef "${SHADER_SRC_DIR}/varying.def.sc"
)
file(GLOB SHADER_SOURCES "${SHADER_SRC_DIR}/vs_*.sc" "${SHADER_SRC_DIR}/fs_*.sc")

# ── Helper function: compile all shaders for one backend ─────────────────────
# Usage: wavy_add_shader_backend(<NAME> <SHADERC> <PLATFORM> <PROFILE> <OUTDIR>
#                                 [extra DEPENDS ...])
function(wavy_add_shader_backend BACKEND SHADERC PLATFORM PROFILE OUT_DIR)
  set(_outputs "")
  file(MAKE_DIRECTORY ${OUT_DIR})

  foreach(_src ${SHADER_SOURCES})
    get_filename_component(_name ${_src} NAME_WE)
    if(_name MATCHES "^vs_")
      set(_type vertex)
    else()
      set(_type fragment)
    endif()

    set(_out "${OUT_DIR}/${_name}.bin")
    add_custom_command(
      OUTPUT  ${_out}
      COMMAND ${SHADERC}
              -f ${_src} -o ${_out}
              --platform ${PLATFORM} --type ${_type}
              ${SHADER_INCLUDES} -p ${PROFILE} -O 3
      DEPENDS ${_src} "${SHADER_SRC_DIR}/varying.def.sc" ${ARGN}
      COMMENT "[${BACKEND}] ${_name}"
    )
    list(APPEND _outputs ${_out})
  endforeach()

  set(${BACKEND}_OUTPUTS ${_outputs} PARENT_SCOPE)
endfunction()

# ── Set up shader backends per platform ──────────────────────────────────────
set(ALL_SHADER_OUTPUTS "")

if(MINGW)
  # DX11 shaders — Windows shaderc.exe produced by the MinGW bgfx build
  set(SHADERC_WIN "${BGFX_BIN_DIR}/shaderc${BGFX_SUFFIX}.exe")
  wavy_add_shader_backend(DX11 "${SHADERC_WIN}" windows s_5_0
    "${CMAKE_BINARY_DIR}/shaders/dx11" ${BGFX_STAMP})
  list(APPEND ALL_SHADER_OUTPUTS ${DX11_OUTPUTS})

  # SPIR-V & GLSL shaders — native Linux shaderc (built separately)
  set(SHADERC_LIN
    "${PROJECT_SOURCE_DIR}/submodules/bgfx/.build/linux64_gcc/bin/shadercRelease")

  wavy_add_shader_backend(SPIRV "${SHADERC_LIN}" linux spirv
    "${CMAKE_BINARY_DIR}/shaders/spirv" ${SHADERC_NATIVE_STAMP})
  list(APPEND ALL_SHADER_OUTPUTS ${SPIRV_OUTPUTS})

  wavy_add_shader_backend(GLSL "${SHADERC_LIN}" linux 120
    "${CMAKE_BINARY_DIR}/shaders/glsl" ${SHADERC_NATIVE_STAMP})
  list(APPEND ALL_SHADER_OUTPUTS ${GLSL_OUTPUTS})

else()
  # Linux native — shaderc comes from the same bgfx build
  set(SHADERC_LIN "${BGFX_BIN_DIR}/shaderc${BGFX_SUFFIX}")

  wavy_add_shader_backend(SPIRV "${SHADERC_LIN}" linux spirv
    "${CMAKE_BINARY_DIR}/shaders/spirv" ${BGFX_STAMP})
  list(APPEND ALL_SHADER_OUTPUTS ${SPIRV_OUTPUTS})

  wavy_add_shader_backend(GLSL "${SHADERC_LIN}" linux 120
    "${CMAKE_BINARY_DIR}/shaders/glsl" ${BGFX_STAMP})
  list(APPEND ALL_SHADER_OUTPUTS ${GLSL_OUTPUTS})
endif()

add_custom_target(shaders DEPENDS ${ALL_SHADER_OUTPUTS})
add_dependencies(wavy shaders)

