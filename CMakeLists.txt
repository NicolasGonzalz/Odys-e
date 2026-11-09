cmake_minimum_required(VERSION 3.24)

project(Odyssee
    VERSION 0.1.0
    DESCRIPTION "Jeu multijoueur d'aventure et de survie en monde persistant"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Type de build" FORCE)
endif()

option(ODYSSEE_BUILD_CLIENT "Compiler le client graphique (raylib)" ON)
option(ODYSSEE_BUILD_SERVER "Compiler le serveur dédié" ON)
option(ODYSSEE_BUILD_TESTS  "Compiler les tests unitaires" ON)
option(ODYSSEE_SANITIZERS   "Activer ASan + UBSan (Debug)" OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(CompilerWarnings)
include(Dependencies)

add_subdirectory(shared)

if(ODYSSEE_BUILD_SERVER)
    add_subdirectory(server)
endif()

if(ODYSSEE_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(ODYSSEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
