#include "ResourceReader.hpp"

#include "DataValues.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <utility>

namespace odyssee::data {

namespace {

constexpr std::array<std::string_view, 9> kAllowedKeys{
    "name", "shape", "item", "hits", "hit_ticks", "tool", "hand", "regrow", "abundance"};
constexpr std::array<std::string_view, 1> kRepeatableKeys{"tool"};
constexpr std::array<std::string_view, 7> kRequiredKeys{"name",      "shape",  "item",     "hits",
                                                        "hit_ticks", "regrow", "abundance"};

using ResourceKeys = std::set<std::string, std::less<>>;

/// Lit un bloc dont les clés sont déjà vérifiées. Erreur, ou nullopt si `resource` est rempli.
class ResourceBlockReader {
  public:
    ResourceBlockReader(const DataBlock& block, const Catalog& items, std::string_view file)
        : _block(block), _items(items), _file(file) {}

    std::optional<DataError> read(ResourceDef& resource) const {
        if (auto error = readName(resource)) {
            return error;
        }
        if (auto error = readShape(resource)) {
            return error;
        }
        if (auto error = readYield(resource)) {
            return error;
        }
        if (auto error = readTimings(resource)) {
            return error;
        }
        if (auto error = readTools(resource)) {
            return error;
        }
        return readHand(resource);
    }

  private:
    std::optional<DataError> readName(ResourceDef& resource) const {
        const DataField& field = *findField(_block, "name");
        if (field.value.size() > kMaxNameLength) {
            return fail(field,
                        "nom trop long (maximum " + std::to_string(kMaxNameLength) + " octets)");
        }
        resource.name = std::string(field.value);
        return std::nullopt;
    }

    std::optional<DataError> readShape(ResourceDef& resource) const {
        const DataField& field = *findField(_block, "shape");
        const auto shape = parseResourceShape(field.value);
        if (!shape) {
            return fail(field,
                        "forme inconnue " + quoted(field.value) + " (tree, rock, bush, vein)");
        }
        resource.shape = *shape;
        return std::nullopt;
    }

    /// `item = identifiant quantité` : objet connu, quantité au plus sa pile max.
    std::optional<DataError> readYield(ResourceDef& resource) const {
        const DataField& field = *findField(_block, "item");
        const std::size_t space = field.value.find_first_of(" \t");
        if (space == std::string_view::npos) {
            return fail(field, "« identifiant quantité » attendu (ex. « wood_log 2 »)");
        }
        const std::string_view key = field.value.substr(0, space);
        // La valeur est sans blanc final (parseDataText) : il reste au moins un caractère.
        const std::string_view countText =
            field.value.substr(field.value.find_first_not_of(" \t", space));
        const auto item = _items.findItem(key);
        if (!item) {
            return fail(field, "objet inconnu " + quoted(key) + " (absent de items.txt)");
        }
        const std::uint16_t maxStack = _items.item(*item)->maxStack;
        const auto count = parseNumber(countText, 1, maxStack);
        if (!count) {
            return fail(field, "quantité " + quoted(countText) + " invalide : entier de 1 à " +
                                   std::to_string(maxStack) + " (pile max de l'objet) attendu");
        }
        resource.yield = ItemStack{*item, static_cast<std::uint16_t>(*count)};
        return std::nullopt;
    }

    std::optional<DataError> readTimings(ResourceDef& resource) const {
        std::uint32_t hits = 0;
        std::uint32_t abundance = 0;
        if (auto error = readNumber("hits", 1, kMaxResourceHits, "nombre de coups", hits)) {
            return error;
        }
        if (auto error =
                readNumber("hit_ticks", 1, kMaxHitTicks, "durée d'un coup", resource.hitTicks)) {
            return error;
        }
        if (auto error = readNumber("regrow", 1, kMaxRegrowTicks, "durée de repousse",
                                    resource.regrowTicks)) {
            return error;
        }
        if (auto error = readNumber("abundance", 0, kMaxAbundance, "abondance", abundance)) {
            return error;
        }
        resource.hits = static_cast<std::uint16_t>(hits);
        resource.abundance = static_cast<std::uint16_t>(abundance);
        return std::nullopt;
    }

    /// Lignes `tool = identifiant` : outils connus, de catégorie `tool`, sans doublon.
    std::optional<DataError> readTools(ResourceDef& resource) const {
        for (const DataField& field : _block.fields) {
            if (field.key != "tool") {
                continue;
            }
            const auto tool = _items.findItem(field.value);
            if (!tool) {
                return fail(field,
                            "objet inconnu " + quoted(field.value) + " (absent de items.txt)");
            }
            if (_items.item(*tool)->category != ItemCategory::Tool) {
                return fail(field, "l'objet " + quoted(field.value) + " n'est pas un outil");
            }
            if (std::find(resource.tools.begin(), resource.tools.end(), *tool) !=
                resource.tools.end()) {
                return fail(field, "outil " + quoted(field.value) + " répété");
            }
            if (resource.tools.size() >= kMaxResourceTools) {
                return fail(field,
                            "trop d'outils (maximum " + std::to_string(kMaxResourceTools) + ")");
            }
            resource.tools.push_back(*tool);
        }
        return std::nullopt;
    }

    /// `hand = yes | no` (facultatif, oui par défaut) ; « no » n'a de sens qu'avec un outil.
    std::optional<DataError> readHand(ResourceDef& resource) const {
        const DataField* field = findField(_block, "hand");
        if (field == nullptr) {
            return std::nullopt;
        }
        if (field->value != "yes" && field->value != "no") {
            return fail(*field, "valeur " + quoted(field->value) + " invalide : yes ou no attendu");
        }
        resource.handAllowed = field->value == "yes";
        if (!resource.handAllowed && resource.tools.empty()) {
            return fail(*field, "« hand = no » exige au moins une ligne « tool »");
        }
        return std::nullopt;
    }

    std::optional<DataError> readNumber(std::string_view key, std::uint32_t min, std::uint32_t max,
                                        std::string_view what, std::uint32_t& out) const {
        const DataField& field = *findField(_block, key);
        const auto value = parseNumber(field.value, min, max);
        if (!value) {
            return fail(field, std::string(what) + " " + quoted(field.value) +
                                   " invalide : entier de " + std::to_string(min) + " à " +
                                   std::to_string(max) + " attendu");
        }
        out = *value;
        return std::nullopt;
    }

    DataError fail(const DataField& field, std::string message) const {
        return makeError(_file, field.line, std::move(message));
    }

    const DataBlock& _block;
    const Catalog& _items;
    std::string_view _file;
};

/// En-tête et clés du bloc ; erreur, ou nullopt si on peut le lire.
std::optional<DataError> checkResourceBlock(const DataBlock& block, const ResourceKeys& known,
                                            std::string_view file) {
    if (block.kind != "resource") {
        return makeError(file, block.line,
                         "type de bloc " + quoted(block.kind) +
                             " inattendu : « resource » attendu");
    }
    if (!isValidKey(block.id)) {
        return makeError(file, block.line,
                         "identifiant " + quoted(block.id) +
                             " invalide (minuscules, chiffres et « _ », ex. « fiber_bush »)");
    }
    if (known.contains(block.id)) {
        return makeError(file, block.line, "ressource " + quoted(block.id) + " déjà définie");
    }
    if (known.size() >= kMaxResourceTypes) {
        return makeError(file, block.line,
                         "trop de types de ressources (maximum " +
                             std::to_string(kMaxResourceTypes) + ")");
    }
    return checkBlockKeys(block, {kAllowedKeys, kRepeatableKeys, kRequiredKeys}, file);
}

} // namespace

ResourcesRead readResources(const std::vector<DataBlock>& blocks, const Catalog& items,
                            std::string_view file) {
    ResourcesRead result;
    ResourceKeys known;
    for (const DataBlock& block : blocks) {
        if (auto error = checkResourceBlock(block, known, file)) {
            result.error = std::move(error);
            return result;
        }

        ResourceDef resource;
        resource.id = ResourceTypeId{static_cast<std::uint8_t>(result.types.size())};
        resource.key = std::string(block.id);
        if (auto error = ResourceBlockReader(block, items, file).read(resource)) {
            result.error = std::move(error);
            return result;
        }
        known.insert(resource.key);
        result.types.push_back(std::move(resource));
    }
    return result;
}

} // namespace odyssee::data
