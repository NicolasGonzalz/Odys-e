#pragma once

#include "odyssee/world/WorldFile.hpp"

#include <string>

namespace odyssee::world {

/// Lit et décode un fichier monde sur le disque (CannotOpen s'il est absent ou illisible).
[[nodiscard]] DecodedWorld loadWorldFile(const std::string& path);

} // namespace odyssee::world
