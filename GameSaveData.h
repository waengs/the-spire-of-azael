#ifndef GAMESAVEDATA_H
#define GAMESAVEDATA_H

#include <string>
#include <vector>

struct GameSnapshot {
    int loopCount = 1;
    std::string roomName;
    int health = 100;
    int maxHealth = 100;
    int gold = 20;
    int baseAttack = 8;
    int baseDefense = 1;
    std::string equippedWeapon;
    std::string equippedArmor;
    std::vector<std::string> inventory;
    std::vector<std::string> party;

    bool knowsGarrisonStory = false;
    bool examinedGarrisonArmor = false;
    int eleanorTalkCount = 0;
    bool eleanorHeardAllDialogue = false;
    bool eleanorRingGifted = false;
    bool examinedFountain = false;
    bool lyraRiddleSolved = false;
    bool pipBreadGiven = false;
    bool pipPraised = false;
    bool pipGenuineBond = false;
    bool pipBribed = false;
    bool pipReadyToRecruit = false;
    bool pipRevealedBySearch = false;
    bool lyraRecruited = false;
    bool bramRecruited = false;
    bool bramMasterwork = false;
    bool bramReadyToRecruit = false;
    bool garrisonCharmed = false;
    bool valerieCharmed = false;
    bool valerieReadyToRecruit = false;
    bool valerieRecruited = false;
    bool spirePacifist = true;
    bool lyraDisruptUsed = false;
    bool specialGiveUnlocked = false;
    bool specialPraiseUnlocked = false;
    bool specialExamineUnlocked = false;
    bool specialRecruitUnlocked = false;
    bool bramSavedGoblin = false;
};

std::string serializeSnapshot(const GameSnapshot& snapshot);
bool deserializeSnapshot(const std::string& blob, GameSnapshot& out);

#endif
