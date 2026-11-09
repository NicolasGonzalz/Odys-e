add_executable(odyssee_server
    src/main.cpp
    src/Server.cpp
)
target_link_libraries(odyssee_server PRIVATE odyssee::shared)
odyssee_set_options(odyssee_server)
