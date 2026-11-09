#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::net {

/// Tampon d'écriture binaire. Tous les entiers sont encodés en little-endian,
/// indépendamment de la plateforme, pour garantir un format réseau stable.
class ByteWriter {
  public:
    void writeU8(std::uint8_t v);
    void writeU16(std::uint16_t v);
    void writeU32(std::uint32_t v);
    void writeU64(std::uint64_t v);
    void writeF32(float v);
    void writeBool(bool v) { writeU8(v ? 1 : 0); }
    /// Chaîne préfixée par sa longueur (u16).
    void writeString(std::string_view s);

    [[nodiscard]] std::span<const std::byte> data() const { return _data; }
    [[nodiscard]] std::size_t size() const { return _data.size(); }
    void clear() { _data.clear(); }

  private:
    std::vector<std::byte> _data;
};

/// Lecteur binaire non-propriétaire. Chaque lecture renvoie std::nullopt
/// si le tampon est trop court : une donnée réseau n'est jamais de confiance.
class ByteReader {
  public:
    explicit ByteReader(std::span<const std::byte> data) : _data(data) {}

    [[nodiscard]] std::optional<std::uint8_t> readU8();
    [[nodiscard]] std::optional<std::uint16_t> readU16();
    [[nodiscard]] std::optional<std::uint32_t> readU32();
    [[nodiscard]] std::optional<std::uint64_t> readU64();
    [[nodiscard]] std::optional<float> readF32();
    [[nodiscard]] std::optional<bool> readBool();
    [[nodiscard]] std::optional<std::string> readString(std::size_t maxLength = 256);

    [[nodiscard]] std::size_t remaining() const { return _data.size() - _offset; }

  private:
    template <typename T> std::optional<T> readUnsigned();

    std::span<const std::byte> _data;
    std::size_t _offset = 0;
};

} // namespace odyssee::net
