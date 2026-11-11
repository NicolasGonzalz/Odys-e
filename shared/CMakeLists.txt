# Code partagé client/serveur : protocole, sérialisation, simulation déterministe.
add_library(odyssee_shared STATIC
    src/ByteBuffer.cpp
    src/Log.cpp
    src/net/Acknowledgements.cpp
    src/net/Address.cpp
    src/net/ClientConnection.cpp
    src/net/ControlMessages.cpp
    src/net/GameMessages.cpp
    src/net/GamePacket.cpp
    src/net/PacketFraming.cpp
    src/net/PlayerName.cpp
    src/net/Quantize.cpp
    src/net/PacketHeader.cpp
    src/net/ReceivedPackets.cpp
    src/net/ReliableCodec.cpp
    src/net/ReliableReceiver.cpp
    src/net/ReliableSender.cpp
)
add_library(odyssee::shared ALIAS odyssee_shared)

# Socket UDP : une implémentation par plateforme.
if(WIN32)
    target_sources(odyssee_shared PRIVATE
        src/net/win32/UdpSocketWin32.cpp
        src/net/win32/WinsockRuntime.cpp
    )
    target_link_libraries(odyssee_shared PRIVATE ws2_32) # bibliothèque Winsock
else()
    target_sources(odyssee_shared PRIVATE src/net/posix/UdpSocketPosix.cpp)
endif()

target_include_directories(odyssee_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(odyssee_shared PUBLIC
    ODYSSEE_VERSION_STRING="${PROJECT_VERSION}")
odyssee_set_options(odyssee_shared)
