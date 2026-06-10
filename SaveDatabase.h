#ifndef SAVEDATABASE_H
#define SAVEDATABASE_H

#include "GameSaveData.h"

#include <optional>
#include <string>

class SaveDatabase {
public:
    explicit SaveDatabase(std::string path);
    ~SaveDatabase();

    SaveDatabase(const SaveDatabase&) = delete;
    SaveDatabase& operator=(const SaveDatabase&) = delete;

    bool isOpen() const;
    std::string lastError() const;

    bool saveSlot(int slot, const GameSnapshot& snapshot, const std::string& label);
    std::optional<GameSnapshot> loadSlot(int slot);
    bool deleteSlot(int slot);
    bool hasSlot(int slot) const;
    std::string slotLabel(int slot) const;

    int getMetaLoopCount() const;
    void setMetaLoopCount(int loopCount);

private:
    bool ensureSchema();

    std::string path_;
    void* db_ = nullptr;
    std::string lastError_;
};

#endif
