#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include <vector>
#include "Item.h"

class Character {
protected:
    std::string name;
    int health;
    int maxHealth;
    int attackPower;
    int defensePower;

public:
    Character(std::string name, int health, int attackPower, int defensePower);
    virtual ~Character() = default;

    std::string getName() const;
    void setName(std::string newName);
    int getHealth() const;
    int getMaxHealth() const;
    virtual int getAttackPower() const;
    virtual int getDefensePower() const;

    void setHealth(int hp);
    void takeDamage(int damage);
    bool isAlive() const;
    void heal(int amount);
};

class Enemy : public Character {
private:
    int goldReward;
    Item dropItem;
    bool hasDrop;
    bool isBoss;

public:
    Enemy(std::string name, int health, int attackPower, int defensePower, int goldReward, bool isBoss = false);
    Enemy(std::string name, int health, int attackPower, int defensePower, int goldReward, Item dropItem, bool isBoss = false);

    int getGoldReward() const;
    bool hasDropItem() const;
    Item getDropItem() const;
    bool getIsBoss() const;
};

class NPC : public Character {
private:
    std::string presenceDescription;
    std::vector<std::string> talkAliases;
    std::vector<std::string> dialogueLines;
    size_t dialogueIndex;
    bool isMerchant;
    std::vector<Item> shopItems;

public:
    NPC(std::string name, std::string presenceDescription, bool isMerchant = false);
    
    void addDialogue(std::string line);
    std::string getDialogue();
    bool isDialogueExhausted() const;
    std::string getPresenceDescription() const;
    void addTalkAlias(std::string alias);
    bool matchesTalkTarget(const std::string& fragment) const;

    bool getIsMerchant() const;
    void addShopItem(Item item);
    std::vector<Item>& getShopItems();
    bool removeShopItem(std::string itemName);
};

#endif
