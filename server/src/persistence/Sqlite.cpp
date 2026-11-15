#include "persistence/Sqlite.hpp"

#include "odyssee/core/Log.hpp"

#include <sqlite3.h>

#include <limits>

namespace odyssee::server::sqlite {

namespace {

constexpr log::Logger kLog{"sauvegarde"};

int lengthOf(std::size_t size) {
    return size > static_cast<std::size_t>(std::numeric_limits<int>::max())
               ? std::numeric_limits<int>::max()
               : static_cast<int>(size);
}

} // namespace

void Database::Close::operator()(sqlite3* db) const {
    sqlite3_close(db);
}

void Statement::Finalize::operator()(sqlite3_stmt* statement) const {
    sqlite3_finalize(statement);
}

std::optional<Database> Database::open(const std::string& path) {
    sqlite3* db = nullptr;
    const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    if (sqlite3_open_v2(path.c_str(), &db, flags, nullptr) != SQLITE_OK) {
        kLog.error("impossible d'ouvrir « {} » : {}", path, db ? sqlite3_errmsg(db) : "mémoire");
        sqlite3_close(db);
        return std::nullopt;
    }
    return Database(db);
}

bool Database::exec(std::string_view sql) {
    const std::string text(sql);
    char* error = nullptr;
    if (sqlite3_exec(_db.get(), text.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        kLog.error("requête refusée : {}", error ? error : "?");
        sqlite3_free(error);
        return false;
    }
    return true;
}

std::optional<Statement> Database::prepare(std::string_view sql) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(_db.get(), sql.data(), lengthOf(sql.size()), &statement, nullptr) !=
        SQLITE_OK) {
        kLog.error("requête invalide : {}", sqlite3_errmsg(_db.get()));
        return std::nullopt;
    }
    return Statement(statement);
}

void Statement::bind(int index, std::int64_t value) {
    sqlite3_bind_int64(_statement.get(), index, value);
}

void Statement::bind(int index, std::string_view text) {
    sqlite3_bind_text(_statement.get(), index, text.data(), lengthOf(text.size()),
                      SQLITE_TRANSIENT);
}

void Statement::bind(int index, std::span<const std::byte> blob) {
    sqlite3_bind_blob(_statement.get(), index, blob.data(), lengthOf(blob.size()),
                      SQLITE_TRANSIENT);
}

bool Statement::step() {
    const int result = sqlite3_step(_statement.get());
    if (result == SQLITE_ROW) {
        return true;
    }
    if (result != SQLITE_DONE) {
        _failed = true;
        kLog.error("lecture ou écriture impossible : {}",
                   sqlite3_errmsg(sqlite3_db_handle(_statement.get())));
    }
    return false;
}

bool Statement::run() {
    _failed = false;
    (void)step();
    sqlite3_reset(_statement.get());
    sqlite3_clear_bindings(_statement.get());
    return !_failed;
}

std::int64_t Statement::integer(int column) const {
    return sqlite3_column_int64(_statement.get(), column);
}

std::string Statement::text(int column) const {
    const unsigned char* text = sqlite3_column_text(_statement.get(), column);
    const int size = sqlite3_column_bytes(_statement.get(), column);
    return text ? std::string(reinterpret_cast<const char*>(text), static_cast<std::size_t>(size))
                : std::string();
}

std::vector<std::byte> Statement::blob(int column) const {
    const auto* data = static_cast<const std::byte*>(sqlite3_column_blob(_statement.get(), column));
    const int size = sqlite3_column_bytes(_statement.get(), column);
    return data ? std::vector<std::byte>(data, data + size) : std::vector<std::byte>();
}

} // namespace odyssee::server::sqlite
