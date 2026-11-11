#include "odyssee/net/ByteBuffer.hpp"

#include <bit>

namespace odyssee::net {

namespace {

template <typename T> void writeLittleEndian(std::vector<std::byte>& out, T v) {
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        out.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFF));
    }
}

} // namespace

void ByteWriter::writeU8(std::uint8_t v) { _data.push_back(static_cast<std::byte>(v)); }
void ByteWriter::writeU16(std::uint16_t v) { writeLittleEndian(_data, v); }
void ByteWriter::writeU32(std::uint32_t v) { writeLittleEndian(_data, v); }
void ByteWriter::writeU64(std::uint64_t v) { writeLittleEndian(_data, v); }
void ByteWriter::writeI8(std::int8_t v) { writeU8(std::bit_cast<std::uint8_t>(v)); }
void ByteWriter::writeI16(std::int16_t v) { writeU16(std::bit_cast<std::uint16_t>(v)); }
void ByteWriter::writeI32(std::int32_t v) { writeU32(std::bit_cast<std::uint32_t>(v)); }
void ByteWriter::writeF32(float v) { writeU32(std::bit_cast<std::uint32_t>(v)); }

void ByteWriter::writeString(std::string_view s) {
    const auto len = static_cast<std::uint16_t>(s.size() > 0xFFFF ? 0xFFFF : s.size());
    writeU16(len);
    for (std::size_t i = 0; i < len; ++i) {
        _data.push_back(static_cast<std::byte>(s[i]));
    }
}

void ByteWriter::writeBytes(std::span<const std::byte> bytes) {
    _data.insert(_data.end(), bytes.begin(), bytes.end());
}

template <typename T> std::optional<T> ByteReader::readUnsigned() {
    if (remaining() < sizeof(T)) {
        return std::nullopt;
    }
    T v = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        v = static_cast<T>(v | (static_cast<T>(std::to_integer<std::uint8_t>(_data[_offset + i]))
                                << (8 * i)));
    }
    _offset += sizeof(T);
    return v;
}

std::optional<std::uint8_t> ByteReader::readU8() { return readUnsigned<std::uint8_t>(); }
std::optional<std::uint16_t> ByteReader::readU16() { return readUnsigned<std::uint16_t>(); }
std::optional<std::uint32_t> ByteReader::readU32() { return readUnsigned<std::uint32_t>(); }
std::optional<std::uint64_t> ByteReader::readU64() { return readUnsigned<std::uint64_t>(); }

template <typename Signed, typename Unsigned> std::optional<Signed> ByteReader::readSigned() {
    const auto raw = readUnsigned<Unsigned>();
    if (!raw) {
        return std::nullopt;
    }
    return std::bit_cast<Signed>(*raw);
}

std::optional<std::int8_t> ByteReader::readI8() { return readSigned<std::int8_t, std::uint8_t>(); }
std::optional<std::int16_t> ByteReader::readI16() {
    return readSigned<std::int16_t, std::uint16_t>();
}
std::optional<std::int32_t> ByteReader::readI32() {
    return readSigned<std::int32_t, std::uint32_t>();
}

std::optional<float> ByteReader::readF32() {
    const auto raw = readU32();
    if (!raw) {
        return std::nullopt;
    }
    return std::bit_cast<float>(*raw);
}

std::optional<bool> ByteReader::readBool() {
    const auto raw = readU8();
    if (!raw || *raw > 1) {
        return std::nullopt;
    }
    return *raw == 1;
}

std::optional<std::string> ByteReader::readString(std::size_t maxLength) {
    const auto len = readU16();
    if (!len || *len > maxLength || remaining() < *len) {
        return std::nullopt;
    }
    std::string s(*len, '\0');
    for (std::size_t i = 0; i < *len; ++i) {
        s[i] = static_cast<char>(std::to_integer<std::uint8_t>(_data[_offset + i]));
    }
    _offset += *len;
    return s;
}

std::optional<std::vector<std::byte>> ByteReader::readBytes(std::size_t count) {
    if (remaining() < count) {
        return std::nullopt;
    }
    const auto first = _data.begin() + static_cast<std::ptrdiff_t>(_offset);
    std::vector<std::byte> bytes(first, first + static_cast<std::ptrdiff_t>(count));
    _offset += count;
    return bytes;
}

} // namespace odyssee::net
