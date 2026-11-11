#pragma once

#include <cstdint>

namespace odyssee::sim {

/// Le sol tel que le voit la simulation : une hauteur en tout point. Positions et hauteurs en
/// 1/256 m (comme net::quantizePosition), x vers l'est, y vers le sud.
///
/// La vraie carte (heightmap, T041) devra elle aussi répondre en entiers, de façon identique
/// sur le client et le serveur.
class Terrain {
  public:
    virtual ~Terrain() = default;

    [[nodiscard]] virtual std::int32_t groundHeight(std::int32_t x, std::int32_t y) const = 0;
};

/// Sol plat à une hauteur donnée : tests, et serveur en attendant la carte (T041).
class FlatTerrain final : public Terrain {
  public:
    explicit FlatTerrain(std::int32_t height) : _height(height) {}

    [[nodiscard]] std::int32_t groundHeight(std::int32_t, std::int32_t) const override {
        return _height;
    }

  private:
    std::int32_t _height;
};

} // namespace odyssee::sim
