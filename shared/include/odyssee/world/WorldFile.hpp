#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace odyssee::world {

/// Fichier monde (docs/WORLD_FORMAT.md) : tout ce que la simulation doit savoir de la carte,
/// sans Unreal. Écrit par tools/worldgen/export_world.py, lu par le serveur et par le client
/// (prédiction : les deux doivent voir exactement le même sol et les mêmes murs).
///
/// Unités de la simulation : positions en 1/256 m (net::quantizePosition), x vers l'est, y vers le
/// sud, z vers le haut ; cap sur 16 bits, un tour = 65536, de +x vers +y (net::quantizeYaw).

inline constexpr std::array<char, 4> kWorldMagic{'O', 'D', 'Y', 'W'};
inline constexpr std::uint16_t kWorldFormatVersion = 1;

/// Relief : une grille de points tous les `quadSize`, rangée par blocs carrés (chunks) de
/// `chunkQuads` cases, pour pouvoir n'en charger qu'une partie. Chaque bloc garde ses points de
/// bord (chunkQuads + 1 de côté) : un bloc se suffit à lui-même.
struct HeightField {
    std::uint32_t quadsX = 0;     ///< cases d'est en ouest (points : quadsX + 1)
    std::uint32_t quadsY = 0;     ///< cases du nord au sud (points : quadsY + 1)
    std::uint32_t quadSize = 256; ///< côté d'une case, en 1/256 m
    std::uint16_t chunkQuads = 64;
    std::uint32_t chunksX = 0;
    std::uint32_t chunksY = 0;
    /// Hauteurs brutes, bloc par bloc (blocs en lignes, puis points en lignes dans chaque bloc) :
    /// z en 1/256 m = (brut - 32768) × 4. Même codage que output/heightmap.png.
    std::vector<std::uint16_t> samples;

    [[nodiscard]] std::size_t samplesPerChunk() const;
    /// Hauteur brute du point (col, row), bornée aux bords de la carte.
    [[nodiscard]] std::uint16_t raw(std::int64_t col, std::int64_t row) const;
    /// Hauteur du point (col, row) en 1/256 m.
    [[nodiscard]] std::int32_t height(std::int64_t col, std::int64_t row) const;
    /// Hauteur de la surface en (x, y), en 1/256 m : interpolée sur les deux triangles de la
    /// case (diagonale de (col, row) à (col + 1, row + 1)). Hors de la carte, celle du bord.
    [[nodiscard]] std::int32_t surfaceHeight(std::int32_t x, std::int32_t y) const;
    /// Largeur et profondeur de la carte, en 1/256 m.
    [[nodiscard]] std::int64_t width() const;
    [[nodiscard]] std::int64_t depth() const;
};

/// Hauteur brute → 1/256 m, et l'inverse (arrondi ; bornée à l'intervalle du codage).
[[nodiscard]] std::int32_t heightFromRaw(std::uint16_t raw);
[[nodiscard]] std::uint16_t rawFromHeight(std::int32_t units);

/// Une case du relief (entre les points (col, row) et (col + 1, row + 1)).
struct QuadCoord {
    std::uint32_t col = 0;
    std::uint32_t row = 0;

    friend bool operator==(const QuadCoord&, const QuadCoord&) = default;
};

/// Ordre du fichier : ligne par ligne, puis colonne.
[[nodiscard]] bool rowMajorLess(const QuadCoord& a, const QuadCoord& b);

enum class BoxShape : std::uint8_t {
    Box = 0,      ///< pavé
    Cylinder = 1, ///< cylindre vertical (section elliptique de demi-axes halfExtents x et y)
};

/// Volume de collision : un bloc du blockout (murs, sols, plafonds, bouchons…), tourné autour
/// de son centre.
struct CollisionBox {
    std::array<std::int32_t, 3> center{};
    std::array<std::uint32_t, 3> halfExtents{};
    std::uint16_t yaw = 0;
    BoxShape shape = BoxShape::Box;

    friend bool operator==(const CollisionBox&, const CollisionBox&) = default;
};

enum class MarkerKind : std::uint8_t {
    PlayerSpawn = 0, ///< apparition des joueurs
    Place = 1,       ///< lieu nommé (village, ferme…)
    Cave = 2,        ///< entrée de grotte
};

/// Point d'intérêt pour le gameplay. Les marqueurs Unreal (T038) s'y ajouteront.
struct Marker {
    MarkerKind kind = MarkerKind::Place;
    std::array<std::int32_t, 3> position{};
    std::uint16_t yaw = 0;
    std::string id; ///< identifiant de world_layout.json (« centre_stase », « grotte_03 »…)

    friend bool operator==(const Marker&, const Marker&) = default;
};

struct WorldFile {
    HeightField terrain;
    /// Cases percées (entrées de grotte) : ni sol ni collision. Triées (rowMajorLess), sans
    /// doublon.
    std::vector<QuadCoord> holes;
    std::vector<CollisionBox> boxes;
    std::vector<Marker> markers;
    /// Empreinte du fichier lu (fingerprintOf ; rempli par decodeWorldFile, pas écrit dans le
    /// fichier). Le serveur l'annonce, le client vérifie qu'il a la même carte.
    std::uint64_t fingerprint = 0;

    [[nodiscard]] bool isHole(QuadCoord quad) const;
};

enum class WorldFileError : std::uint8_t {
    None,
    BadMagic,
    UnsupportedVersion,
    Truncated,
    CannotOpen,
    BadDirectory,
    MissingTerrain,
    BadTerrain,
    BadHoles,
    BadBoxes,
    BadMarkers,
};

[[nodiscard]] const char* describe(WorldFileError error);

struct DecodedWorld {
    std::optional<WorldFile> world;
    WorldFileError error = WorldFileError::None;
};

/// Lit un fichier monde complet. Rien n'est supposé valide : toute incohérence est une erreur.
[[nodiscard]] DecodedWorld decodeWorldFile(std::span<const std::byte> bytes);

/// Écrit un fichier monde (tests, et outils C++ éventuels ; l'outil de référence est en Python).
[[nodiscard]] std::vector<std::byte> encodeWorldFile(const WorldFile& world);

} // namespace odyssee::world
