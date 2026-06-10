#include "Character.h"
#include <algorithm>
#include <cctype>

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

// --- Character Implementation ---
Character::Character(std::string name, int health, int attackPower, int defensePower)
    : name(name), health(health), maxHealth(health), attackPower(attackPower), defensePower(defensePower) {}

std::string Character::getName() const { return name; }
void Character::setName(std::string newName) { name = std::move(newName); }
int Character::getHealth() const { return health; }
int Character::getMaxHealth() const { return maxHealth; }
int Character::getAttackPower() const { return attackPower; }
int Character::getDefensePower() const { return defensePower; }

void Character::setHealth(int hp) {
    health = std::max(0, std::min(maxHealth, hp));
}

void Character::takeDamage(int damage) {
    int effectiveDamage = std::max(1, damage - getDefensePower());
    health = std::max(0, health - effectiveDamage);
}

bool Character::isAlive() const {
    return health > 0;
}

void Character::heal(int amount) {
    health = std::min(maxHealth, health + amount);
}

// --- Enemy Implementation ---
Enemy::Enemy(std::string name, int health, int attackPower, int defensePower, int goldReward, bool isBoss)
    : Character(name, health, attackPower, defensePower), goldReward(goldReward), dropItem(Item()), hasDrop(false), isBoss(isBoss) {}

Enemy::Enemy(std::string name, int health, int attackPower, int defensePower, int goldReward, Item dropItem, bool isBoss)
    : Character(name, health, attackPower, defensePower), goldReward(goldReward), dropItem(dropItem), hasDrop(true), isBoss(isBoss) {}

int Enemy::getGoldReward() const { return goldReward; }
bool Enemy::hasDropItem() const { return hasDrop; }
Item Enemy::getDropItem() const { return dropItem; }
bool Enemy::getIsBoss() const { return isBoss; }

// --- NPC Implementation ---
NPC::NPC(std::string name, std::string presenceDescription, bool isMerchant)
    : Character(name, 100, 0, 0), presenceDescription(presenceDescription),
      dialogueIndex(0), isMerchant(isMerchant) {}

std::string NPC::getPresenceDescription() const { return presenceDescription; }

static std::string npcToLower(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

void NPC::addTalkAlias(std::string alias) {
    talkAliases.push_back(alias);
}

bool NPC::matchesTalkTarget(const std::string& fragment) const {
    std::string target = npcToLower(fragment);
    if (target.empty()) {
        return false;
    }
    if (npcToLower(name).find(target) != std::string::npos) {
        return true;
    }
    for (const std::string& alias : talkAliases) {
        if (npcToLower(alias).find(target) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void NPC::addDialogue(std::string line) {
    dialogueLines.push_back(line);
}

std::string NPC::getDialogue() {
    if (dialogueLines.empty()) {
        return "...";
    }
    if (dialogueIndex >= dialogueLines.size()) {
        return dialogueLines.back();
    }
    std::string line = dialogueLines[dialogueIndex];
    ++dialogueIndex;
    return line;
}

bool NPC::isDialogueExhausted() const {
    return !dialogueLines.empty() && dialogueIndex >= dialogueLines.size();
}

bool NPC::getIsMerchant() const { return isMerchant; }

void NPC::addShopItem(Item item) {
    shopItems.push_back(item);
}

std::vector<Item>& NPC::getShopItems() {
    return shopItems;
}

bool NPC::removeShopItem(std::string itemName) {
    auto it = std::find_if(shopItems.begin(), shopItems.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(itemName);
    });

    if (it != shopItems.end()) {
        shopItems.erase(it);
        return true;
    }
    return false;
}
