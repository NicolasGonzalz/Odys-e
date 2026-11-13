# Cœur du serveur, en bibliothèque pour pouvoir le tester.
add_library(odyssee_server_core STATIC
    src/Server.cpp
    src/CatalogLoading.cpp
    src/ChallengeTokens.cpp
    src/ClientRegistry.cpp
    src/CommandLine.cpp
    src/ecs/EntityPool.cpp
    src/ConnectionService.cpp
    src/GameStep.cpp
    src/GameWorld.cpp
    src/InputQueue.cpp
    src/Inventory.cpp
    src/IpRateLimiter.cpp
    src/ShutdownSignals.cpp
    src/SipHash.cpp
    src/TokenBucket.cpp
    src/Vitals.cpp
    src/ResourceLoading.cpp
    src/WorldLoading.cpp
    src/resources/HarvestReach.cpp
    src/resources/Harvesting.cpp
    src/resources/ResourceField.cpp
    src/resources/ResourceScatter.cpp
)
target_include_directories(odyssee_server_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(odyssee_server_core PUBLIC odyssee::shared)
odyssee_set_options(odyssee_server_core)

# L'exécutable ne fait qu'assembler.
add_executable(odyssee_server src/main.cpp)
target_link_libraries(odyssee_server PRIVATE odyssee_server_core)
odyssee_set_options(odyssee_server)

# Binaire de déploiement (homelab) : lié en statique, il ne dépend pas de la version de la glibc
# du conteneur. Possible car le serveur n'utilise ni DNS ni comptes système (NSS).
if(ODYSSEE_STATIC_SERVER AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(odyssee_server PRIVATE -static)
endif()
