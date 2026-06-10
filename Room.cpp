#include "Room.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <vector>

static std::string normalizeItemKey(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (unsigned char ch : text) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return out;
}

static std::string toLowerStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return s;
}

static std::string capitalizeDirection(const std::string& direction) {
    if (direction.empty()) {
        return direction;
    }
    std::string out = direction;
    out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
    return out;
}

static const char* directionColorAnsi(const std::string& direction) {
    const std::string dir = toLowerStr(direction);
    if (dir == "north") {
        return "\033[1;36m";
    }
    if (dir == "south") {
        return "\033[1;33m";
    }
    if (dir == "east") {
        return "\033[1;32m";
    }
    if (dir == "west") {
        return "\033[1;35m";
    }
    if (dir == "up") {
        return "\033[1;34m";
    }
    if (dir == "down") {
        return "\033[1;31m";
    }
    return "\033[1;37m";
}

static std::string trimCopy(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

// Collapse manual newlines/tabs into spaces so wrapping stays natural in the console.
static std::string normalizeDisplayText(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    bool lastWasSpace = true;
    for (unsigned char ch : text) {
        if (ch == '\r' || ch == '\n' || ch == '\t' || std::isspace(ch)) {
            if (!lastWasSpace && !out.empty()) {
                out.push_back(' ');
                lastWasSpace = true;
            }
        } else {
            out.push_back(static_cast<char>(ch));
            lastWasSpace = false;
        }
    }
    return trimCopy(out);
}

static std::vector<std::string> wrapDisplayLines(const std::string& text, size_t softMax = 72) {
    std::vector<std::string> lines;
    std::string rest = normalizeDisplayText(text);
    while (!rest.empty()) {
        if (rest.size() <= softMax) {
            lines.push_back(rest);
            break;
        }
        size_t space = rest.rfind(' ', softMax);
        if (space == std::string::npos || space < 16) {
            space = softMax;
        }
        lines.push_back(trimCopy(rest.substr(0, space)));
        rest = trimCopy(rest.substr(space));
    }
    return lines;
}

static std::vector<std::string> splitDisplayLines(const std::string& text, size_t softMax = 68) {
    std::vector<std::string> lines;
    std::string rest = normalizeDisplayText(text);
    if (rest.empty()) {
        return lines;
    }

    while (!rest.empty()) {
        std::string segment;
        size_t period = rest.find(". ");
        if (period != std::string::npos) {
            segment = rest.substr(0, period + 1);
            rest = trimCopy(rest.substr(period + 2));
        } else {
            segment = rest;
            rest.clear();
        }

        while (segment.size() > softMax) {
            size_t comma = segment.rfind(", ", softMax);
            if (comma != std::string::npos && comma >= 12) {
                lines.push_back(trimCopy(segment.substr(0, comma + 1)));
                segment = trimCopy(segment.substr(comma + 2));
                continue;
            }

            size_t space = segment.rfind(' ', softMax);
            if (space == std::string::npos || space < 10) {
                space = softMax;
            }
            lines.push_back(trimCopy(segment.substr(0, space)));
            segment = trimCopy(segment.substr(space));
        }

        if (!segment.empty()) {
            lines.push_back(segment);
        }
    }

    return lines;
}

static void appendHighlightMarkers(std::stringstream& ss, const std::string& text) {
    size_t pos = 0;
    while (pos < text.size()) {
        size_t start = text.find('!', pos);
        if (start == std::string::npos) {
            ss << text.substr(pos);
            break;
        }
        ss << text.substr(pos, start - pos);
        size_t end = text.find('!', start + 1);
        if (end == std::string::npos) {
            ss << text.substr(start);
            break;
        }
        ss << "\033[1;35m" << text.substr(start + 1, end - start - 1) << "\033[0m";
        pos = end + 1;
    }
}

static std::vector<std::string> splitParagraphs(const std::string& text) {
    std::vector<std::string> paragraphs;
    size_t start = 0;
    while (start <= text.size()) {
        size_t sep = text.find("||", start);
        std::string chunk = trimCopy(text.substr(start, sep == std::string::npos ? std::string::npos : sep - start));
        if (!chunk.empty()) {
            paragraphs.push_back(chunk);
        }
        if (sep == std::string::npos) {
            break;
        }
        start = sep + 2;
    }
    return paragraphs;
}

static void appendHighlightedIndentedBlock(std::stringstream& ss, const std::string& text, const std::string& indent) {
    const std::vector<std::string> paragraphs = splitParagraphs(text);
    for (size_t p = 0; p < paragraphs.size(); ++p) {
        if (p > 0) {
            ss << "\n";
        }
        for (const std::string& line : wrapDisplayLines(paragraphs[p])) {
            ss << indent;
            appendHighlightMarkers(ss, line);
            ss << "\n";
        }
    }
}

static void appendColoredIndentedBlock(
    std::stringstream& ss,
    const std::string& text,
    const std::string& indent,
    const char* colorOn,
    const char* colorOff
) {
    for (const std::string& line : splitDisplayLines(text)) {
        ss << indent << colorOn << line << colorOff << "\n";
    }
}

Room::Room(std::string name, std::string description)
    : name(name), description(description), enemy(nullptr), isLocked(false), keyRequired(""),
      lockMessage(""), hideExitsWhileGuard(false), hasPendingLoot(false), pendingLoot(Item()) {}

Room::~Room() {
    if (enemy) delete enemy;
    for (NPC* npc : npcs) {
        delete npc;
    }
}

std::string Room::getName() const { return name; }
std::string Room::getDescription() const { return description; }
void Room::setDescription(std::string newDescription) { description = std::move(newDescription); }

std::string Room::getDetailedDescription() const {
    std::stringstream ss;

    ss << "\033[1;36m[" << name << "]\033[0m\n\n";
    appendHighlightedIndentedBlock(ss, description, "  ");

    if (enemy && enemy->isAlive()) {
        ss << "\n";
    }
    if (enemy && enemy->isAlive()) {
        ss << "  \033[1;31m!\033[0m\n";
        if (enemy->getName() == "Giant Cave Bat") {
            appendColoredIndentedBlock(ss,
                "A Giant Cave Bat swoops down from the darkness!", "      ", "\033[1;31m", "\033[0m");
            std::string threat = "Giant Cave Bat (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Goblin Scout") {
            appendColoredIndentedBlock(ss,
                "Goblin Scout (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) turns toward you.",
                "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss, "The goblin snarls.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "\"Another one? Fine. We'll take both of you down!\"", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "The Goblin Scout blocks your path!", "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Armored Knight" || enemy->getName() == "Sir Garrison") {
            appendColoredIndentedBlock(ss,
                "An Armored Knight stands at the center of the hall.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "His armor is ancient, layered with scratches and old battle marks.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss, "He does not move aside.", "      ", "\033[1;31m", "\033[0m");
            std::string threat = enemy->getName() + " (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "Every exit is sealed while he stands.", "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Demon Hound") {
            appendColoredIndentedBlock(ss,
                "A Demon Hound steps into view.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "Its body is too still for something alive.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "Its eyes track you without blinking.", "      ", "\033[1;31m", "\033[0m");
            std::string threat = "Demon Hound (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "The corridor seals behind you.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "There is no retreat.", "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Cave Troll") {
            appendColoredIndentedBlock(ss,
                "A massive Cave Troll stands before the altar.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "It turns slowly toward you, its footsteps shaking the ground.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss, "\"Intruder...\"", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "The creature raises its weapon.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "\"You will not take what sleeps below.\"", "      ", "\033[1;31m", "\033[0m");
            std::string threat = "Cave Troll (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Demon General Malakor") {
            appendColoredIndentedBlock(ss,
                "Demon General Malakor stands among the crimson runes.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "His armor drinks the torchlight. He does not yield the stair.", "      ", "\033[1;31m", "\033[0m");
            std::string threat = "Demon General Malakor (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Demon Lord Azael") {
            std::string threat = "Demon Lord Azael (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP)";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "He does not rise.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "He does not need to.", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "\"...So you arrived.\"", "      ", "\033[1;31m", "\033[0m");
            appendColoredIndentedBlock(ss,
                "The air tightens. Every exit is sealed.", "      ", "\033[1;31m", "\033[0m");
        } else if (enemy->getName() == "Valerie") {
            appendColoredIndentedBlock(ss,
                "Valerie spreads her wings; wind screams across the ledge.", "      ", "\033[1;31m", "\033[0m");
            std::string threat = "Valerie (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
        } else {
            std::string threat = enemy->getName() + " (" + std::to_string(enemy->getHealth()) + "/"
                + std::to_string(enemy->getMaxHealth()) + " HP) blocks your path!";
            appendColoredIndentedBlock(ss, threat, "      ", "\033[1;31m", "\033[0m");
        }
    }

    if (hasPendingLoot) {
        ss << "\n\033[1;33mUnclaimed loot\033[0m\n";
        ss << "  *\n";
        ss << "      Something lies among the fallen foe. Use \033[1;32mpickup loot\033[0m.\n";
    }

    bool anyDiscovered = false;
    for (const auto& hi : hiddenItems) {
        if (hi.discovered) {
            if (!anyDiscovered) {
                ss << "\n\033[1;33mItems here\033[0m\n";
                anyDiscovered = true;
            }
            ss << "  \033[1;33m*\033[0m\n";
            appendColoredIndentedBlock(ss, hi.descriptor, "      ", "\033[1;33m", "\033[0m");
        }
    }

    if (hideExitsWhileGuard && enemy && enemy->isAlive()) {
        ss << "\n\033[1;31mEvery exit is sealed while the guard still stands.\033[0m\n";
    } else {
    ss << "\n\033[1;33mExits\033[0m\n";
    if (exits.empty()) {
        ss << "  (none)\n";
    } else {
        for (const auto& pair : exits) {
            std::string label = capitalizeDirection(pair.first);
            std::string blurb;
            auto blurbIt = exitBlurbs.find(pair.first);
            if (blurbIt != exitBlurbs.end()) {
                blurb = blurbIt->second;
            } else if (pair.second) {
                blurb = pair.second->getName();
            } else {
                blurb = "unknown";
            }

            ss << "  " << directionColorAnsi(pair.first) << "* " << label << "\033[0m\n";
            appendColoredIndentedBlock(ss, blurb, "      ", "\033[0;37m", "\033[0m");
        }
    }
    }

    return ss.str();
}

void Room::setHideExitsWhileGuard(bool hide) {
    hideExitsWhileGuard = hide;
}

void Room::setPendingLoot(Item item) {
    pendingLoot = item;
    hasPendingLoot = true;
}

bool Room::hasUnclaimedLoot() const {
    return hasPendingLoot;
}

bool Room::claimPendingLoot(Item& outItem) {
    if (!hasPendingLoot) {
        return false;
    }
    outItem = pendingLoot;
    hasPendingLoot = false;
    pendingLoot = Item();
    return true;
}

void Room::addExit(std::string direction, Room* neighbor, std::string blurb) {
    exits[direction] = neighbor;
    if (!blurb.empty()) {
        exitBlurbs[direction] = blurb;
    }
}

Room* Room::getExit(std::string direction) const {
    auto it = exits.find(direction);
    if (it != exits.end()) {
        return it->second;
    }
    return nullptr;
}

std::map<std::string, Room*> Room::getExits() const {
    return exits;
}

void Room::addHiddenItem(Item item, std::string descriptor) {
    HiddenItem hi;
    hi.item = item;
    hi.descriptor = descriptor;
    hi.discovered = false;
    hiddenItems.push_back(hi);
}

void Room::addDiscoveredItem(Item item, std::string descriptor) {
    HiddenItem hi;
    hi.item = item;
    hi.descriptor = descriptor;
    hi.discovered = true;
    hiddenItems.push_back(hi);
}

int Room::searchUndiscovered(std::vector<std::string>& newlyFound) {
    int count = 0;
    for (auto& hi : hiddenItems) {
        if (!hi.discovered) {
            hi.discovered = true;
            newlyFound.push_back(hi.descriptor);
            count++;
        }
    }
    return count;
}

bool Room::removeItem(std::string itemName, Item& outItem) {
    std::string key = normalizeItemKey(itemName);
    auto it = std::find_if(hiddenItems.begin(), hiddenItems.end(), [&](const HiddenItem& hi) {
        if (!hi.discovered) return false;
        return normalizeItemKey(hi.item.getName()) == key
            || toLowerStr(hi.descriptor).find(toLowerStr(itemName)) != std::string::npos;
    });

    if (it != hiddenItems.end()) {
        outItem = it->item;
        hiddenItems.erase(it);
        return true;
    }
    return false;
}

bool Room::hasItem(std::string itemName) const {
    Item dummy;
    return findItem(itemName, dummy);
}

bool Room::findItem(std::string itemName, Item& outItem) const {
    std::string key = normalizeItemKey(itemName);
    auto it = std::find_if(hiddenItems.begin(), hiddenItems.end(), [&](const HiddenItem& hi) {
        if (!hi.discovered) return false;
        return normalizeItemKey(hi.item.getName()) == key
            || toLowerStr(hi.descriptor).find(toLowerStr(itemName)) != std::string::npos;
    });

    if (it != hiddenItems.end()) {
        outItem = it->item;
        return true;
    }
    return false;
}

bool Room::findDiscoveredByDescriptor(std::string text, Item& outItem) const {
    return findItem(text, outItem);
}

void Room::setEnemy(Enemy* newEnemy) {
    if (enemy) delete enemy;
    enemy = newEnemy;
}

Enemy* Room::getEnemy() const {
    return enemy;
}

void Room::clearEnemy() {}

void Room::setNPC(NPC* newNPC) {
    for (NPC* npc : npcs) {
        delete npc;
    }
    npcs.clear();
    if (newNPC) {
        npcs.push_back(newNPC);
    }
}

NPC* Room::getNPC() const {
    if (npcs.empty()) {
        return nullptr;
    }
    return npcs.front();
}

void Room::addNPC(NPC* newNPC) {
    if (newNPC) {
        npcs.push_back(newNPC);
    }
}

bool Room::removeNPC(const std::string& nameFragment) {
    std::string target = toLowerStr(nameFragment);
    auto it = std::find_if(npcs.begin(), npcs.end(), [&](NPC* npc) {
        return npc->matchesTalkTarget(nameFragment);
    });
    if (it != npcs.end()) {
        delete *it;
        npcs.erase(it);
        return true;
    }
    return false;
}

std::vector<NPC*> Room::getNPCs() const {
    return npcs;
}

void Room::setLock(std::string keyName, std::string lockMsg) {
    isLocked = true;
    keyRequired = keyName;
    lockMessage = lockMsg;
}

bool Room::getIsLocked() const { return isLocked; }
std::string Room::getLockMessage() const { return lockMessage; }
std::string Room::getKeyRequired() const { return keyRequired; }

bool Room::unlock(const std::string& keyName) {
    const std::string k1 = toLowerStr(keyRequired);
    const std::string k2 = toLowerStr(keyName);

    if (isLocked && k1 == k2) {
        isLocked = false;
        return true;
    }
    return false;
}
