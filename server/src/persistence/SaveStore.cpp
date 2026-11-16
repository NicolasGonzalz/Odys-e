#include "persistence/SaveStore.hpp"

#include "odyssee/core/Log.hpp"
#include "odyssee/net/ByteBuffer.hpp"

#include <algorithm>
#include <limits>

namespace odyssee::server {

namespace {

constexpr log::Logger kLog{"sauvegarde"};

/// Version du format de la base. Incrémentée à chaque changement de tables ; un serveur refuse
/// une base plus récente que lui (il ne saurait pas la relire sans rien perdre).
/// 2 : type de chaque ressource (une ressource sauvegardée n'est reprise que si son type n'a pas
/// changé depuis : data/resources.txt modifié, le tirage ne donne plus les mêmes ressources).
constexpr std::int64_t kSchemaVersion = 2;

constexpr std::string_view kResourcesTable = R"(
CREATE TABLE IF NOT EXISTS resources(
    rank INTEGER PRIMARY KEY, type INTEGER NOT NULL, hits INTEGER NOT NULL,
    regrow INTEGER NOT NULL);
)";

constexpr std::string_view kSchema = R"(
CREATE TABLE IF NOT EXISTS meta(key TEXT PRIMARY KEY, value INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS players(
    name TEXT PRIMARY KEY, x INTEGER NOT NULL, y INTEGER NOT NULL, z INTEGER NOT NULL,
    yaw INTEGER NOT NULL, health INTEGER NOT NULL, hunger INTEGER NOT NULL,
    thirst INTEGER NOT NULL, rest_x INTEGER NOT NULL, rest_y INTEGER NOT NULL,
    rest_z INTEGER NOT NULL, rest_yaw INTEGER NOT NULL, inventory BLOB NOT NULL);
CREATE TABLE IF NOT EXISTS bags(
    rank INTEGER PRIMARY KEY, x INTEGER NOT NULL, y INTEGER NOT NULL, z INTEGER NOT NULL,
    owner INTEGER NOT NULL, contents BLOB NOT NULL);
)";

/// Ramène une valeur lue dans la base dans les bornes de son type (base abîmée ou modifiée).
template <typename T> T clamped(std::int64_t value) {
    return static_cast<T>(std::clamp<std::int64_t>(value, std::numeric_limits<T>::min(),
                                                   std::numeric_limits<T>::max()));
}

std::int32_t vital(std::int64_t value) {
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(value, 0, VitalsTuning::kVitalScale));
}

std::vector<std::byte> encodeSlots(const DroppedSlots& slots) {
    net::ByteWriter w;
    for (const net::InventorySlot& slot : slots) {
        w.writeU16(slot.item);
        w.writeU16(slot.count);
    }
    return {w.data().begin(), w.data().end()};
}

/// Cases relues ; une case incohérente (objet sans quantité) est vidée.
DroppedSlots decodeSlots(std::span<const std::byte> bytes) {
    DroppedSlots slots{};
    net::ByteReader r(bytes);
    for (net::InventorySlot& slot : slots) {
        const auto item = r.readU16();
        const auto count = r.readU16();
        if (item && count && *count != 0) {
            slot = {*item, *count};
        }
    }
    return slots;
}

std::array<std::int32_t, 3> positionAt(const sqlite::Statement& row, int first) {
    return {clamped<std::int32_t>(row.integer(first)),
            clamped<std::int32_t>(row.integer(first + 1)),
            clamped<std::int32_t>(row.integer(first + 2))};
}

void bindPosition(sqlite::Statement& statement, int first, const std::array<std::int32_t, 3>& p) {
    for (int i = 0; i < 3; ++i) {
        statement.bind(first + i, std::int64_t{p[static_cast<std::size_t>(i)]});
    }
}

/// Met une base d'un format plus ancien au format actuel. Format 1 : les ressources n'avaient pas
/// leur type ; leur état est oublié (elles repartent pleines), le reste est gardé.
bool upgrade(sqlite::Database& db, std::int64_t version) {
    if (version == 1 && !db.exec("DROP TABLE resources")) {
        return false;
    }
    return db.exec(kResourcesTable);
}

std::optional<std::int64_t> schemaVersionOf(sqlite::Database& db) {
    auto query = db.prepare("SELECT value FROM meta WHERE key = 'schema_version'");
    if (!query) {
        return std::nullopt;
    }
    return query->step() ? query->integer(0) : 0;
}

} // namespace

std::optional<SaveStore> SaveStore::open(const std::string& path) {
    auto db = sqlite::Database::open(path);
    if (!db || !db->exec(kSchema)) {
        return std::nullopt;
    }
    const auto version = schemaVersionOf(*db);
    if (!version || *version > kSchemaVersion) {
        kLog.error("« {} » vient d'une version plus récente du serveur (format {}, connu : {})",
                   path, version.value_or(-1), kSchemaVersion);
        return std::nullopt;
    }
    if (!upgrade(*db, *version)) {
        return std::nullopt;
    }
    return SaveStore(std::move(*db));
}

std::optional<WorldSave> SaveStore::load() {
    WorldSave save;
    auto meta = _db.prepare("SELECT value FROM meta WHERE key = 'world_fingerprint'");
    if (!meta) {
        return std::nullopt;
    }
    if (meta->step()) {
        save.worldFingerprint = static_cast<std::uint64_t>(meta->integer(0));
    }
    if (!loadPlayers(save) || !loadResources(save) || !loadBags(save)) {
        return std::nullopt;
    }
    return save;
}

bool SaveStore::loadPlayers(WorldSave& save) {
    auto rows = _db.prepare("SELECT name, x, y, z, yaw, health, hunger, thirst, rest_x, rest_y, "
                            "rest_z, rest_yaw, inventory FROM players ORDER BY name");
    if (!rows) {
        return false;
    }
    while (rows->step()) {
        SavedPlayer player;
        player.name = rows->text(0);
        player.position = positionAt(*rows, 1);
        player.yaw = clamped<std::uint16_t>(rows->integer(4));
        player.vitals = {vital(rows->integer(5)), vital(rows->integer(6)), vital(rows->integer(7))};
        player.rest = {positionAt(*rows, 8), clamped<std::uint16_t>(rows->integer(11))};
        player.inventory = decodeSlots(rows->blob(12));
        save.players.push_back(std::move(player));
    }
    return !rows->failed();
}

bool SaveStore::loadResources(WorldSave& save) {
    auto rows = _db.prepare("SELECT type, hits, regrow FROM resources ORDER BY rank");
    if (!rows) {
        return false;
    }
    while (rows->step()) {
        save.resources.push_back({clamped<std::uint8_t>(rows->integer(0)),
                                  clamped<std::uint16_t>(rows->integer(1)),
                                  clamped<std::uint32_t>(rows->integer(2))});
    }
    return !rows->failed();
}

bool SaveStore::loadBags(WorldSave& save) {
    auto rows = _db.prepare("SELECT x, y, z, owner, contents FROM bags ORDER BY rank");
    if (!rows) {
        return false;
    }
    while (rows->step()) {
        SavedBag bag;
        bag.position = positionAt(*rows, 0);
        bag.contents.owner = clamped<net::PlayerId>(rows->integer(3));
        bag.contents.slots = decodeSlots(rows->blob(4));
        save.bags.push_back(bag);
    }
    return !rows->failed();
}

bool SaveStore::store(const WorldSave& save) {
    if (!_db.exec("BEGIN IMMEDIATE")) {
        return false;
    }
    const bool written = _db.exec("DELETE FROM players; DELETE FROM resources; DELETE FROM bags") &&
                         storePlayers(save) && storeResources(save) && storeBags(save) &&
                         storeMeta(save);
    if (!written) {
        (void)_db.exec("ROLLBACK");
        return false;
    }
    return _db.exec("COMMIT");
}

bool SaveStore::storePlayers(const WorldSave& save) {
    auto insert = _db.prepare("INSERT INTO players VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    if (!insert) {
        return false;
    }
    for (const SavedPlayer& player : save.players) {
        insert->bind(1, player.name);
        bindPosition(*insert, 2, player.position);
        insert->bind(5, std::int64_t{player.yaw});
        insert->bind(6, std::int64_t{player.vitals.health});
        insert->bind(7, std::int64_t{player.vitals.hunger});
        insert->bind(8, std::int64_t{player.vitals.thirst});
        bindPosition(*insert, 9, player.rest.position);
        insert->bind(12, std::int64_t{player.rest.yaw});
        insert->bind(13, encodeSlots(player.inventory));
        if (!insert->run()) {
            return false;
        }
    }
    return true;
}

bool SaveStore::storeResources(const WorldSave& save) {
    auto insert = _db.prepare("INSERT INTO resources VALUES (?, ?, ?, ?)");
    if (!insert) {
        return false;
    }
    for (std::size_t rank = 0; rank < save.resources.size(); ++rank) {
        insert->bind(1, static_cast<std::int64_t>(rank));
        insert->bind(2, std::int64_t{save.resources[rank].type});
        insert->bind(3, std::int64_t{save.resources[rank].hitsLeft});
        insert->bind(4, std::int64_t{save.resources[rank].regrowTicksLeft});
        if (!insert->run()) {
            return false;
        }
    }
    return true;
}

bool SaveStore::storeBags(const WorldSave& save) {
    auto insert = _db.prepare("INSERT INTO bags VALUES (?, ?, ?, ?, ?, ?)");
    if (!insert) {
        return false;
    }
    for (std::size_t rank = 0; rank < save.bags.size(); ++rank) {
        const SavedBag& bag = save.bags[rank];
        insert->bind(1, static_cast<std::int64_t>(rank));
        bindPosition(*insert, 2, bag.position);
        insert->bind(5, std::int64_t{bag.contents.owner});
        insert->bind(6, encodeSlots(bag.contents.slots));
        if (!insert->run()) {
            return false;
        }
    }
    return true;
}

bool SaveStore::storeMeta(const WorldSave& save) {
    auto upsert = _db.prepare("INSERT OR REPLACE INTO meta VALUES (?, ?)");
    if (!upsert) {
        return false;
    }
    upsert->bind(1, std::string_view("schema_version"));
    upsert->bind(2, kSchemaVersion);
    if (!upsert->run()) {
        return false;
    }
    upsert->bind(1, std::string_view("world_fingerprint"));
    upsert->bind(2, static_cast<std::int64_t>(save.worldFingerprint));
    return upsert->run();
}

} // namespace odyssee::server
