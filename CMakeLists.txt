cmake_minimum_required(VERSION 3.2)
project(CubeChip)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL3 QUIET SDL3-shared QUIET)

if(NOT SDL3_FOUND)
  include(FetchContent)
  set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
  set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
  FetchContent_Declare(
      SDL
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG main
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
  )
  message(STATUS "Using SDL3 via FetchContent")
  FetchContent_MakeAvailable(SDL)
  set_property(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)
endif()

add_executable(CubeChipSDL
  src/CubeChip.cpp
  src/HostClass/BasicAudioSpec.cpp
  src/HostClass/HomeDirManager.cpp
  src/HostClass/HostFunctions.cpp
  src/HostClass/BasicVideoSpec.cpp
  src/GuestClass/GuestFunctions.cpp
  src/GuestClass/GameFileChecker.cpp
  src/GuestClass/HexInput.cpp
  src/GuestClass/InstructionSets/_LegacySC.cpp
  src/GuestClass/InstructionSets/_Gigachip.cpp
  src/GuestClass/InstructionSets/_Classic8.cpp
  src/GuestClass/InstructionSets/_ModernXO.cpp
  src/GuestClass/InstructionSets/_Megachip.cpp
  src/GuestClass/EmuCores/CHIP8_MODERN.cpp
  src/GuestClass/EmuCores/EmuCores.cpp
  src/GuestClass/Init.cpp
  src/Assistants/BasicLogger.cpp
  src/Assistants/BasicInput.cpp
  src/Assistants/FrameLimiter.cpp
  src/Assistants/BasicHome.cpp
  src/Assistants/SHA1.cpp
)
target_include_directories(CubeChipSDL PRIVATE src)
target_link_libraries(CubeChipSDL PRIVATE SDL3)

if(MSVC)
  # MSVC compiler options.
  target_compile_options(CubeChipSDL PRIVATE /W4 /WX /std:c++latest)
else()
  # Everything else.
  target_compile_options(CubeChipSDL PRIVATE -Wall -Wextra --std=c++23)
endif()
