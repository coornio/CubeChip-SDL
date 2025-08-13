
set(VERSION_MAJOR "0" CACHE STRING "")
set(VERSION_MINOR "0" CACHE STRING "")
set(VERSION_PATCH "0" CACHE STRING "")
set(VERSION_TWEAK "0" CACHE STRING "")
set(VERSION_GHASH "0" CACHE STRING "")

if(NOT DEFINED FROM_WORKFLOW)
	include("${CMAKE_CURRENT_LIST_DIR}/__gitversion.cmake")
endif()

# ==================================================================================== #

math(EXPR VERSION_MAJOR_I "${VERSION_MAJOR}")
math(EXPR VERSION_MINOR_I "${VERSION_MINOR}")
math(EXPR VERSION_PATCH_I "${VERSION_PATCH}")
math(EXPR VERSION_TWEAK_I "${VERSION_TWEAK}")

# ==================================================================================== #

message(STATUS "Version Information:")

message(STATUS "| MAJOR: '${VERSION_MAJOR}'")
message(STATUS "| MINOR: '${VERSION_MINOR}'")
message(STATUS "| PATCH: '${VERSION_PATCH}'")
message(STATUS "| TWEAK: '${VERSION_TWEAK}'")
message(STATUS "| GHASH: '${VERSION_GHASH}'")

set(VERSION_FULL_DATE "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}" CACHE INTERNAL "")
set(VERSION_FULL_HASH "${VERSION_FULL_DATE}.${VERSION_GHASH}" CACHE INTERNAL "")

message(STATUS "| FULL_DATE: '${VERSION_FULL_DATE}'")
message(STATUS "| WITH_HASH: '${VERSION_FULL_HASH}'")

# integrals
add_definitions(-DPROJECT_VERSION_MAJOR_I=${VERSION_MAJOR_I})
add_definitions(-DPROJECT_VERSION_MINOR_I=${VERSION_MINOR_I})
add_definitions(-DPROJECT_VERSION_PATCH_I=${VERSION_PATCH_I})
add_definitions(-DPROJECT_VERSION_TWEAK_I=${VERSION_TWEAK_I})

# strings (zero-padded)
add_definitions(-DPROJECT_VERSION_MAJOR=\"${VERSION_MAJOR}\")
add_definitions(-DPROJECT_VERSION_MINOR=\"${VERSION_MINOR}\")
add_definitions(-DPROJECT_VERSION_PATCH=\"${VERSION_PATCH}\")
add_definitions(-DPROJECT_VERSION_TWEAK=\"${VERSION_TWEAK}\")
add_definitions(-DPROJECT_VERSION_GHASH=\"${VERSION_GHASH}\")

# full version string
add_definitions(-DPROJECT_VERSION_WITH_DATE=\"${VERSION_FULL_DATE}\")
add_definitions(-DPROJECT_VERSION_WITH_HASH=\"${VERSION_FULL_HASH}\")
