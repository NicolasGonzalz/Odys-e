# Code partagé client/serveur : protocole, sérialisation, simulation déterministe.
add_library(odyssee_shared STATIC
    src/ByteBuffer.cpp
)
add_library(odyssee::shared ALIAS odyssee_shared)

target_include_directories(odyssee_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(odyssee_shared PUBLIC
    ODYSSEE_VERSION_STRING="${PROJECT_VERSION}")
odyssee_set_options(odyssee_shared)
