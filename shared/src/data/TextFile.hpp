#pragma once

// Lecture interne d'un fichier texte de données, de taille bornée.

#include <cstddef>
#include <optional>
#include <string>

namespace odyssee::data {

inline constexpr std::size_t kMaxDataFileSize = 1024 * 1024; ///< 1 Mio : large pour du texte

/// Contenu du fichier `path`, ou nullopt s'il est absent, illisible ou plus gros que la limite.
[[nodiscard]] std::optional<std::string> readTextFile(const std::string& path);

} // namespace odyssee::data
