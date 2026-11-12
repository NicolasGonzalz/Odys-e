#pragma once

#include "odyssee/world/WorldFile.hpp"

#include <raylib.h>

#include <cstdint>
#include <map>
#include <utility>

namespace odyssee::debugclient {

/// Rendu de la vraie carte (T041) dans le client de debug : le relief et les volumes de
/// collision, tels que la simulation les voit (pas de décor : c'est un outil de test).
///
/// Le relief entier ferait 24 millions de triangles : seuls les blocs (64 m de côté) proches
/// du joueur sont transformés en maillages, au fil de ses déplacements. Il faut une fenêtre
/// ouverte (contexte OpenGL) pendant toute la vie de l'objet.
class MapView {
  public:
    explicit MapView(const world::WorldFile& world);
    ~MapView();

    MapView(const MapView&) = delete;
    MapView& operator=(const MapView&) = delete;

    /// Prépare les blocs autour de `around` (point raylib, en mètres) et oublie les lointains.
    void update(Vector3 around);
    /// Dessine le relief préparé et les volumes proches de `around` (dans BeginMode3D).
    void draw(Vector3 around) const;

  private:
    using ChunkKey = std::pair<std::int64_t, std::int64_t>; ///< (colonne, ligne) du bloc

    void forgetFarChunks(ChunkKey center);
    [[nodiscard]] Model buildChunk(ChunkKey key) const;
    [[nodiscard]] bool isNear(ChunkKey key, ChunkKey center) const;

    const world::WorldFile& _world;
    std::map<ChunkKey, Model> _chunks;
};

} // namespace odyssee::debugclient
