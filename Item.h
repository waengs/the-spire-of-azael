#ifndef ITEM_H
#define ITEM_H

#include <string>

enum class ItemType {
    WEAPON,
    ARMOR,
    CONSUMABLE,
    KEY,
    VALUABLE
};

class Item {
private:
    std::string name;
    std::string description;
    ItemType type;
    int effectValue; // Healing amount for consumables, attack bonus for weapons, defense bonus for armor
    int goldValue;   // Buy/sell price

public:
    Item();
    Item(std::string name, std::string description, ItemType type, int effectValue, int goldValue);

    std::string getName() const;
    std::string getDescription() const;
    ItemType getType() const;
    int getEffectValue() const;
    int getGoldValue() const;
    
    std::string getTypeString() const;
};

#endif
