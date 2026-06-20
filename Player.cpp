#include "Player.h"
#include <iostream>
#include <algorithm>
#include <sstream>
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

Player::Player(std::string name, int health, int attackPower, int defensePower)
    : Character(name, health, attackPower, defensePower),
      currentRoom(nullptr), gold(20),
      weaponEquipped(false), equippedWeapon(Item()),
      armorEquipped(false), equippedArmor(Item()) {}

Room* Player::getCurrentRoom() const { return currentRoom; }
void Player::setCurrentRoom(Room* room) { currentRoom = room; }

int Player::getGold() const { return gold; }
void Player::addGold(int amount) { gold += amount; }
bool Player::removeGold(int amount) {
    if (gold >= amount) {
        gold -= amount;
        return true;
    }
    return false;
}

static std::string displayItemName(const std::string& name) {
    const std::string key = normalizeItemKey(name);
    if (key == "eleanorsring") {
        return "Eleanor's Ring";
    }
    return name;
}

void Player::receiveItem(Item item) {
    inventory.push_back(item);
    std::cout << "You acquired: \033[1;33m" << displayItemName(item.getName()) << "\033[0m\n";
}

bool Player::takeItem(std::string itemName) {
    if (!currentRoom) return false;
    
    Item item;
    if (currentRoom->removeItem(itemName, item)) {
        inventory.push_back(item);
        std::cout << "You pick up: \033[1;33m" << item.getName() << "\033[0m\n";
        return true;
    }
    return false;
}

bool Player::dropItem(std::string itemName) {
    auto it = std::find_if(inventory.begin(), inventory.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(itemName);
    });

    if (it != inventory.end()) {
        Item item = *it;
        
        // If the item is currently equipped, unequip it
        if (weaponEquipped && equippedWeapon.getName() == item.getName()) {
            unequipWeapon();
        } else if (armorEquipped && equippedArmor.getName() == item.getName()) {
            unequipArmor();
        }

        inventory.erase(it);
        if (currentRoom) {
            currentRoom->addDiscoveredItem(item, "something you dropped");
        }
        std::cout << "You drop: \033[1;33m" << item.getName() << "\033[0m\n";
        return true;
    }
    return false;
}

bool Player::hasItem(std::string itemName) const {
    Item dummy;
    return getItem(itemName, dummy);
}

bool Player::getItem(std::string itemName, Item& outItem) const {
    auto it = std::find_if(inventory.begin(), inventory.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(itemName);
    });

    if (it != inventory.end()) {
        outItem = *it;
        return true;
    }
    return false;
}

bool Player::removeItem(std::string itemName) {
    auto it = std::find_if(inventory.begin(), inventory.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(itemName);
    });

    if (it != inventory.end()) {
        if (weaponEquipped && equippedWeapon.getName() == it->getName()) {
            unequipWeapon();
        } else if (armorEquipped && equippedArmor.getName() == it->getName()) {
            unequipArmor();
        }
        inventory.erase(it);
        return true;
    }
    return false;
}

bool Player::useItem(std::string itemName) {
    auto it = std::find_if(inventory.begin(), inventory.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(itemName);
    });

    if (it != inventory.end()) {
        if (it->getType() == ItemType::CONSUMABLE) {
            int healAmt = it->getEffectValue();
            heal(healAmt);
            std::cout << "You use \033[1;33m" << it->getName() << "\033[0m and heal for \033[1;32m" << healAmt << " HP\033[0m.\n";
            std::cout << "HP is now: \033[1;32m" << health << "/" << maxHealth << "\033[0m\n";
            inventory.erase(it);
            return true;
        } else if (it->getType() == ItemType::KEY) {
            std::cout << "Keys are used automatically when you attempt to open locked doors.\n";
            return true;
        } else {
            std::cout << "You can't use " << it->getName() << " directly. Perhaps you mean to 'equip' it?\n";
            return true;
        }
    }
    return false;
}

bool Player::equipItem(std::string itemName) {
    auto it = std::find_if(inventory.begin(), inventory.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(itemName);
    });

    if (it != inventory.end()) {
        if (it->getType() == ItemType::WEAPON) {
            if (weaponEquipped) {
                std::cout << "Unequipping " << equippedWeapon.getName() << ".\n";
            }
            weaponEquipped = true;
            equippedWeapon = *it;
            std::cout << "You equipped \033[1;33m" << equippedWeapon.getName() << "\033[0m (+ " 
                      << equippedWeapon.getEffectValue() << " Attack Power)!\n";
            return true;
        } else if (it->getType() == ItemType::ARMOR) {
            if (armorEquipped) {
                std::cout << "Unequipping " << equippedArmor.getName() << ".\n";
            }
            armorEquipped = true;
            equippedArmor = *it;
            std::cout << "You equipped \033[1;33m" << equippedArmor.getName() << "\033[0m (+ " 
                      << equippedArmor.getEffectValue() << " Defense Power)!\n";
            return true;
        } else {
            std::cout << "You cannot equip " << it->getName() << "!\n";
            return true;
        }
    }
    return false;
}

void Player::unequipWeapon() {
    if (weaponEquipped) {
        std::cout << "You unequipped: \033[1;33m" << equippedWeapon.getName() << "\033[0m\n";
        weaponEquipped = false;
        equippedWeapon = Item();
    }
}

void Player::unequipArmor() {
    if (armorEquipped) {
        std::cout << "You unequipped: \033[1;33m" << equippedArmor.getName() << "\033[0m\n";
        armorEquipped = false;
        equippedArmor = Item();
    }
}

void Player::showInventory() const {
    std::cout << "\033[1;36m=== INVENTORY ===\033[0m\n";
    std::cout << "Gold: \033[1;33m" << gold << "g\033[0m\n";
    
    std::cout << "Equipped Weapon: ";
    if (weaponEquipped) {
        std::cout << "\033[1;33m" << equippedWeapon.getName() << "\033[0m (+" << equippedWeapon.getEffectValue() << " AP)\n";
    } else {
        std::cout << "None\n";
    }

    std::cout << "Equipped Armor:  ";
    if (armorEquipped) {
        std::cout << "\033[1;33m" << equippedArmor.getName() << "\033[0m (+" << equippedArmor.getEffectValue() << " Def)\n";
    } else {
        std::cout << "None\n";
    }

    std::cout << "Bag Contents:\n";
    if (inventory.empty()) {
        std::cout << "  (empty)\n";
    } else {
        bool anyBagItem = false;
        for (const auto& item : inventory) {
            if (weaponEquipped && item.getName() == equippedWeapon.getName()) {
                continue;
            }
            if (armorEquipped && item.getName() == equippedArmor.getName()) {
                continue;
            }
            anyBagItem = true;
            std::cout << "  - \033[1;33m" << item.getName() << "\033[0m: " << item.getDescription();
            if (item.getType() == ItemType::WEAPON) {
                std::cout << " [+" << item.getEffectValue() << " attack]";
            } else if (item.getType() == ItemType::ARMOR) {
                std::cout << " [+" << item.getEffectValue() << " defense]";
            } else if (item.getType() == ItemType::CONSUMABLE && item.getEffectValue() > 0) {
                std::cout << " [heals " << item.getEffectValue() << " HP]";
            }
            if (item.getGoldValue() > 0) {
                std::cout << " (" << item.getGoldValue() << "g value)";
            }
            std::cout << "\n";
        }
        if (!anyBagItem) {
            std::cout << "  (empty)\n";
        }
    }
    std::cout << "\033[1;36m=================\033[0m\n";
}

void Player::showStats() const {
    std::cout << "\033[1;36m=== PLAYER STATS ===\033[0m\n";
    std::cout << "Name:    " << name << "\n";
    std::cout << "HP:      \033[1;32m" << health << "/" << maxHealth << "\033[0m\n";
    std::cout << "Attack:  \033[1;31m" << getAttackPower() << "\033[0m (" << attackPower << " Base + " 
              << (weaponEquipped ? equippedWeapon.getEffectValue() : 0) << " Weapon)\n";
    std::cout << "Defense: \033[1;34m" << getDefensePower() << "\033[0m (" << defensePower << " Base + " 
              << (armorEquipped ? equippedArmor.getEffectValue() : 0) << " Armor)\n";
    std::cout << "Gold:    \033[1;33m" << gold << "g\033[0m\n";
    std::cout << "\033[1;36m====================\033[0m\n";
}

int Player::getAttackPower() const {
    int total = attackPower;
    if (weaponEquipped) {
        total += equippedWeapon.getEffectValue();
    }
    return total;
}

int Player::getDefensePower() const {
    int total = defensePower;
    if (armorEquipped) {
        total += equippedArmor.getEffectValue();
    }
    return total;
}

bool Player::hasEquippedWeapon() const { return weaponEquipped; }
Item Player::getEquippedWeapon() const { return equippedWeapon; }
bool Player::hasEquippedArmor() const { return armorEquipped; }
Item Player::getEquippedArmor() const { return equippedArmor; }

const std::vector<Item>& Player::getInventory() const { return inventory; }

void Player::applyPersistentState(int hp, int maxHp, int newGold, int attack, int defense) {
    health = hp;
    maxHealth = maxHp;
    gold = newGold;
    attackPower = attack;
    defensePower = defense;
}

void Player::addInventoryItem(const Item& item) {
    inventory.push_back(item);
}
