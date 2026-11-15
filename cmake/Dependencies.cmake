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

# SQLite (persistance du monde, T053) : l'amalgame officiel, compilé avec le serveur (y compris
# le binaire statique du homelab et le serveur local Windows). Pas de code d'Odyssée dedans :
# compilé sans nos avertissements stricts.
if(ODYSSEE_BUILD_SERVER)
    enable_language(C) # l'amalgame est en C ; le reste du projet est en C++
    FetchContent_Declare(sqlite
        URL      https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
        URL_HASH SHA256=77823cb110929c2bcb0f5d48e4833b5c59a8a6e40cdea3936b99e199dbbe5784
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(sqlite)
    add_library(odyssee_sqlite STATIC ${sqlite_SOURCE_DIR}/sqlite3.c)
    target_include_directories(odyssee_sqlite SYSTEM PUBLIC ${sqlite_SOURCE_DIR})
    # Un seul fil d'exécution touche la base ; pas d'extensions chargées à l'exécution.
    target_compile_definitions(odyssee_sqlite PRIVATE SQLITE_THREADSAFE=0 SQLITE_OMIT_LOAD_EXTENSION
                                                      SQLITE_DEFAULT_FOREIGN_KEYS=1)
    set_target_properties(odyssee_sqlite PROPERTIES POSITION_INDEPENDENT_CODE ON)
    add_library(odyssee::sqlite ALIAS odyssee_sqlite)
endif()
