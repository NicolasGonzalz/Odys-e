#include "odyssee/world/WorldFile.hpp"
#include "odyssee/world/WorldFingerprint.hpp"

#include "WorldSections.hpp"
#include "odyssee/net/ByteBuffer.hpp"

#include <algorithm>

namespace odyssee::world {

namespace {

using detail::SectionTag;

struct Section {
    SectionTag tag{};
    std::span<const std::byte> bytes;
};

std::optional<SectionTag> readTag(net::ByteReader& in) {
    SectionTag tag{};
    for (char& c : tag) {
        const auto byte = in.readU8();
        if (!byte) {
            return std::nullopt;
        }
        c = static_cast<char>(*byte);
    }
    return tag;
}

/// Une entrée du répertoire, si elle désigne bien une partie du fichier.
std::optional<Section> readEntry(net::ByteReader& in, std::span<const std::byte> file) {
    const auto tag = readTag(in);
    const auto offset = in.readU64();
    const auto size = in.readU64();
    if (!tag || !offset || !size || *offset > file.size() || *size > file.size() - *offset) {
        return std::nullopt;
    }
    return Section{
        *tag, file.subspan(static_cast<std::size_t>(*offset), static_cast<std::size_t>(*size))};
}

/// En-tête et répertoire. Une étiquette inconnue est gardée (et ignorée plus loin) : un fichier
/// d'une version compatible peut ajouter des sections.
WorldFileError readDirectory(std::span<const std::byte> file, std::vector<Section>& sections) {
    net::ByteReader in(file);
    const auto magic = readTag(in);
    const auto version = in.readU16();
    const auto count = in.readU16();
    if (!magic || !version || !count) {
        return WorldFileError::Truncated;
    }
    if (*magic != kWorldMagic) {
        return WorldFileError::BadMagic;
    }
    if (*version != kWorldFormatVersion) {
        return WorldFileError::UnsupportedVersion;
    }
    if (*count > detail::kMaxSections) {
        return WorldFileError::BadDirectory;
    }
    for (std::uint16_t i = 0; i < *count; ++i) {
        const auto section = readEntry(in, file);
        if (!section) {
            return WorldFileError::BadDirectory;
        }
        sections.push_back(*section);
    }
    return WorldFileError::None;
}

/// La section `tag`, si elle est présente une seule fois (nullopt si absente).
std::optional<std::span<const std::byte>> findSection(const std::vector<Section>& sections,
                                                      const SectionTag& tag, bool& duplicated) {
    const auto matches = std::count_if(sections.begin(), sections.end(),
                                       [&](const Section& s) { return s.tag == tag; });
    duplicated = matches > 1;
    const auto it = std::find_if(sections.begin(), sections.end(),
                                 [&](const Section& s) { return s.tag == tag; });
    if (it == sections.end()) {
        return std::nullopt;
    }
    return it->bytes;
}

std::uint32_t chunksFor(std::uint32_t quads, std::uint16_t chunkQuads) {
    return (quads + chunkQuads - 1) / chunkQuads;
}

bool validTerrainShape(const HeightField& t) {
    return t.quadsX > 0 && t.quadsY > 0 && t.quadsX <= detail::kMaxQuadsPerSide &&
           t.quadsY <= detail::kMaxQuadsPerSide && t.quadSize > 0 && t.chunkQuads > 0 &&
           t.chunkQuads <= detail::kMaxChunkQuads &&
           t.chunksX == chunksFor(t.quadsX, t.chunkQuads) &&
           t.chunksY == chunksFor(t.quadsY, t.chunkQuads);
}

std::optional<HeightField> readTerrainShape(net::ByteReader& in) {
    HeightField t;
    const auto quadsX = in.readU32();
    const auto quadsY = in.readU32();
    const auto quadSize = in.readU32();
    const auto chunkQuads = in.readU16();
    const auto reserved = in.readU16();
    const auto chunksX = in.readU32();
    const auto chunksY = in.readU32();
    if (!quadsX || !quadsY || !quadSize || !chunkQuads || !reserved || !chunksX || !chunksY) {
        return std::nullopt;
    }
    t.quadsX = *quadsX;
    t.quadsY = *quadsY;
    t.quadSize = *quadSize;
    t.chunkQuads = *chunkQuads;
    t.chunksX = *chunksX;
    t.chunksY = *chunksY;
    return validTerrainShape(t) ? std::optional<HeightField>(t) : std::nullopt;
}

std::optional<HeightField> readTerrain(std::span<const std::byte> bytes) {
    net::ByteReader in(bytes);
    auto terrain = readTerrainShape(in);
    if (!terrain) {
        return std::nullopt;
    }
    const std::size_t count =
        std::size_t{terrain->chunksX} * terrain->chunksY * terrain->samplesPerChunk();
    if (in.remaining() != count * 2) {
        return std::nullopt;
    }
    terrain->samples.resize(count);
    for (std::uint16_t& sample : terrain->samples) {
        sample = *in.readU16(); // taille vérifiée juste au-dessus
    }
    return terrain;
}

/// Nombre d'enregistrements annoncé, s'il tient exactement dans la section.
std::optional<std::uint32_t> readCount(net::ByteReader& in, std::size_t recordSize) {
    const auto count = in.readU32();
    if (!count || in.remaining() != std::size_t{*count} * recordSize) {
        return std::nullopt;
    }
    return count;
}

std::optional<std::vector<QuadCoord>> readHoles(std::span<const std::byte> bytes,
                                                const HeightField& terrain) {
    net::ByteReader in(bytes);
    const auto count = readCount(in, detail::kHoleRecordSize);
    if (!count) {
        return std::nullopt;
    }
    std::vector<QuadCoord> holes(*count);
    for (QuadCoord& hole : holes) {
        hole = {*in.readU32(), *in.readU32()};
        if (hole.col >= terrain.quadsX || hole.row >= terrain.quadsY) {
            return std::nullopt;
        }
    }
    const auto sorted = std::adjacent_find(holes.begin(), holes.end(), [](auto& a, auto& b) {
                            return !rowMajorLess(a, b);
                        }) == holes.end();
    return sorted ? std::optional(std::move(holes)) : std::nullopt;
}

std::optional<CollisionBox> readBox(net::ByteReader& in) {
    CollisionBox box;
    for (std::int32_t& v : box.center) {
        v = *in.readI32();
    }
    for (std::uint32_t& v : box.halfExtents) {
        v = *in.readU32();
    }
    box.yaw = *in.readU16();
    const std::uint8_t shape = *in.readU8();
    const std::uint8_t reserved = *in.readU8();
    const bool empty =
        std::find(box.halfExtents.begin(), box.halfExtents.end(), 0U) != box.halfExtents.end();
    if (shape > static_cast<std::uint8_t>(BoxShape::Cylinder) || reserved != 0 || empty) {
        return std::nullopt;
    }
    box.shape = static_cast<BoxShape>(shape);
    return box;
}

std::optional<std::vector<CollisionBox>> readBoxes(std::span<const std::byte> bytes) {
    net::ByteReader in(bytes);
    const auto count = readCount(in, detail::kBoxRecordSize); // tailles fixes : lectures sûres
    if (!count) {
        return std::nullopt;
    }
    std::vector<CollisionBox> boxes;
    boxes.reserve(*count);
    for (std::uint32_t i = 0; i < *count; ++i) {
        const auto box = readBox(in);
        if (!box) {
            return std::nullopt;
        }
        boxes.push_back(*box);
    }
    return boxes;
}

std::optional<Marker> readMarker(net::ByteReader& in) {
    Marker marker;
    const auto kind = in.readU8();
    std::optional<std::int32_t> position[3] = {in.readI32(), in.readI32(), in.readI32()};
    const auto yaw = in.readU16();
    auto id = in.readString(detail::kMaxMarkerIdLength);
    if (!kind || !position[0] || !position[1] || !position[2] || !yaw || !id ||
        *kind > static_cast<std::uint8_t>(MarkerKind::Cave)) {
        return std::nullopt;
    }
    marker.kind = static_cast<MarkerKind>(*kind);
    marker.position = {*position[0], *position[1], *position[2]};
    marker.yaw = *yaw;
    marker.id = std::move(*id);
    return marker;
}

std::optional<std::vector<Marker>> readMarkers(std::span<const std::byte> bytes) {
    net::ByteReader in(bytes);
    const auto count = in.readU32();
    if (!count) {
        return std::nullopt;
    }
    std::vector<Marker> markers;
    for (std::uint32_t i = 0; i < *count; ++i) {
        auto marker = readMarker(in); // taille variable : chaque lecture est vérifiée
        if (!marker) {
            return std::nullopt;
        }
        markers.push_back(std::move(*marker));
    }
    return in.remaining() == 0 ? std::optional(std::move(markers)) : std::nullopt;
}

/// Sections facultatives : absentes, elles sont vides.
template <typename T, typename Reader>
WorldFileError readOptional(const std::vector<Section>& sections, const SectionTag& tag,
                            WorldFileError onError, T& target, Reader read) {
    bool duplicated = false;
    const auto bytes = findSection(sections, tag, duplicated);
    if (duplicated) {
        return WorldFileError::BadDirectory;
    }
    if (bytes) {
        auto value = read(*bytes);
        if (!value) {
            return onError;
        }
        target = std::move(*value);
    }
    return WorldFileError::None;
}

WorldFileError readTerrainSection(const std::vector<Section>& sections, WorldFile& world) {
    bool duplicated = false;
    const auto bytes = findSection(sections, detail::kTerrainTag, duplicated);
    if (duplicated) {
        return WorldFileError::BadDirectory;
    }
    if (!bytes) {
        return WorldFileError::MissingTerrain;
    }
    auto terrain = readTerrain(*bytes);
    if (!terrain) {
        return WorldFileError::BadTerrain;
    }
    world.terrain = std::move(*terrain);
    return WorldFileError::None;
}

WorldFileError readSections(const std::vector<Section>& sections, WorldFile& world) {
    WorldFileError error = readTerrainSection(sections, world);
    if (error == WorldFileError::None) {
        error = readOptional(sections, detail::kHolesTag, WorldFileError::BadHoles, world.holes,
                             [&](auto bytes) { return readHoles(bytes, world.terrain); });
    }
    if (error == WorldFileError::None) {
        error = readOptional(sections, detail::kBoxesTag, WorldFileError::BadBoxes, world.boxes,
                             readBoxes);
    }
    if (error == WorldFileError::None) {
        error = readOptional(sections, detail::kMarkersTag, WorldFileError::BadMarkers,
                             world.markers, readMarkers);
    }
    return error;
}

} // namespace

DecodedWorld decodeWorldFile(std::span<const std::byte> bytes) {
    std::vector<Section> sections;
    if (const WorldFileError error = readDirectory(bytes, sections);
        error != WorldFileError::None) {
        return {std::nullopt, error};
    }
    WorldFile world;
    if (const WorldFileError error = readSections(sections, world); error != WorldFileError::None) {
        return {std::nullopt, error};
    }
    world.fingerprint = fingerprintOf(bytes);
    return {std::move(world), WorldFileError::None};
}

const char* describe(WorldFileError error) {
    switch (error) {
    case WorldFileError::None:
        return "aucune erreur";
    case WorldFileError::BadMagic:
        return "ce n'est pas un fichier monde (signature)";
    case WorldFileError::UnsupportedVersion:
        return "version du format non prise en charge";
    case WorldFileError::Truncated:
        return "fichier tronqué";
    case WorldFileError::CannotOpen:
        return "fichier introuvable ou illisible";
    case WorldFileError::BadDirectory:
        return "répertoire des sections invalide";
    case WorldFileError::MissingTerrain:
        return "section du relief (TERR) absente";
    case WorldFileError::BadTerrain:
        return "section du relief (TERR) invalide";
    case WorldFileError::BadHoles:
        return "section des trous (HOLE) invalide";
    case WorldFileError::BadBoxes:
        return "section des collisions (BOXS) invalide";
    case WorldFileError::BadMarkers:
        return "section des marqueurs (MARK) invalide";
    }
    return "erreur inconnue";
}

} // namespace odyssee::world
