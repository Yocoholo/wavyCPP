cmake_minimum_required(VERSION 3.0.0)
project(wavy VERSION 0.1.0)
add_executable(wavy wavy.cpp)
# add_executable(wavy cubes.cpp)

#           DOWNLOAD ALL THE SUBMODULES
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()


# make sure they exist
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/bx/makefile")
	message(FATAL_ERROR "The bx submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/bimg/makefile")
	message(FATAL_ERROR "The bimg submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/bgfx/makefile")
	message(FATAL_ERROR "The bgfx submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/bnet/makefile")
	message(FATAL_ERROR "The bnet submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(MINGW)
  option(BX_CONFIG_DEBUG "Log debug messages (default: on in debug)" OFF)
  target_include_directories(wavy PRIVATE src)

  #include dirs
  target_include_directories(wavy 
	PRIVATE
		submodules/bgfx/include
		submodules/bx/include
		submodules/bx/include/compat/mingw
		submodules/bimg/include
    submodules/bgfx/3rdparty/glslang/glslang/MachineIndependent/preprocessor
  )

  target_include_directories(wavy 
	PRIVATE 
		submodules/bgfx/3rdparty/
		submodules/bgfx/examples/common
	)

  #LINK LIBS
  target_link_libraries(wavy d3d11.lib dxgi.lib d3dcompiler.lib dxguid.lib)

  #BGFX LINKS LOGIC
  target_link_directories(wavy PRIVATE ./submodules/bgfx/.build/win64_mingw-gcc/bin/)

  if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(wavy PRIVATE BX_CONFIG_DEBUG=1)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/bgfx/.build/win64_mingw-gcc/bin/libbgfxDebug.a")
      message(FATAL_ERROR "The bgfx submodule was not built as debug! Go to bgfx folder and run: make mingw-gcc-debug64")
    endif()
	target_link_libraries(wavy 
		bgfxDebug
		bimgDebug
		bxDebug
		example-glueDebug
		example-commonDebug
		bimg_decodeDebug
	)
  elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    target_compile_definitions(wavy PRIVATE BX_CONFIG_DEBUG=0)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/submodules/bgfx/.build/win64_mingw-gcc/bin/libbgfxRelease.a")
      message(FATAL_ERROR "The bgfx submodule was not built as release! Go to bgfx folder and run: make mingw-gcc-release64")
    endif()
	target_link_libraries(wavy
		example-glueRelease
		example-commonRelease
		bimg_decodeRelease
		bgfxRelease
		bimgRelease 
		bxRelease
	)
  endif()
else()
	message(FATAL_ERROR "This is for win64_mingw-gcc only right now, what's on your Hard Drive?")
endif()

file (COPY ${CMAKE_SOURCE_DIR}/submodules/bgfx/examples/runtime/ DESTINATION ${CMAKE_BINARY_DIR})

target_include_directories(wavy PRIVATE ${CMAKE_BINARY_DIR}/shaders)

