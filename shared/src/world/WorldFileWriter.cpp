#include "odyssee/world/WorldFile.hpp"

#include "WorldSections.hpp"
#include "odyssee/net/ByteBuffer.hpp"

#include <iterator>
#include <utility>

namespace odyssee::world {

namespace {

using detail::SectionTag;

void writeTag(net::ByteWriter& out, const SectionTag& tag) {
    for (const char c : tag) {
        out.writeU8(static_cast<std::uint8_t>(c));
    }
}

std::vector<std::byte> toBytes(const net::ByteWriter& writer) {
    const auto data = writer.data();
    return {data.begin(), data.end()};
}

std::vector<std::byte> terrainSection(const HeightField& terrain) {
    net::ByteWriter out;
    out.writeU32(terrain.quadsX);
    out.writeU32(terrain.quadsY);
    out.writeU32(terrain.quadSize);
    out.writeU16(terrain.chunkQuads);
    out.writeU16(0); // réservé
    out.writeU32(terrain.chunksX);
    out.writeU32(terrain.chunksY);
    for (const std::uint16_t sample : terrain.samples) {
        out.writeU16(sample);
    }
    return toBytes(out);
}

std::vector<std::byte> holesSection(const std::vector<QuadCoord>& holes) {
    net::ByteWriter out;
    out.writeU32(static_cast<std::uint32_t>(holes.size()));
    for (const QuadCoord& hole : holes) {
        out.writeU32(hole.col);
        out.writeU32(hole.row);
    }
    return toBytes(out);
}

std::vector<std::byte> boxesSection(const std::vector<CollisionBox>& boxes) {
    net::ByteWriter out;
    out.writeU32(static_cast<std::uint32_t>(boxes.size()));
    for (const CollisionBox& box : boxes) {
        for (const std::int32_t v : box.center) {
            out.writeI32(v);
        }
        for (const std::uint32_t v : box.halfExtents) {
            out.writeU32(v);
        }
        out.writeU16(box.yaw);
        out.writeU8(static_cast<std::uint8_t>(box.shape));
        out.writeU8(0); // réservé
    }
    return toBytes(out);
}

std::vector<std::byte> markersSection(const std::vector<Marker>& markers) {
    net::ByteWriter out;
    out.writeU32(static_cast<std::uint32_t>(markers.size()));
    for (const Marker& marker : markers) {
        out.writeU8(static_cast<std::uint8_t>(marker.kind));
        for (const std::int32_t v : marker.position) {
            out.writeI32(v);
        }
        out.writeU16(marker.yaw);
        out.writeString(marker.id);
    }
    return toBytes(out);
}

} // namespace

std::vector<std::byte> encodeWorldFile(const WorldFile& world) {
    const std::pair<SectionTag, std::vector<std::byte>> sections[] = {
        {detail::kTerrainTag, terrainSection(world.terrain)},
        {detail::kHolesTag, holesSection(world.holes)},
        {detail::kBoxesTag, boxesSection(world.boxes)},
        {detail::kMarkersTag, markersSection(world.markers)},
    };
    const auto count = static_cast<std::uint16_t>(std::size(sections));

    net::ByteWriter out;
    writeTag(out, kWorldMagic);
    out.writeU16(kWorldFormatVersion);
    out.writeU16(count);
    std::uint64_t offset = detail::kHeaderSize + count * detail::kDirectoryEntrySize;
    for (const auto& [tag, payload] : sections) {
        writeTag(out, tag);
        out.writeU64(offset);
        out.writeU64(payload.size());
        offset += payload.size();
    }
    for (const auto& section : sections) {
        out.writeBytes(section.second);
    }
    return toBytes(out);
}

} // namespace odyssee::world
