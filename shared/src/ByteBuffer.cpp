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
void ByteWriter::writeF32(float v) { writeU32(std::bit_cast<std::uint32_t>(v)); }

void ByteWriter::writeString(std::string_view s) {
    const auto len = static_cast<std::uint16_t>(s.size() > 0xFFFF ? 0xFFFF : s.size());
    writeU16(len);
    for (std::size_t i = 0; i < len; ++i) {
        _data.push_back(static_cast<std::byte>(s[i]));
    }
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

} // namespace odyssee::net
