# Client de debug (raylib) : teste le réseau et la prédiction sous Linux, sans Unreal.
add_executable(odyssee_client
    src/main.cpp
    src/Options.cpp
    src/InputSampler.cpp
    src/Scene.cpp
    src/Hud.cpp
    src/DebugClient.cpp
)
target_link_libraries(odyssee_client PRIVATE odyssee::shared raylib)
odyssee_set_options(odyssee_client)
