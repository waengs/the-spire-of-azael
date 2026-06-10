#ifndef PLAYER_H
#define PLAYER_H

#include "Character.h"
#include "Room.h"
#include <vector>
#include <string>

class Player : public Character {
private:
    std::vector<Item> inventory;
    Room* currentRoom;
    int gold;

    bool weaponEquipped;
    Item equippedWeapon;

    bool armorEquipped;
    Item equippedArmor;

public:
    Player(std::string name, int health, int attackPower, int defensePower);

    Room* getCurrentRoom() const;
    void setCurrentRoom(Room* room);

    int getGold() const;
    void addGold(int amount);
    bool removeGold(int amount);

    void receiveItem(Item item);
    bool takeItem(std::string itemName);
    bool dropItem(std::string itemName);
    bool hasItem(std::string itemName) const;
    bool getItem(std::string itemName, Item& outItem) const;
    bool removeItem(std::string itemName);

    bool useItem(std::string itemName);
    bool equipItem(std::string itemName);
    void unequipWeapon();
    void unequipArmor();

    void showInventory() const;
    void showStats() const;

    // Overridden methods to account for gear
    int getAttackPower() const;
    int getDefensePower() const;

    bool hasEquippedWeapon() const;
    Item getEquippedWeapon() const;
    bool hasEquippedArmor() const;
    Item getEquippedArmor() const;

    const std::vector<Item>& getInventory() const;

    void applyPersistentState(int health, int maxHealth, int gold, int attack, int defense);
    void addInventoryItem(const Item& item);
};

#endif
