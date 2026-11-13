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
    test_invite_code.cpp
    test_control_messages.cpp
    test_client_connection.cpp
    test_quantize.cpp
    test_game_messages.cpp
    test_game_packet.cpp
    test_session.cpp
    test_fixed_math.cpp
    test_movement.cpp
    test_determinism.cpp
    test_network_simulator.cpp
    test_fuzz_decoders.cpp
    test_world_file.cpp
    test_world_terrain.cpp
    test_input_history.cpp
    test_prediction.cpp
    test_interpolation.cpp
    test_traffic_meter.cpp
)
target_link_libraries(odyssee_tests PRIVATE odyssee::shared doctest::doctest)
# Pour retrouver les fichiers du dépôt (la vraie carte) d'où que les tests soient lancés.
target_compile_definitions(odyssee_tests PRIVATE ODYSSEE_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# Les tests du serveur ne sont compilés que si le serveur l'est.
if(ODYSSEE_BUILD_SERVER)
    target_sources(odyssee_tests PRIVATE
        test_command_line.cpp
        test_client_registry.cpp
        test_siphash.cpp
        test_rate_limit.cpp
        test_handshake.cpp
        test_input_queue.cpp
        test_game_world.cpp
        test_ecs.cpp
        test_game_session.cpp
    )
    target_link_libraries(odyssee_tests PRIVATE odyssee_server_core)
endif()
odyssee_set_options(odyssee_tests)

add_test(NAME odyssee_tests COMMAND odyssee_tests)
