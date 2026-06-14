#include "SaveDatabase.h"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace {

std::string nowIso8601() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &t);
#else
    localtime_r(&t, &localTime);
#endif
    std::ostringstream out;
    out << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

} // namespace

SaveDatabase::SaveDatabase(std::string path) : path_(std::move(path)) {
    if (sqlite3_open(path_.c_str(), reinterpret_cast<sqlite3**>(&db_)) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_));
        if (db_) {
            sqlite3_close(reinterpret_cast<sqlite3*>(db_));
            db_ = nullptr;
        }
        return;
    }
    ensureSchema();
}

SaveDatabase::~SaveDatabase() {
    if (db_) {
        sqlite3_close(reinterpret_cast<sqlite3*>(db_));
        db_ = nullptr;
    }
}

bool SaveDatabase::isOpen() const {
    return db_ != nullptr;
}

std::string SaveDatabase::lastError() const {
    return lastError_;
}

bool SaveDatabase::ensureSchema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS meta ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS save_slots ("
        "  slot INTEGER PRIMARY KEY,"
        "  label TEXT NOT NULL,"
        "  saved_at TEXT NOT NULL,"
        "  snapshot TEXT NOT NULL"
        ");";

    char* err = nullptr;
    if (sqlite3_exec(reinterpret_cast<sqlite3*>(db_), sql, nullptr, nullptr, &err) != SQLITE_OK) {
        lastError_ = err ? err : "schema error";
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool SaveDatabase::saveSlot(int slot, const GameSnapshot& snapshot, const std::string& label) {
    if (!db_ || slot < 1 || slot > 3) {
        return false;
    }

    std::string trimmedLabel = label;
    if (trimmedLabel.size() > kMaxSaveLabelLength) {
        trimmedLabel.resize(kMaxSaveLabelLength);
    }

    const std::string blob = serializeSnapshot(snapshot);
    const std::string savedAt = nowIso8601();

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO save_slots(slot, label, saved_at, snapshot) VALUES(?, ?, ?, ?) "
        "ON CONFLICT(slot) DO UPDATE SET label=excluded.label, saved_at=excluded.saved_at, snapshot=excluded.snapshot;";

    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_));
        return false;
    }

    sqlite3_bind_int(stmt, 1, slot);
    sqlite3_bind_text(stmt, 2, trimmedLabel.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, savedAt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, blob.c_str(), -1, SQLITE_TRANSIENT);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (!ok) {
        lastError_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_));
    }
    sqlite3_finalize(stmt);
    return ok;
}

std::optional<GameSnapshot> SaveDatabase::loadSlot(int slot) {
    if (!db_ || slot < 1 || slot > 3) {
        return std::nullopt;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT snapshot FROM save_slots WHERE slot = ?;";
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_));
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, slot);
    std::optional<GameSnapshot> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* blob = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (blob) {
            GameSnapshot snapshot;
            if (deserializeSnapshot(blob, snapshot)) {
                result = snapshot;
            }
        }
    }
    sqlite3_finalize(stmt);
    return result;
}

bool SaveDatabase::deleteSlot(int slot) {
    if (!db_ || slot < 1 || slot > 3) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM save_slots WHERE slot = ?;";
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_));
        return false;
    }

    sqlite3_bind_int(stmt, 1, slot);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool SaveDatabase::hasSlot(int slot) const {
    if (!db_ || slot < 1 || slot > 3) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT 1 FROM save_slots WHERE slot = ? LIMIT 1;";
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, slot);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

std::string SaveDatabase::slotLabel(int slot) const {
    if (!db_ || slot < 1 || slot > 3) {
        return "Empty";
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT label FROM save_slots WHERE slot = ?;";
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "Empty";
    }

    sqlite3_bind_int(stmt, 1, slot);
    std::string label = "Empty";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (name) {
            label = name;
            if (label.size() > kMaxSaveLabelLength) {
                label.resize(kMaxSaveLabelLength);
            }
        }
    }
    sqlite3_finalize(stmt);
    return label;
}

int SaveDatabase::getMetaLoopCount() const {
    if (!db_) {
        return 1;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT value FROM meta WHERE key = 'loop_count';";
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 1;
    }

    int loopCount = 1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (value) {
            loopCount = std::max(1, std::stoi(value));
        }
    }
    sqlite3_finalize(stmt);
    return loopCount;
}

void SaveDatabase::setMetaLoopCount(int loopCount) {
    if (!db_) {
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO meta(key, value) VALUES('loop_count', ?) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value;";

    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(reinterpret_cast<sqlite3*>(db_));
        return;
    }

    const std::string value = std::to_string(std::max(1, loopCount));
    sqlite3_bind_text(stmt, 1, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
