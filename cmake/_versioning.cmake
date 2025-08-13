
include("${CMAKE_CURRENT_LIST_DIR}/__gitversion.cmake")

# ==================================================================================== #

math(EXPR VERSION_MAJOR_I "${VERSION_MAJOR}")
math(EXPR VERSION_MINOR_I "${VERSION_MINOR}")
math(EXPR VERSION_PATCH_I "${VERSION_PATCH}")
math(EXPR VERSION_TWEAK_I "${VERSION_TWEAK}")

# ==================================================================================== #

# integrals
add_definitions(-DPROJECT_VERSION_MAJOR=${VERSION_MAJOR_I})
add_definitions(-DPROJECT_VERSION_MINOR=${VERSION_MINOR_I})
add_definitions(-DPROJECT_VERSION_PATCH=${VERSION_PATCH_I})
add_definitions(-DPROJECT_VERSION_TWEAK=${VERSION_TWEAK_I})

# strings (zero-padded)
add_definitions(-DPROJECT_VERSION_MAJOR_S=\"${VERSION_MAJOR}\")
add_definitions(-DPROJECT_VERSION_MINOR_S=\"${VERSION_MINOR}\")
add_definitions(-DPROJECT_VERSION_PATCH_S=\"${VERSION_PATCH}\")
add_definitions(-DPROJECT_VERSION_TWEAK_S=\"${VERSION_TWEAK}\")

add_definitions(-DPROJECT_VERSION_GHASH=\"${VERSION_GHASH}\")

# full version string
add_definitions(-DPROJECT_VERSION_WITH_DATE=\"${VERSION_FULL_DATE}\")
add_definitions(-DPROJECT_VERSION_WITH_HASH=\"${VERSION_FULL_HASH}\")
