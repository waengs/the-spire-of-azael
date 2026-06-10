#include "GameSaveData.h"

#include <sstream>

namespace {

void writeBool(std::ostringstream& out, const char* key, bool value) {
    out << key << '=' << (value ? '1' : '0') << '\n';
}

bool readBool(const std::string& line, const char* key, bool& value) {
    const std::string prefix = std::string(key) + '=';
    if (line.rfind(prefix, 0) != 0) {
        return false;
    }
    value = line[prefix.size()] == '1';
    return true;
}

bool readInt(const std::string& line, const char* key, int& value) {
    const std::string prefix = std::string(key) + '=';
    if (line.rfind(prefix, 0) != 0) {
        return false;
    }
    value = std::stoi(line.substr(prefix.size()));
    return true;
}

bool readString(const std::string& line, const char* key, std::string& value) {
    const std::string prefix = std::string(key) + '=';
    if (line.rfind(prefix, 0) != 0) {
        return false;
    }
    value = line.substr(prefix.size());
    return true;
}

} // namespace

std::string serializeSnapshot(const GameSnapshot& snapshot) {
    std::ostringstream out;
    out << "loopCount=" << snapshot.loopCount << '\n';
    out << "roomName=" << snapshot.roomName << '\n';
    out << "health=" << snapshot.health << '\n';
    out << "maxHealth=" << snapshot.maxHealth << '\n';
    out << "gold=" << snapshot.gold << '\n';
    out << "baseAttack=" << snapshot.baseAttack << '\n';
    out << "baseDefense=" << snapshot.baseDefense << '\n';
    out << "equippedWeapon=" << snapshot.equippedWeapon << '\n';
    out << "equippedArmor=" << snapshot.equippedArmor << '\n';

    out << "inventory=";
    for (size_t i = 0; i < snapshot.inventory.size(); ++i) {
        if (i > 0) {
            out << '|';
        }
        out << snapshot.inventory[i];
    }
    out << '\n';

    out << "party=";
    for (size_t i = 0; i < snapshot.party.size(); ++i) {
        if (i > 0) {
            out << '|';
        }
        out << snapshot.party[i];
    }
    out << '\n';

    writeBool(out, "knowsGarrisonStory", snapshot.knowsGarrisonStory);
    writeBool(out, "examinedGarrisonArmor", snapshot.examinedGarrisonArmor);
    out << "eleanorTalkCount=" << snapshot.eleanorTalkCount << '\n';
    writeBool(out, "eleanorHeardAllDialogue", snapshot.eleanorHeardAllDialogue);
    writeBool(out, "eleanorRingGifted", snapshot.eleanorRingGifted);
    writeBool(out, "examinedFountain", snapshot.examinedFountain);
    writeBool(out, "lyraRiddleSolved", snapshot.lyraRiddleSolved);
    writeBool(out, "pipBreadGiven", snapshot.pipBreadGiven);
    writeBool(out, "pipPraised", snapshot.pipPraised);
    writeBool(out, "pipGenuineBond", snapshot.pipGenuineBond);
    writeBool(out, "pipBribed", snapshot.pipBribed);
    writeBool(out, "pipReadyToRecruit", snapshot.pipReadyToRecruit);
    writeBool(out, "pipRevealedBySearch", snapshot.pipRevealedBySearch);
    writeBool(out, "lyraRecruited", snapshot.lyraRecruited);
    writeBool(out, "bramRecruited", snapshot.bramRecruited);
    writeBool(out, "bramMasterwork", snapshot.bramMasterwork);
    writeBool(out, "bramReadyToRecruit", snapshot.bramReadyToRecruit);
    writeBool(out, "garrisonCharmed", snapshot.garrisonCharmed);
    writeBool(out, "valerieCharmed", snapshot.valerieCharmed);
    writeBool(out, "valerieReadyToRecruit", snapshot.valerieReadyToRecruit);
    writeBool(out, "valerieRecruited", snapshot.valerieRecruited);
    writeBool(out, "spirePacifist", snapshot.spirePacifist);
    writeBool(out, "lyraDisruptUsed", snapshot.lyraDisruptUsed);
    writeBool(out, "specialGiveUnlocked", snapshot.specialGiveUnlocked);
    writeBool(out, "specialPraiseUnlocked", snapshot.specialPraiseUnlocked);
    writeBool(out, "specialExamineUnlocked", snapshot.specialExamineUnlocked);
    writeBool(out, "specialRecruitUnlocked", snapshot.specialRecruitUnlocked);
    writeBool(out, "bramSavedGoblin", snapshot.bramSavedGoblin);

    return out.str();
}

bool deserializeSnapshot(const std::string& blob, GameSnapshot& out) {
    GameSnapshot snapshot;
    std::istringstream input(blob);
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        readInt(line, "loopCount", snapshot.loopCount);
        readString(line, "roomName", snapshot.roomName);
        readInt(line, "health", snapshot.health);
        readInt(line, "maxHealth", snapshot.maxHealth);
        readInt(line, "gold", snapshot.gold);
        readInt(line, "baseAttack", snapshot.baseAttack);
        readInt(line, "baseDefense", snapshot.baseDefense);
        readString(line, "equippedWeapon", snapshot.equippedWeapon);
        readString(line, "equippedArmor", snapshot.equippedArmor);

        if (line.rfind("inventory=", 0) == 0) {
            snapshot.inventory.clear();
            std::string payload = line.substr(10);
            std::stringstream items(payload);
            std::string item;
            while (std::getline(items, item, '|')) {
                if (!item.empty()) {
                    snapshot.inventory.push_back(item);
                }
            }
        }

        if (line.rfind("party=", 0) == 0) {
            snapshot.party.clear();
            std::string payload = line.substr(6);
            std::stringstream members(payload);
            std::string member;
            while (std::getline(members, member, '|')) {
                if (!member.empty()) {
                    snapshot.party.push_back(member);
                }
            }
        }

        readBool(line, "knowsGarrisonStory", snapshot.knowsGarrisonStory);
        readBool(line, "examinedGarrisonArmor", snapshot.examinedGarrisonArmor);
        readInt(line, "eleanorTalkCount", snapshot.eleanorTalkCount);
        readBool(line, "eleanorHeardAllDialogue", snapshot.eleanorHeardAllDialogue);
        readBool(line, "eleanorRingGifted", snapshot.eleanorRingGifted);
        readBool(line, "examinedFountain", snapshot.examinedFountain);
        readBool(line, "lyraRiddleSolved", snapshot.lyraRiddleSolved);
        readBool(line, "pipBreadGiven", snapshot.pipBreadGiven);
        readBool(line, "pipPraised", snapshot.pipPraised);
        readBool(line, "pipGenuineBond", snapshot.pipGenuineBond);
        readBool(line, "pipBribed", snapshot.pipBribed);
        readBool(line, "pipReadyToRecruit", snapshot.pipReadyToRecruit);
        readBool(line, "pipRevealedBySearch", snapshot.pipRevealedBySearch);
        readBool(line, "lyraRecruited", snapshot.lyraRecruited);
        readBool(line, "bramRecruited", snapshot.bramRecruited);
        readBool(line, "bramMasterwork", snapshot.bramMasterwork);
        readBool(line, "bramReadyToRecruit", snapshot.bramReadyToRecruit);
        readBool(line, "garrisonCharmed", snapshot.garrisonCharmed);
        readBool(line, "valerieCharmed", snapshot.valerieCharmed);
        readBool(line, "valerieReadyToRecruit", snapshot.valerieReadyToRecruit);
        readBool(line, "valerieRecruited", snapshot.valerieRecruited);
        readBool(line, "spirePacifist", snapshot.spirePacifist);
        readBool(line, "lyraDisruptUsed", snapshot.lyraDisruptUsed);
        readBool(line, "specialGiveUnlocked", snapshot.specialGiveUnlocked);
        readBool(line, "specialPraiseUnlocked", snapshot.specialPraiseUnlocked);
        readBool(line, "specialExamineUnlocked", snapshot.specialExamineUnlocked);
        readBool(line, "specialRecruitUnlocked", snapshot.specialRecruitUnlocked);
        readBool(line, "bramSavedGoblin", snapshot.bramSavedGoblin);
    }

    if (snapshot.roomName.empty()) {
        return false;
    }

    out = std::move(snapshot);
    return true;
}
