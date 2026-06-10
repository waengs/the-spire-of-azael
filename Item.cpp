#include "Item.h"

Item::Item() : name(""), description(""), type(ItemType::VALUABLE), effectValue(0), goldValue(0) {}

Item::Item(std::string name, std::string description, ItemType type, int effectValue, int goldValue)
    : name(name), description(description), type(type), effectValue(effectValue), goldValue(goldValue) {}

std::string Item::getName() const { return name; }
std::string Item::getDescription() const { return description; }
ItemType Item::getType() const { return type; }
int Item::getEffectValue() const { return effectValue; }
int Item::getGoldValue() const { return goldValue; }

std::string Item::getTypeString() const {
    switch (type) {
        case ItemType::WEAPON: return "Weapon";
        case ItemType::ARMOR: return "Armor";
        case ItemType::CONSUMABLE: return "Consumable";
        case ItemType::KEY: return "Key";
        case ItemType::VALUABLE: return "Valuable";
    }
    return "Unknown";
}
