#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace odyssee::server::sqlite {

/// Enveloppe minimale de SQLite (T053), sans exceptions : chaque opération dit si elle a réussi,
/// et la raison d'un échec est écrite dans le journal.

class Statement;

/// Une base ouverte. Se ferme à la destruction.
class Database {
  public:
    /// Ouvre (ou crée) le fichier `path`. std::nullopt si c'est impossible.
    [[nodiscard]] static std::optional<Database> open(const std::string& path);

    /// Exécute une ou plusieurs requêtes sans résultat. Faux en cas d'erreur.
    bool exec(std::string_view sql);

    /// Prépare une requête. std::nullopt si elle est invalide.
    [[nodiscard]] std::optional<Statement> prepare(std::string_view sql);

  private:
    struct Close {
        void operator()(sqlite3* db) const;
    };
    explicit Database(sqlite3* db) : _db(db) {}

    std::unique_ptr<sqlite3, Close> _db;
};

/// Une requête préparée : on lie ses paramètres (numérotés à partir de 1), puis on avance ligne
/// par ligne avec step().
class Statement {
  public:
    void bind(int index, std::int64_t value);
    void bind(int index, std::string_view text);
    void bind(int index, std::span<const std::byte> blob);

    /// Avance d'une ligne : vrai s'il y en a une à lire. Faux à la fin, ou en cas d'erreur
    /// (alors failed() le dit).
    bool step();
    /// Exécute une requête sans résultat, puis la remet à zéro pour la suivante. Faux si erreur.
    bool run();
    [[nodiscard]] bool failed() const { return _failed; }

    [[nodiscard]] std::int64_t integer(int column) const;
    [[nodiscard]] std::string text(int column) const;
    [[nodiscard]] std::vector<std::byte> blob(int column) const;

  private:
    friend class Database;
    struct Finalize {
        void operator()(sqlite3_stmt* statement) const;
    };
    explicit Statement(sqlite3_stmt* statement) : _statement(statement) {}

    std::unique_ptr<sqlite3_stmt, Finalize> _statement;
    bool _failed = false;
};

} // namespace odyssee::server::sqlite
