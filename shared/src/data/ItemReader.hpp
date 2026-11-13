#pragma once

// Lecture interne de data/items.txt : blocs « [item …] » → ItemDef.

#include "odyssee/data/DataError.hpp"
#include "odyssee/data/DataText.hpp"
#include "odyssee/data/ItemDef.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odyssee::data {

inline constexpr std::uint32_t kMaxStackSize = 9999;

using ItemIndex = std::map<std::string, ItemId, std::less<>>;

struct ItemsRead {
    std::vector<ItemDef> items; ///< indice = ItemId
    ItemIndex byKey;
    std::optional<DataError> error;
};

/// Interprète les blocs d'objets ; `file` sert aux messages d'erreur.
[[nodiscard]] ItemsRead readItems(const std::vector<DataBlock>& blocks, std::string_view file);

} // namespace odyssee::data
