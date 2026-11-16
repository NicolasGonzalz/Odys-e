# Cœur du serveur, en bibliothèque pour pouvoir le tester.
add_library(odyssee_server_core STATIC
    src/Server.cpp
    src/CatalogLoading.cpp
    src/ChallengeTokens.cpp
    src/ClientRegistry.cpp
    src/CommandLine.cpp
    src/ecs/EntityPool.cpp
    src/ConnectionService.cpp
    src/Crafting.cpp
    src/GameStep.cpp
    src/GameWorld.cpp
    src/GameWorldSave.cpp
    src/GameWorldStory.cpp
    src/GameWorldMap.cpp
    src/StoryLoading.cpp
    src/InputQueue.cpp
    src/Inventory.cpp
    src/ItemUse.cpp
    src/IpRateLimiter.cpp
    src/ShutdownSignals.cpp
    src/SipHash.cpp
    src/TokenBucket.cpp
    src/Vitals.cpp
    src/death/Death.cpp
    src/death/DroppedBags.cpp
    src/exploration/Explorer.cpp
    src/exploration/MapTables.cpp
    src/exploration/WorldMap.cpp
    src/persistence/SaveStore.cpp
    src/persistence/Sqlite.cpp
    src/story/ClueField.cpp
    src/story/ClueReading.cpp
    src/story/Journal.cpp
    src/story/SurvivorRoster.cpp
    src/ResourceLoading.cpp
    src/WorldLoading.cpp
    src/resources/HarvestReach.cpp
    src/resources/Harvesting.cpp
    src/resources/ResourceField.cpp
    src/resources/ResourceScatter.cpp
)
target_include_directories(odyssee_server_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(odyssee_server_core PUBLIC odyssee::shared PRIVATE odyssee::sqlite)
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
