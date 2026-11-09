add_executable(odyssee_client
    src/main.cpp
)
target_link_libraries(odyssee_client PRIVATE odyssee::shared raylib)
odyssee_set_options(odyssee_client)
