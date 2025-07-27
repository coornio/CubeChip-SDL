cmake_minimum_required(VERSION 3.24)
project(CubeChip)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==================================================================================== #

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++20" SUPPORT_CPP20) # minimum required

if(NOT SUPPORT_CPP20)
	message(FATAL_ERROR "Compiler must support at least C++20. Aborting.")	
endif()

# ==================================================================================== #

set(SDL3_REQUIRED_VERSION 3.2.18) # SDL3 version required
find_package(SDL3 ${SDL3_REQUIRED_VERSION} QUIET) # check if present already

if(NOT SDL3_FOUND)
	include(FetchContent)
	message(STATUS "SDL3 ${SDL3_REQUIRED_VERSION} not found, fetching...")

	set(SDL_SHARED TRUE CACHE BOOL "Build SDL shared library" FORCE)

	FetchContent_Declare(
		SDL3
		GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
		GIT_TAG release-${SDL3_REQUIRED_VERSION}
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)

	FetchContent_GetProperties(SDL3)
	if(SDL3_SOURCE_DIR)
		set_property(DIRECTORY "${SDL3_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)
	endif()
	FetchContent_MakeAvailable(SDL3)
endif()

# ==================================================================================== #

set(MAIN_SOURCES
	src/FrontendHost.cpp
	src/CubeChip.cpp
)

set(SYSTEM_SOURCES
	src/Systems/SystemInterface.cpp
	src/Systems/CoreRegistry.cpp
)

set(ASSISTANTS_SOURCES
	src/Assistants/BasicLogger.cpp
	src/Assistants/BasicVideoSpec.cpp
	src/Assistants/HomeDirManager.cpp
	src/Assistants/GlobalAudioBase.cpp
	src/Assistants/AudioDevice.cpp
	src/Assistants/AudioFilters.cpp
	src/Assistants/DefaultConfig.cpp
	src/Assistants/FrameLimiter.cpp
	src/Assistants/AttachConsole.cpp
	src/Assistants/PathGetters.cpp
	src/Assistants/ThreadAffinity.cpp
	src/Assistants/LifetimeWrapperSDL.cpp
	src/Assistants/FrontendInterface.cpp
	src/Assistants/Well512.cpp
	src/Assistants/BasicInput.cpp
	src/Assistants/Millis.cpp
	src/Assistants/SHA1.cpp
)

set(SYSTEM_CHIP8_SOURCES
	src/Systems/CHIP8/Chip8_CoreInterface.cpp
	src/Systems/CHIP8/Cores/CHIP8_MODERN.cpp
	src/Systems/CHIP8/Cores/SCHIP_MODERN.cpp
	src/Systems/CHIP8/Cores/SCHIP_LEGACY.cpp
	src/Systems/CHIP8/Cores/XOCHIP.cpp
	src/Systems/CHIP8/Cores/MEGACHIP.cpp
	src/Systems/CHIP8/Cores/CHIP8X.cpp
)

set(SYSTEM_BYTEPUSHER_SOURCES
	src/Systems/BYTEPUSHER/BytePusher_CoreInterface.cpp
	src/Systems/BYTEPUSHER/Cores/BYTEPUSHER_STANDARD.cpp
)

set(SYSTEM_GAMEBOY_SOURCES
	src/Systems/GAMEBOY/GameBoy_CoreInterface.cpp
	src/Systems/GAMEBOY/Cores/GAMEBOY_CLASSIC.cpp
)

add_subdirectory(src/Libraries) # set up extern lib cmakelists
add_executable(${PROJECT_NAME}
    ${MAIN_SOURCES}
    ${ASSISTANTS_SOURCES}
    ${SYSTEM_SOURCES}
    ${SYSTEM_CHIP8_SOURCES}
    ${SYSTEM_BYTEPUSHER_SOURCES}
    ${SYSTEM_GAMEBOY_SOURCES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_latest) # always attempt the latest
target_include_directories(${PROJECT_NAME} PRIVATE src) # specify where all the headers are

target_link_libraries(${PROJECT_NAME} PRIVATE
	SDL3::SDL3-shared
	imgui
	atomic_queue
) # link the extra libs to the project

# ==================================================================================== #

if(WIN32)
	if (CMAKE_SYSTEM_VERSION VERSION_LESS "10.0.22000.0")
		message(WARNING "Windows SDK version is lower than 22000. Some features may be disabled.")
		add_definitions(-DOLD_WINDOWS_SDK)
	endif()
	target_link_libraries(${PROJECT_NAME} PRIVATE "Dwmapi.lib")
endif()

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
