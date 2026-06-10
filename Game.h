#ifndef GAME_H
#define GAME_H

#include "GameSaveData.h"
#include "Player.h"
#include "Room.h"
#include <vector>
#include <string>

struct FellowshipMember {
    std::string name;
    int health;
    int maxHealth;
    int attackPower;
    int defensePower;
};

class Game {
private:
    enum class EndingType {
        NONE,
        TRUE_ENDING,
        REDEMPTION_ENDING,
        BITTERSWEET_ENDING,
        BETRAYAL_ENDING
    };

    Player* player;
    NPC* valerieNpc;
    bool valerieNpcPlaced;
    std::vector<Room*> rooms;
    bool isRunning;
    bool gameWon;
    EndingType endingType;

    std::vector<std::string> party;
    std::vector<FellowshipMember> fellowshipRoster;

    bool knowsGarrisonStory;
    bool examinedGarrisonArmor;
    int eleanorTalkCount;
    bool eleanorHeardAllDialogue;
    bool eleanorRingGifted;
    bool examinedFountain;
    bool lyraRiddleSolved;
    bool pipBreadGiven;
    bool pipPraised;
    bool pipGenuineBond;
    bool pipBribed;
    bool pipReadyToRecruit;
    bool pipRevealedBySearch;
    bool lyraRecruited;
    bool bramRecruited;
    bool bramMasterwork;
    bool bramReadyToRecruit;
    bool garrisonCharmed;
    bool valerieCharmed;
    bool valerieReadyToRecruit;
    bool valerieRecruited;
    bool spirePacifist;
    bool lyraDisruptUsed;
    bool specialGiveUnlocked;
    bool specialPraiseUnlocked;
    bool specialExamineUnlocked;
    bool specialRecruitUnlocked;
    int loopCount;
    int malakorTalkCount;
    int azaelTalkCount;
    int knightTalkCount;
    bool bramSavedGoblin;
    bool bramForestFled;
    std::string lastTalkNpcName;

    void initializeMap();
    void resetStoryState();
    void reinitializeWorldAfterLoop();
    void respawnAfterDeath();
    void respawnAfterBetrayal();
    bool isInParty(const std::string& nameFragment) const;
    bool canReachCompanion(const std::string& nameFragment) const;
    void addToParty(const std::string& name, const std::string& roomToLeave);
    void registerFellowshipMember(const std::string& name);
    void healFellowshipRoster();
    void refreshRoomDescriptionsForRecruits();
    static FellowshipMember defaultFellowshipStats(const std::string& name);
    void printWelcome() const;
    void printHelp() const;
    void printTitle() const;
    void playEndingEpilogue();
    void playRedemptionOakvaleEpilogue();
    static bool isPeacefulWorthItAnswer(const std::string& line);
    void printCreditsScreen() const;
    void waitForExitCommand();
    bool processExitCommand(const std::string& line) const;
    void printGameOverScreen() const;
    void printParty() const;

    void handleLook(const std::string& arg);
    void handleGo(const std::string& arg);
    void handleTake(const std::string& arg);
    void handleDrop(const std::string& arg);
    void handleUse(const std::string& arg);
    void handleEquip(const std::string& arg);
    void handleTalk(const std::string& arg);
    void handleBuy(const std::string& arg);
    void handleSell(const std::string& arg);
    void handleGive(const std::string& arg);
    void handlePraise(const std::string& arg);
    void handleRecruit(const std::string& arg);
    void promptLyraRiddleAnswer();
    void handleShow(const std::string& arg);
    void handleExamine(const std::string& arg);
    void handleSearch(const std::string& arg);
    void handlePickupLoot();
    void handleWork();
    void handleRest();
    bool hasLivingEnemyInRoom() const;
    static bool isArmoredKnightEnemy(const Enemy* enemy);
    static bool isGiantCaveBatEnemy(const Enemy* enemy);
    static bool isCaveTrollEnemy(const Enemy* enemy);
    static bool isDemonHoundEnemy(const Enemy* enemy);
    void printPlayerStats(int heroAttackTurnCount = -1) const;
    void tryBribePip(const std::string& target);
    void handleRead(const std::string& arg);
    static void printDiaryContents();
    bool isSpireRoomName(const std::string& roomName) const;
    bool isCombatFleeBlocked(const Enemy* enemy) const;
    static bool topicIsArmorExam(const std::string& topic);

    void startCombat(Enemy* enemy);
    void fellowshipCombatAssist(Enemy* enemy, const std::vector<FellowshipMember>& fighters, int primaryIndex);
    bool charmKnightWithRing();
    bool charmValerieWithRibbon();
    static bool isValerieEnemy(const Enemy* enemy);
    void handleKnightTalk(const std::string& arg);
    void resolveCombatEnding();
    bool canRedeemAzaelWithDiary() const;
    bool tryGiveDiaryToAzael();
    Room* findRoomByName(const std::string& roomName) const;
    NPC* findNPCInCurrentRoom(const std::string& nameFragment) const;
    NPC* selectNpcForConversation(const std::string& arg) const;
    void printConversationTargetsHint() const;
    void processCommandLine(const std::string& line);

    bool hasPendingSnapshot_;
    GameSnapshot pendingSnapshot_;
    static Item createItemByName(const std::string& name);
    void applyStoryFlags(const GameSnapshot& snapshot);

public:
    Game();
    ~Game();

    void start();
    void resetForNewAdventure();

    bool getIsRunning() const;
    bool hasGameWon() const;
    const Player* getPlayer() const;
    const std::vector<std::string>& getFellowship() const;
    int getLoopCount() const;
    const std::string& getLastTalkNpcName() const;

    bool captureSnapshot(GameSnapshot& out) const;
    void queueSnapshotRestore(const GameSnapshot& snapshot);
    bool applyQueuedSnapshot();
};

#endif
