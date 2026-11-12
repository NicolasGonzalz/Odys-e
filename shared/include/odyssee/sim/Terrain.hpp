#pragma once

#include <cstdint>

namespace odyssee::sim {

/// Hauteur renvoyée quand il n'y a aucun sol sous un point (on tombe).
inline constexpr std::int32_t kNoGround = -(1 << 30);

/// Le sol tel que le voit la simulation. Positions et hauteurs en 1/256 m (comme
/// net::quantizePosition), x vers l'est, y vers le sud, z vers le haut. Toutes les réponses sont
/// des entiers calculés de façon identique sur le client et le serveur (déterminisme).
///
/// Un terrain simple n'a qu'une hauteur par point (groundHeight). La carte (T041) a aussi des
/// étages : le bunker de départ, avec un sol sous le relief et un plafond. D'où deux questions de
/// plus, qui par défaut retombent sur le cas simple :
/// - groundBelow : le sol le plus haut sous des pieds placés à z ;
/// - isSolid : un volume solide (mur, plafond) occupe-t-il une partie d'une tranche de hauteur.
class Terrain {
  public:
    virtual ~Terrain() = default;

    /// Hauteur du sol au point (x, y), en ignorant les étages (le plus haut).
    [[nodiscard]] virtual std::int32_t groundHeight(std::int32_t x, std::int32_t y) const = 0;

    /// Hauteur du sol le plus haut au point (x, y) qui ne dépasse pas `z` ; kNoGround s'il n'y en
    /// a aucun (le joueur tombe).
    [[nodiscard]] virtual std::int32_t groundBelow(std::int32_t x, std::int32_t y,
                                                   std::int32_t /*z*/) const {
        return groundHeight(x, y);
    }

    /// Vrai si un volume solide occupe, au point (x, y), une partie de la tranche ]bottom, top[.
    [[nodiscard]] virtual bool isSolid(std::int32_t /*x*/, std::int32_t /*y*/,
                                       std::int32_t /*bottom*/, std::int32_t /*top*/) const {
        return false;
    }
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
