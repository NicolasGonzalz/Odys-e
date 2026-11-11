add_executable(odyssee_tests
    main.cpp
    test_bytebuffer.cpp
    test_log.cpp
    test_udp.cpp
    test_sequence.cpp
    test_packet_header.cpp
    test_received_packets.cpp
    test_acknowledgements.cpp
    test_reliable_codec.cpp
    test_reliable_sender.cpp
    test_reliable_receiver.cpp
    test_reliable_channel.cpp
    test_player_name.cpp
    test_control_messages.cpp
    test_client_connection.cpp
    test_quantize.cpp
    test_game_messages.cpp
    test_game_packet.cpp
    test_network_simulator.cpp
)
target_link_libraries(odyssee_tests PRIVATE odyssee::shared doctest::doctest)

# Les tests du serveur ne sont compilés que si le serveur l'est.
if(ODYSSEE_BUILD_SERVER)
    target_sources(odyssee_tests PRIVATE
        test_command_line.cpp
        test_client_registry.cpp
        test_handshake.cpp
    )
    target_link_libraries(odyssee_tests PRIVATE odyssee_server_core)
endif()
odyssee_set_options(odyssee_tests)

add_test(NAME odyssee_tests COMMAND odyssee_tests)
