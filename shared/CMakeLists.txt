# Code partagé client/serveur : protocole, sérialisation, simulation déterministe.
add_library(odyssee_shared STATIC
    src/ByteBuffer.cpp
    src/Log.cpp
    src/net/Address.cpp
)
add_library(odyssee::shared ALIAS odyssee_shared)

# Socket UDP : une implémentation par plateforme.
if(WIN32)
    message(FATAL_ERROR "UdpSocket pour Windows (Winsock) pas encore implémenté : voir T019.")
else()
    target_sources(odyssee_shared PRIVATE src/net/posix/UdpSocketPosix.cpp)
endif()

target_include_directories(odyssee_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(odyssee_shared PUBLIC
    ODYSSEE_VERSION_STRING="${PROJECT_VERSION}")
odyssee_set_options(odyssee_shared)
