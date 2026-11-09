// Client Odyssée — prototype M0 : fenêtre 3D, caméra orbitale, sol et repères.
// Le client n'est pas autoritaire : à terme il envoie des inputs et affiche
// l'état reçu du serveur (avec prédiction locale + interpolation).
#include "odyssee/core/Config.hpp"

#include <raylib.h>

#include <string>

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1280, 720, "Odyssée");

    Camera3D camera{};
    camera.position = {10.0f, 10.0f, 10.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    const std::string title = std::string("Odyssée v") + ODYSSEE_VERSION_STRING + " — prototype";

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        BeginDrawing();
        ClearBackground(Color{135, 190, 230, 255});

        BeginMode3D(camera);
        DrawPlane({0.0f, 0.0f, 0.0f}, {40.0f, 40.0f}, Color{96, 160, 80, 255});
        DrawCube({0.0f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, Color{230, 140, 60, 255});
        DrawCubeWires({0.0f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, DARKBROWN);
        DrawGrid(40, 1.0f);
        EndMode3D();

        DrawText(title.c_str(), 16, 16, 20, RAYWHITE);
        DrawFPS(16, 44);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
