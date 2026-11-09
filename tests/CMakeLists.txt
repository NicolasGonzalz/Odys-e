add_executable(odyssee_tests
    main.cpp
    test_bytebuffer.cpp
    test_log.cpp
    test_udp.cpp
)
target_link_libraries(odyssee_tests PRIVATE odyssee::shared doctest::doctest)
odyssee_set_options(odyssee_tests)

add_test(NAME odyssee_tests COMMAND odyssee_tests)
