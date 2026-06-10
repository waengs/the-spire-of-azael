#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <map>
#include <vector>
#include "Item.h"
#include "Character.h"

struct HiddenItem {
    Item item;
    std::string descriptor;
    bool discovered;
};

class Room {
private:
    std::string name;
    std::string description;
    std::map<std::string, Room*> exits;
    std::map<std::string, std::string> exitBlurbs;
    std::vector<HiddenItem> hiddenItems;
    Enemy* enemy;
    std::vector<NPC*> npcs;
    
    bool isLocked;
    std::string keyRequired;
    std::string lockMessage;
    bool hideExitsWhileGuard;
    bool hasPendingLoot;
    Item pendingLoot;

public:
    Room(std::string name, std::string description);
    ~Room();

    std::string getName() const;
    std::string getDescription() const;
    void setDescription(std::string newDescription);
    std::string getDetailedDescription() const;

    void addExit(std::string direction, Room* neighbor, std::string blurb = "");
    Room* getExit(std::string direction) const;
    std::map<std::string, Room*> getExits() const;

    void addHiddenItem(Item item, std::string descriptor);
    void addDiscoveredItem(Item item, std::string descriptor);
    int searchUndiscovered(std::vector<std::string>& newlyFound);
    bool removeItem(std::string itemName, Item& outItem);
    bool hasItem(std::string itemName) const;
    bool findItem(std::string itemName, Item& outItem) const;
    bool findDiscoveredByDescriptor(std::string text, Item& outItem) const;

    void setEnemy(Enemy* newEnemy);
    Enemy* getEnemy() const;
    void clearEnemy();

    void setNPC(NPC* newNPC);
    NPC* getNPC() const;
    void addNPC(NPC* newNPC);
    bool removeNPC(const std::string& nameFragment);
    std::vector<NPC*> getNPCs() const;

    void setLock(std::string keyName, std::string lockMsg = "The way is locked. You need a key.");
    bool getIsLocked() const;
    std::string getLockMessage() const;
    std::string getKeyRequired() const;
    bool unlock(const std::string& keyName);
    void setHideExitsWhileGuard(bool hide);
    void setPendingLoot(Item item);
    bool hasUnclaimedLoot() const;
    bool claimPendingLoot(Item& outItem);
};

#endif
