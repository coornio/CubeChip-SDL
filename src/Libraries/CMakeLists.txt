CMAKE_MINIMUM_REQUIRED( VERSION 3.25 )

add_library(
    atomic_queue
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/atomic_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/atomic_queue_mutex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/barrier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/defs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/atomic_queue/spinlock.h
)

target_include_directories(
    atomic_queue
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(
    imgui
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_stdlib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(
    imgui
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)