#include "odyssee/world/WorldFileLoader.hpp"

#include <fstream>
#include <vector>

namespace odyssee::world {

DecodedWorld loadWorldFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {std::nullopt, WorldFileError::CannotOpen};
    }
    const std::streamoff size = file.tellg();
    if (size < 0) {
        return {std::nullopt, WorldFileError::CannotOpen};
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), size)) {
        return {std::nullopt, WorldFileError::CannotOpen};
    }
    return decodeWorldFile(bytes);
}

} // namespace odyssee::world
