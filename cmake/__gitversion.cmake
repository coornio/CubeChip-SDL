
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    message(STATUS "Versioning path: cloned repo")
    message(STATUS "| Fetching commit hash...")
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION_GHASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "| Check if worktree/index is dirty...")
    execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_MODIFIED
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(GIT_MODIFIED)
        message(STATUS "  | Worktree has been modified, adjusting hash...")
        set(VERSION_GHASH "${VERSION_GHASH}-wip")
    endif()
    message(STATUS "| Fetch commit date...")
    execute_process(
        COMMAND git show -s --format=%ci HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "  | Commit date: '${GIT_COMMIT_DATE}'")
    message(STATUS "| Parse commit version from date...")
    string(REGEX MATCH "([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9])" _ "${GIT_COMMIT_DATE}")
    set(VERSION_MAJOR 0)
    set(VERSION_MINOR "${CMAKE_MATCH_1}")
    set(VERSION_PATCH "${CMAKE_MATCH_2}")
    set(VERSION_TWEAK "${CMAKE_MATCH_3}")
else()
    message(STATUS "Versioning path: local copy w/o git")
    set(VERSION_MAJOR 0)
    string(TIMESTAMP VERSION_MINOR "%Y")
    string(TIMESTAMP VERSION_PATCH "%m")
    string(TIMESTAMP VERSION_TWEAK "%d")
    string(TIMESTAMP VERSION_GHASH "local-%H:%M:%S")
endif()
