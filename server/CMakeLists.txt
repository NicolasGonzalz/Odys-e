# Cœur du serveur, en bibliothèque pour pouvoir le tester.
add_library(odyssee_server_core STATIC
    src/Server.cpp
    src/ClientRegistry.cpp
    src/CommandLine.cpp
    src/ConnectionService.cpp
    src/GameStep.cpp
    src/GameWorld.cpp
    src/InputQueue.cpp
    src/ShutdownSignals.cpp
)
target_include_directories(odyssee_server_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(odyssee_server_core PUBLIC odyssee::shared)
odyssee_set_options(odyssee_server_core)

# L'exécutable ne fait qu'assembler.
add_executable(odyssee_server src/main.cpp)
target_link_libraries(odyssee_server PRIVATE odyssee_server_core)
odyssee_set_options(odyssee_server)
