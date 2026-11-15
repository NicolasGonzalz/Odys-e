#pragma once

#include "persistence/Sqlite.hpp"
#include "persistence/WorldSave.hpp"

#include <optional>
#include <string>

namespace odyssee::server {

/// La sauvegarde du monde dans un fichier SQLite (T053) : une base par monde, même format en
/// solo et sur le homelab (docs/MODES.md).
///
/// Chaque écriture remplace toute la sauvegarde précédente, en une transaction : un arrêt
/// brutal en pleine écriture laisse l'ancienne sauvegarde intacte.
class SaveStore {
  public:
    /// Ouvre (ou crée) la base `path` et prépare ses tables. std::nullopt si c'est impossible,
    /// ou si elle vient d'une version plus récente du serveur.
    [[nodiscard]] static std::optional<SaveStore> open(const std::string& path);

    /// La sauvegarde enregistrée. Une base neuve donne une sauvegarde vide.
    /// std::nullopt si la lecture échoue.
    [[nodiscard]] std::optional<WorldSave> load();

    /// Remplace la sauvegarde par `save`. Faux si l'écriture échoue (l'ancienne reste).
    bool store(const WorldSave& save);

  private:
    explicit SaveStore(sqlite::Database db) : _db(std::move(db)) {}

    bool loadPlayers(WorldSave& save);
    bool loadResources(WorldSave& save);
    bool loadBags(WorldSave& save);
    bool storePlayers(const WorldSave& save);
    bool storeResources(const WorldSave& save);
    bool storeBags(const WorldSave& save);
    bool storeMeta(const WorldSave& save);

    sqlite::Database _db;
};

} // namespace odyssee::server
