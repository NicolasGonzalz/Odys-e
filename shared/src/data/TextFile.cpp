#include "TextFile.hpp"

#include <fstream>

namespace odyssee::data {

std::optional<std::string> readTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return std::nullopt;
    }
    const std::streamoff size = file.tellg();
    if (size < 0 || static_cast<std::size_t>(size) > kMaxDataFileSize) {
        return std::nullopt;
    }

    std::string text(static_cast<std::size_t>(size), '\0');
    file.seekg(0);
    if (!file.read(text.data(), size)) {
        return std::nullopt;
    }
    return text;
}

} // namespace odyssee::data
