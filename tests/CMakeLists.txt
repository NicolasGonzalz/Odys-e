add_executable(odyssee_tests
    main.cpp
    test_bytebuffer.cpp
    test_log.cpp
    test_udp.cpp
    test_sequence.cpp
    test_packet_header.cpp
    test_received_packets.cpp
    test_acknowledgements.cpp
)
target_link_libraries(odyssee_tests PRIVATE odyssee::shared doctest::doctest)

# Les tests du serveur ne sont compilés que si le serveur l'est.
if(ODYSSEE_BUILD_SERVER)
    target_sources(odyssee_tests PRIVATE test_command_line.cpp)
    target_link_libraries(odyssee_tests PRIVATE odyssee_server_core)
endif()
odyssee_set_options(odyssee_tests)

add_test(NAME odyssee_tests COMMAND odyssee_tests)
