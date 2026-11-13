#include "ItemReader.hpp"

#include "DataValues.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace odyssee::data {

namespace {

static_assert(kMaxBlocks <= std::numeric_limits<std::uint16_t>::max(),
              "chaque objet doit tenir dans un ItemId sur 16 bits");

constexpr std::array<std::string_view, 4> kAllowedKeys{"name", "stack", "category", "model"};
constexpr std::array<std::string_view, 3> kRequiredKeys{"name", "stack", "category"};

/// Lit un bloc dont les clés sont déjà vérifiées. Erreur, ou nullopt si `item` est rempli.
class ItemBlockReader {
  public:
    ItemBlockReader(const DataBlock& block, std::string_view file) : _block(block), _file(file) {}

    std::optional<DataError> read(ItemDef& item) const {
        if (auto error = readName(item)) {
            return error;
        }
        if (auto error = readStack(item)) {
            return error;
        }
        if (auto error = readCategory(item)) {
            return error;
        }
        return readModel(item);
    }

  private:
    std::optional<DataError> readName(ItemDef& item) const {
        const DataField& field = *findField(_block, "name");
        if (field.value.size() > kMaxNameLength) {
            return fail(field,
                        "nom trop long (maximum " + std::to_string(kMaxNameLength) + " octets)");
        }
        item.name = std::string(field.value);
        return std::nullopt;
    }

    std::optional<DataError> readStack(ItemDef& item) const {
        const DataField& field = *findField(_block, "stack");
        const auto stack = parseNumber(field.value, 1, kMaxStackSize);
        if (!stack) {
            return fail(field, "pile max " + quoted(field.value) + " invalide : entier de 1 à " +
                                   std::to_string(kMaxStackSize) + " attendu");
        }
        item.maxStack = static_cast<std::uint16_t>(*stack);
        return std::nullopt;
    }

    std::optional<DataError> readCategory(ItemDef& item) const {
        const DataField& field = *findField(_block, "category");
        const auto category = parseItemCategory(field.value);
        if (!category) {
            return fail(field, "catégorie inconnue " + quoted(field.value) +
                                   " (resource, tool, weapon, ammo, station, consumable, "
                                   "equipment)");
        }
        item.category = *category;
        return std::nullopt;
    }

    std::optional<DataError> readModel(ItemDef& item) const {
        const DataField* field = findField(_block, "model");
        if (field == nullptr) {
            return std::nullopt;
        }
        if (!isValidModelPath(field->value)) {
            return fail(*field, "chemin de modèle " + quoted(field->value) +
                                    " invalide (ex. « outils/hache_pierre », sans extension)");
        }
        item.model = std::string(field->value);
        return std::nullopt;
    }

    DataError fail(const DataField& field, std::string message) const {
        return makeError(_file, field.line, std::move(message));
    }

    const DataBlock& _block;
    std::string_view _file;
};

/// En-tête et clés du bloc ; erreur, ou nullopt si on peut le lire.
std::optional<DataError> checkItemBlock(const DataBlock& block, const ItemIndex& known,
                                        std::string_view file) {
    if (block.kind != "item") {
        return makeError(file, block.line,
                         "type de bloc " + quoted(block.kind) + " inattendu : « item » attendu");
    }
    if (!isValidKey(block.id)) {
        return makeError(file, block.line,
                         "identifiant " + quoted(block.id) +
                             " invalide (minuscules, chiffres et « _ », ex. « stone_axe »)");
    }
    if (known.contains(block.id)) {
        return makeError(file, block.line, "objet " + quoted(block.id) + " déjà défini");
    }
    return checkBlockKeys(block, {kAllowedKeys, {}, kRequiredKeys}, file);
}

} // namespace

ItemsRead readItems(const std::vector<DataBlock>& blocks, std::string_view file) {
    ItemsRead result;
    for (const DataBlock& block : blocks) {
        if (auto error = checkItemBlock(block, result.byKey, file)) {
            result.error = std::move(error);
            return result;
        }

        ItemDef item;
        item.id = ItemId{static_cast<std::uint16_t>(result.items.size())};
        item.key = std::string(block.id);
        if (auto error = ItemBlockReader(block, file).read(item)) {
            result.error = std::move(error);
            return result;
        }
        result.byKey.emplace(item.key, item.id);
        result.items.push_back(std::move(item));
    }
    return result;
}

} // namespace odyssee::data
