# Dépendances externes, toutes récupérées et versionnées via FetchContent.
# Règle : chaque dépendance est épinglée sur un tag précis (voir docs/adr/0001-stack-technique.md).
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

if(ODYSSEE_BUILD_CLIENT)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(raylib)
endif()

if(ODYSSEE_BUILD_TESTS)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(doctest)
endif()
