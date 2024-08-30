cmake_minimum_required(VERSION 3.5)
project(CubeChip)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL3 QUIET)

if(NOT SDL3_FOUND)
  include(FetchContent)
  set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
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
  src/Assistants/BasicLogger.cpp
  src/Assistants/BasicInput.cpp
  src/Assistants/FrameLimiter.cpp
  src/Assistants/SHA1.cpp
  src/HostClass/HomeDirManager.cpp
  src/HostClass/BasicVideoSpec.cpp
  src/HostClass/BasicAudioSpec.cpp
  src/HostClass/EmuHost.cpp
  src/GuestClass/HexInput.cpp
  src/GuestClass/EmuCores/EmuCores.cpp
  src/GuestClass/EmuCores/CHIP8_MODERN.cpp
  src/GuestClass/GameFileChecker.cpp
  src/CubeChip.cpp
)
target_include_directories(CubeChipSDL PRIVATE src)
target_link_libraries(CubeChipSDL PRIVATE SDL3::SDL3-shared)

if(MSVC)
  # MSVC compiler options.
  target_compile_options(CubeChipSDL PRIVATE /W4 /std:c++latest)
else()
  # Everything else.
  target_compile_options(CubeChipSDL PRIVATE -Wall -Wextra --std=c++23)
endif()
