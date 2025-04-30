cmake_minimum_required(VERSION 3.10)
project(CubeChip)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_subdirectory(src/Libraries)

add_executable(${PROJECT_NAME}
  src/Assistants/BasicLogger.cpp
  src/Assistants/HomeDirManager.cpp
  src/Assistants/BasicVideoSpec.cpp
  src/Assistants/BasicAudioSpec.cpp
  src/Assistants/DefaultConfig.cpp
  src/Assistants/FrameLimiter.cpp
  src/Assistants/AttachConsole.cpp
  src/Assistants/PathGetters.cpp
  src/Assistants/ShutdownSignal.cpp
  src/Assistants/ThreadAffinity.cpp
  src/Assistants/Well512.cpp
  src/Assistants/SHA1.cpp

  src/Systems/EmuInterface.cpp
  src/Systems/CoreRegistry.cpp

  src/Systems/BYTEPUSHER/BytePusher_CoreInterface.cpp
  src/Systems/BYTEPUSHER/Cores/BYTEPUSHER_STANDARD.cpp

  src/Systems/GAMEBOY/GameBoy_CoreInterface.cpp
  src/Systems/GAMEBOY/Cores/GAMEBOY_CLASSIC.cpp

  src/Systems/CHIP8/Chip8_CoreInterface.cpp
  src/Systems/CHIP8/Cores/CHIP8_MODERN.cpp
  src/Systems/CHIP8/Cores/SCHIP_MODERN.cpp
  src/Systems/CHIP8/Cores/SCHIP_LEGACY.cpp
  src/Systems/CHIP8/Cores/XOCHIP.cpp
  src/Systems/CHIP8/Cores/MEGACHIP.cpp
  src/Systems/CHIP8/Cores/CHIP8X.cpp

  src/EmuHost.cpp
  src/CubeChip.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3-shared
    imgui
    atomic_queue
)

if(WIN32)
    if (CMAKE_SYSTEM_VERSION VERSION_LESS "10.0.22000.0")
        message(WARNING "Windows SDK version is lower than 22000. Some features may be disabled.")
        add_definitions(-DOLD_WINDOWS_SDK)
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE "Dwmapi.lib")
else()
    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
endif()

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /std:c++latest /utf-8)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra --std=c++23)
endif()
