#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace odyssee::world::detail {

/// Découpage du fichier (docs/WORLD_FORMAT.md) : partagé par la lecture et l'écriture.

using SectionTag = std::array<char, 4>;

inline constexpr SectionTag kTerrainTag{'T', 'E', 'R', 'R'};
inline constexpr SectionTag kHolesTag{'H', 'O', 'L', 'E'};
inline constexpr SectionTag kBoxesTag{'B', 'O', 'X', 'S'};
inline constexpr SectionTag kMarkersTag{'M', 'A', 'R', 'K'};

/// Signature (4) + version (2) + nombre de sections (2).
inline constexpr std::size_t kHeaderSize = 8;
/// Étiquette (4) + position depuis le début du fichier (8) + taille (8).
inline constexpr std::size_t kDirectoryEntrySize = 20;
/// Dimensions du relief avant ses hauteurs.
inline constexpr std::size_t kTerrainHeaderSize = 24;
/// Centre (3 × 4) + demi-tailles (3 × 4) + cap (2) + forme (1) + réservé (1).
inline constexpr std::size_t kBoxRecordSize = 28;
inline constexpr std::size_t kHoleRecordSize = 8;

/// Bornes de bon sens : un fichier corrompu ne doit pas faire allouer des gigaoctets.
inline constexpr std::uint32_t kMaxQuadsPerSide = 1U << 16;
inline constexpr std::uint16_t kMaxChunkQuads = 1024;
inline constexpr std::size_t kMaxMarkerIdLength = 64;
inline constexpr std::uint16_t kMaxSections = 64;

} // namespace odyssee::world::detail
