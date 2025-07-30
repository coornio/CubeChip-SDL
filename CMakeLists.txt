cmake_minimum_required(VERSION 3.25)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(CubeChip)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src")
set(PROJECT_LIBRARY_DIR "${PROJECT_INCLUDE_DIR}/Libraries")

# ==================================================================================== #

include(FetchContent)

set(SDL3_REQUIRED_VERSION "3.2.18")

set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL3_test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build the test directory" FORCE)
set(SDL_INSTALL_TESTS OFF CACHE BOOL "Install test-cases" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Build the examples directory" FORCE)

set(SDL_SHARED OFF CACHE BOOL "Build a shared version of the library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build a static version of the library" FORCE)

FetchContent_Declare(
	SDL3
	GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
	GIT_TAG "release-${SDL3_REQUIRED_VERSION}"
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)

FetchContent_GetProperties(SDL3)

if(NOT SDL3_POPULATED)
	message(STATUS "Fetching SDL3 v${SDL3_REQUIRED_VERSION}...")
	FetchContent_MakeAvailable(SDL3)
endif()

# ==================================================================================== #

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)

message(STATUS "Fetching ImGUI: [docking] branch...")
FetchContent_MakeAvailable(imgui)

add_library(
	imgui STATIC
	# Core ImGui source files
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"

    # Optional std::string support
    "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"

    # Backend implementation files (SDL3 + SDLRenderer3)
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
)

target_include_directories(
	imgui PUBLIC
	"${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${imgui_SOURCE_DIR}/misc/cpp"
	"${SDL3_INCLUDE_DIRS}"
)

target_link_libraries(
	imgui PRIVATE
	SDL3::SDL3
)

# ==================================================================================== #

set(MAIN_SOURCES
	"${PROJECT_INCLUDE_DIR}/FrontendHost.cpp"
	"${PROJECT_INCLUDE_DIR}/CubeChip.cpp"
)

set(SYSTEM_SOURCES
	"${PROJECT_INCLUDE_DIR}/Systems/SystemInterface.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CoreRegistry.cpp"
)

set(ASSISTANTS_SOURCES
	"${PROJECT_INCLUDE_DIR}/Assistants/BasicLogger.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/BasicVideoSpec.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/HomeDirManager.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/GlobalAudioBase.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/AudioDevice.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/AudioFilters.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/DefaultConfig.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/FrameLimiter.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/AttachConsole.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/PathGetters.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/ThreadAffinity.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/LifetimeWrapperSDL.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/FrontendInterface.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/Well512.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/BasicInput.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/Millis.cpp"
	"${PROJECT_INCLUDE_DIR}/Assistants/SHA1.cpp"
)

set(SYSTEM_CHIP8_SOURCES
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Chip8_CoreInterface.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Cores/CHIP8_MODERN.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Cores/SCHIP_MODERN.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Cores/SCHIP_LEGACY.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Cores/XOCHIP.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Cores/MEGACHIP.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/CHIP8/Cores/CHIP8X.cpp"
)

set(SYSTEM_BYTEPUSHER_SOURCES
	"${PROJECT_INCLUDE_DIR}/Systems/BYTEPUSHER/BytePusher_CoreInterface.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/BYTEPUSHER/Cores/BYTEPUSHER_STANDARD.cpp"
)

set(SYSTEM_GAMEBOY_SOURCES
	"${PROJECT_INCLUDE_DIR}/Systems/GAMEBOY/GameBoy_CoreInterface.cpp"
	"${PROJECT_INCLUDE_DIR}/Systems/GAMEBOY/Cores/GAMEBOY_CLASSIC.cpp"
)

add_subdirectory(
	"${PROJECT_LIBRARY_DIR}/atomic_queue"
	EXCLUDE_FROM_ALL
)

add_executable("${PROJECT_NAME}"
    ${MAIN_SOURCES}
    ${ASSISTANTS_SOURCES}
    ${SYSTEM_SOURCES}
    ${SYSTEM_CHIP8_SOURCES}
    ${SYSTEM_BYTEPUSHER_SOURCES}
    ${SYSTEM_GAMEBOY_SOURCES}
)

target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_20)
target_include_directories("${PROJECT_NAME}" PRIVATE "${PROJECT_INCLUDE_DIR}")

target_link_libraries(
	"${PROJECT_NAME}" PRIVATE
	SDL3::SDL3
	imgui
	atomic_queue
) # link the extra libs to the project

# ==================================================================================== #

if(WIN32)
	if (CMAKE_SYSTEM_VERSION VERSION_LESS "10.0.22000.0")
		message(WARNING "Windows SDK version is lower than 22000. Some features may be disabled.")
		add_definitions(-DOLD_WINDOWS_SDK)
	endif()
	target_link_libraries("${PROJECT_NAME}" PRIVATE "Dwmapi.lib")
endif()

# ==================================================================================== #

if(MSVC)
	set_property(DIRECTORY
		"${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY 
		VS_STARTUP_PROJECT "${PROJECT_NAME}"
	)

	target_compile_options(
		"${PROJECT_NAME}" PRIVATE
		$<$<CONFIG:Release>: /W4 /MP /utf-8 /O2 /Ob2 /Oi /Ot /GT /GL>
		$<$<CONFIG:Debug>: /W4 /MP /utf-8 /Od /Zi /RTC1>
	)

	target_link_options(
		"${PROJECT_NAME}" PRIVATE
		$<$<CONFIG:Release>: /LTCG /SUBSYSTEM:WINDOWS /INCREMENTAL:NO>
		$<$<CONFIG:Debug>: /SUBSYSTEM:CONSOLE /INCREMENTAL>
	)
else()
	target_compile_options(
		"${PROJECT_NAME}" PRIVATE
		$<$<CONFIG:Release>: -O3 -march=native -flto>
		$<$<CONFIG:Debug>: -Og -g>
	)

	target_link_options(
		"${PROJECT_NAME}" PRIVATE
		$<$<CONFIG:Release>: -flto -s>
		$<$<CONFIG:Debug>: >
	)
endif()
