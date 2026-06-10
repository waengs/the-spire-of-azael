#include "Game.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cctype>

// Helper to trim whitespace
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Helper to convert to lowercase
static std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return str;
}

static bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

static int editDistance(const std::string& a, const std::string& b) {
    const size_t n = a.size();
    const size_t m = b.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (size_t i = 0; i <= n; ++i) dp[i][0] = static_cast<int>(i);
    for (size_t j = 0; j <= m; ++j) dp[0][j] = static_cast<int>(j);
    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,
                dp[i][j - 1] + 1,
                dp[i - 1][j - 1] + cost
            });
        }
    }
    return dp[n][m];
}

static std::string closestWord(
    const std::string& input,
    const std::vector<std::string>& options,
    int maxDistance = 2
) {
    std::string best = "";
    int bestDist = 999;
    for (const std::string& opt : options) {
        int dist = editDistance(input, opt);
        if (dist < bestDist) {
            bestDist = dist;
            best = opt;
        }
    }
    if (bestDist <= maxDistance) return best;
    return "";
}

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

static std::string formatItemDisplayName(const std::string& internalName) {
    if (internalName.empty()) {
        return internalName;
    }
    if (normalizeItemKey(internalName) == "eleanorsring") {
        return "Eleanor's Ring";
    }
    std::string out;
    out.push_back(internalName[0]);
    for (size_t i = 1; i < internalName.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(internalName[i]);
        const unsigned char prev = static_cast<unsigned char>(internalName[i - 1]);
        if (std::isupper(c) && std::islower(prev)) {
            out.push_back(' ');
        }
        out.push_back(internalName[i]);
    }
    return out;
}

static void printMerchantInventory(const std::vector<Item>& shopItems) {
    struct StockLine {
        std::string name;
        ItemType type;
        int unitPrice = 0;
        int count = 0;
    };
    std::vector<StockLine> lines;
    for (const Item& item : shopItems) {
        const std::string key = normalizeItemKey(item.getName());
        StockLine* match = nullptr;
        for (StockLine& line : lines) {
            if (normalizeItemKey(line.name) == key && line.unitPrice == item.getGoldValue()) {
                match = &line;
                break;
            }
        }
        if (match) {
            ++match->count;
        } else {
            lines.push_back({item.getName(), item.getType(), item.getGoldValue(), 1});
        }
    }

    Item probe;
    for (const StockLine& stock : lines) {
        const std::string label = formatItemDisplayName(stock.name);
        probe = Item(stock.name, "", stock.type, 0, stock.unitPrice);
        if (stock.count > 1 && stock.type == ItemType::CONSUMABLE) {
            std::cout << "  - \033[1;33m" << label << " x" << stock.count << "\033[0m : \033[1;32m"
                      << stock.unitPrice << "g\033[0m each\n";
        } else {
            std::cout << "  - \033[1;33m" << label << "\033[0m (" << probe.getTypeString() << "): \033[1;32m"
                      << stock.unitPrice << "g\033[0m\n";
        }
    }
}

Game::Game()
    : player(nullptr),
      valerieNpc(nullptr),
      valerieNpcPlaced(false),
      hasPendingSnapshot_(false),
      isRunning(false),
      gameWon(false),
      endingType(EndingType::NONE),
      knowsGarrisonStory(false),
      examinedGarrisonArmor(false),
      eleanorTalkCount(0),
      eleanorHeardAllDialogue(false),
      eleanorRingGifted(false),
      pipBreadGiven(false),
      pipPraised(false),
      pipGenuineBond(false),
      pipBribed(false),
      lyraRecruited(false),
      bramRecruited(false),
      bramMasterwork(false),
      garrisonCharmed(false),
      valerieCharmed(false),
      spirePacifist(true),
      lyraDisruptUsed(false),
      specialGiveUnlocked(false),
      specialPraiseUnlocked(false),
      specialExamineUnlocked(false),
      specialRecruitUnlocked(false),
      loopCount(1),
      malakorTalkCount(0),
      azaelTalkCount(0),
      knightTalkCount(0),
      bramSavedGoblin(false),
      bramForestFled(false),
      examinedFountain(false),
      lyraRiddleSolved(false),
      pipReadyToRecruit(false),
      pipRevealedBySearch(false),
      bramReadyToRecruit(false),
      valerieReadyToRecruit(false),
      valerieRecruited(false),
      lastTalkNpcName() {}

Game::~Game() {
    delete player;
    if (valerieNpc && !valerieNpcPlaced) {
        delete valerieNpc;
    }
    for (auto room : rooms) {
        delete room;
    }
}

void Game::resetStoryState() {
    party.clear();
    fellowshipRoster.clear();
    knowsGarrisonStory = false;
    examinedGarrisonArmor = false;
    eleanorTalkCount = 0;
    eleanorHeardAllDialogue = false;
    eleanorRingGifted = false;
    examinedFountain = false;
    lyraRiddleSolved = false;
    pipBreadGiven = false;
    pipPraised = false;
    pipGenuineBond = false;
    pipBribed = false;
    pipReadyToRecruit = false;
    pipRevealedBySearch = false;
    lyraRecruited = false;
    bramRecruited = false;
    bramMasterwork = false;
    bramReadyToRecruit = false;
    garrisonCharmed = false;
    valerieCharmed = false;
    valerieReadyToRecruit = false;
    valerieRecruited = false;
    valerieNpcPlaced = false;
    spirePacifist = true;
    lyraDisruptUsed = false;
    specialGiveUnlocked = false;
    specialPraiseUnlocked = false;
    specialExamineUnlocked = false;
    specialRecruitUnlocked = false;
    bramSavedGoblin = false;
    bramForestFled = false;
    malakorTalkCount = 0;
    azaelTalkCount = 0;
    knightTalkCount = 0;
    endingType = EndingType::NONE;
    lastTalkNpcName.clear();
}

void Game::resetForNewAdventure() {
    isRunning = false;
    gameWon = false;
    hasPendingSnapshot_ = false;
    loopCount = 1;

    delete player;
    player = nullptr;
    if (valerieNpc && !valerieNpcPlaced) {
        delete valerieNpc;
    }
    for (Room* room : rooms) {
        delete room;
    }
    rooms.clear();
    valerieNpc = nullptr;
    valerieNpcPlaced = false;

    resetStoryState();
}

bool Game::isArmoredKnightEnemy(const Enemy* enemy) {
    if (!enemy) {
        return false;
    }
    const std::string& name = enemy->getName();
    return name == "Armored Knight" || name == "Sir Garrison";
}

bool Game::isGiantCaveBatEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Giant Cave Bat";
}

bool Game::isCaveTrollEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Cave Troll";
}

bool Game::isDemonHoundEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Demon Hound";
}

static bool isGoblinScoutEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Goblin Scout";
}

static bool isMalakorEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Demon General Malakor";
}

static bool isAzaelEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Demon Lord Azael";
}

static void printCombatBattleStart(const Enemy* enemy, bool knightFight, bool azaelFight, bool batFight,
    bool trollFight, bool houndFight, bool goblinFight, bool malakorFight, bool valerieFight) {
    if (knightFight) {
        std::cout << "The \033[1;31mArmored Knight\033[0m lifts his blade.\n";
        std::cout << "\"Steel decides who passes. Not questions.\"\n";
    } else if (azaelFight) {
        std::cout << "\033[1;31mDemon Lord Azael\033[0m rises from the throne.\n\n";
        std::cout << "The air in the cathedral tightens instantly.\n";
    } else if (batFight) {
        std::cout << "A wild \033[1;31mGiant Cave Bat\033[0m attacks!\n";
    } else if (trollFight) {
        std::cout << "The \033[1;31mCave Troll\033[0m attacks!\n";
    } else if (houndFight) {
        std::cout << "The \033[1;31mDemon Hound\033[0m lunges.\n";
    } else if (goblinFight) {
        std::cout << "The \033[1;31mGoblin Scout\033[0m rushes toward you!\n";
    } else if (malakorFight) {
        std::cout << "\033[1;31mDemon General Malakor\033[0m steps forward through the ritual fire.\n";
        std::cout << "\"The throne lies beyond me. You will not pass alive.\"\n";
    } else if (valerieFight) {
        std::cout << "\033[1;31mValerie\033[0m spreads her wings; the mountain wind screams around the ledge.\n";
    } else if (enemy) {
        std::cout << "\033[1;31m" << enemy->getName() << "\033[0m attacks!\n";
    }
}

static void printCombatHeroStrike(const Enemy* enemy, bool batFight, bool trollFight, bool houndFight,
    bool goblinFight, bool knightFight, bool malakorFight, bool azaelFight, bool valerieFight,
    int damageDealt) {
    if (!enemy) {
        return;
    }
    const std::string& name = enemy->getName();
    if (azaelFight) {
        std::cout << "You strike \033[1;31mDemon Lord Azael\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (malakorFight) {
        std::cout << "You strike \033[1;31mDemon General Malakor\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (goblinFight) {
        std::cout << "You strike the \033[1;31mGoblin Scout\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (knightFight) {
        std::cout << "You strike the \033[1;31mArmored Knight\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (batFight) {
        std::cout << "You strike the \033[1;31mGiant Cave Bat\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (trollFight) {
        std::cout << "You strike the \033[1;31mCave Troll\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (houndFight) {
        std::cout << "You strike the \033[1;31mDemon Hound\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else if (valerieFight) {
        std::cout << "You strike \033[1;31mValerie\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    } else {
        std::cout << "You strike \033[1;31m" << name << "\033[0m for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    }
}

static void printCombatEnemyStrike(const Enemy* enemy, bool batFight, bool trollFight, bool houndFight,
    bool goblinFight, bool knightFight, bool malakorFight, bool azaelFight, bool valerieFight,
    int targetIdx, const std::string& targetName, int enemyDmg, bool batUsedPotionThisTurn,
    int batAttackCount, bool heroStruckThisTurn, int azaelStrikeCount) {
    if (batFight) {
        if (batUsedPotionThisTurn) {
            std::cout << "\nThe Giant Cave Bat screeches and strikes while you recover!\n\n";
        } else if (batAttackCount > 0) {
            std::cout << "\nThe wounded creature flaps wildly and attacks back!\n\n";
        } else {
            std::cout << "\nThe Giant Cave Bat strikes from the darkness!\n\n";
        }
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (trollFight && targetIdx == 0) {
        std::cout << "\nThe Cave Troll swings with enormous force!\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (trollFight) {
        std::cout << "\nThe Cave Troll swings a massive fist at \033[1;32m" << targetName
                  << "\033[0m!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (houndFight && heroStruckThisTurn && targetIdx == 0) {
        std::cout << "\nIt twists mid-motion and retaliates instantly.\n\n";
    }
    if (houndFight && targetIdx == 0) {
        std::cout << "\nThe Demon Hound bites into you.\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (houndFight) {
        std::cout << "\nThe Demon Hound lunges at \033[1;32m" << targetName << "\033[0m!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (goblinFight && targetIdx == 0) {
        std::cout << "\nThe Goblin Scout slashes wildly at you!\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (goblinFight) {
        std::cout << "\nThe Goblin Scout swipes at \033[1;32m" << targetName << "\033[0m!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (knightFight && targetIdx == 0) {
        std::cout << "\nThe Armored Knight drives his blade forward in a heavy arc!\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (knightFight) {
        std::cout << "\nThe Armored Knight strikes \033[1;32m" << targetName << "\033[0m with his shield edge!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (malakorFight && targetIdx == 0) {
        std::cout << "\nMalakor cleaves toward you with brutal force!\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (malakorFight) {
        std::cout << "\nMalakor's axe finds \033[1;32m" << targetName << "\033[0m!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (azaelFight && targetIdx == 0) {
        if (azaelStrikeCount == 0) {
            std::cout << "\nAzael does not stagger.\n\n";
            std::cout << "He exhales once.\n\n";
            std::cout << "The throne behind him cracks.\n\n";
        }
        std::cout << "A wave of force tears across the hall.\n\n";
        std::cout << "You are struck.\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (azaelFight) {
        std::cout << "\nDark power lashes toward \033[1;32m" << targetName << "\033[0m!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (valerieFight && targetIdx == 0) {
        std::cout << "\nValerie rakes the wind into razor talons toward you!\n\n";
        std::cout << "You take \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (valerieFight) {
        std::cout << "\nValerie dives at \033[1;32m" << targetName << "\033[0m with her talons extended!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
        return;
    }
    if (enemy) {
        std::cout << "\n\033[1;31m" << enemy->getName() << "\033[0m attacks \033[1;32m" << targetName
                  << "\033[0m!\n\n";
        std::cout << targetName << " takes \033[1;31m" << enemyDmg << " damage\033[0m.\n";
    }
}

static void printCombatVictory(const Enemy* enemy, bool batFight, bool trollFight, bool goblinFight,
    bool knightFight, bool malakorFight, bool houndFight, bool valerieFight, bool azaelFight) {
    if (batFight) {
        std::cout << "\nThe creature lets out one final screech before collapsing into the darkness.\n\n";
        std::cout << "\033[1;32mYou defeated the Giant Cave Bat!\033[0m\n\n";
    } else if (trollFight) {
        std::cout << "\nThe troll lets out a final roar before collapsing onto the cavern floor.\n\n";
        std::cout << "\033[1;32mYou defeated the Cave Troll!\033[0m\n";
    } else if (goblinFight) {
        std::cout << "\nThe goblin stumbles back before collapsing.\n\n";
        std::cout << "\033[1;32mYou defeated the Goblin Scout!\033[0m\n";
    } else if (knightFight) {
        std::cout << "\nThe final blow lands.\n\n";
        std::cout << "The knight staggers backward.\n\n";
        std::cout << "\"...So the Spire has chosen you.\"\n\n";
        std::cout << "\033[1;32mYou defeated the Armored Knight!\033[0m\n";
    } else if (houndFight) {
        std::cout << "\nThe Demon Hound whines once, then goes still.\n\n";
        std::cout << "\033[1;32mYou defeated the Demon Hound!\033[0m\n";
    } else if (malakorFight) {
        std::cout << "\nMalakor falls to one knee as the ritual runes fade.\n\n";
        std::cout << "\033[1;32mYou defeated Demon General Malakor!\033[0m\n";
    } else if (valerieFight) {
        std::cout << "\nValerie breaks off mid-dive, wings folding as she yields the ledge.\n\n";
        std::cout << "\033[1;32mYou defeated Valerie!\033[0m\n";
    } else if (azaelFight) {
        std::cout << "\nAzael falls in battle.\n\n";
        std::cout << "\033[1;32mYou defeated Demon Lord Azael!\033[0m\n";
    } else if (enemy) {
        std::cout << "\n\033[1;32mYou defeated " << enemy->getName() << "!\033[0m\n";
    }
}

static constexpr int kBramMasterworkDefenseBonus = 5;

static FellowshipMember applyFellowshipGearBonuses(const FellowshipMember& member, bool masterwork) {
    FellowshipMember out = member;
    if (masterwork) {
        out.defensePower += kBramMasterworkDefenseBonus;
    }
    return out;
}

void Game::printDiaryContents() {
    std::cout << "\033[1;33m--- Azael's Diary ---\033[0m\n\n";
    std::cout << "The cover is burned, but the pages inside survived.\n\n";
    std::cout << "You read fragments of a man who once guarded a kingdom, not ruled it.\n";
    std::cout << "He speaks of knights who swore to him, of a sky-touched guardian who chose exile over slaughter,\n";
    std::cout << "and of a scholar who warned that mercy and power cannot share one throne forever.\n\n";
    std::cout << "Near the end, the handwriting breaks:\n";
    std::cout << "\"If they reach me only through ruin, then ruin is all I will be.\"\n\n";
    std::cout << "The final line is almost gentle:\n";
    std::cout << "\"Perhaps someone will climb without breaking what I could not save.\"\n";
    std::cout << "\033[1;33m---------------------\033[0m\n";
}

void Game::printPlayerStats(int heroAttackTurnCount) const {
    if (!player) {
        return;
    }
    player->showStats();
    if (bramMasterwork) {
        std::cout << "Fellowship: \033[1;34m+" << kBramMasterworkDefenseBonus
                  << " defense\033[0m from Bram's masterwork (all companions).\n";
    }
    if (!fellowshipRoster.empty()) {
        std::cout << "Fellowship: " << fellowshipRoster.size() << " companion(s) enlisted.\n";
    }
    if (heroAttackTurnCount >= 0 && fellowshipRoster.size() > 0) {
        const int untilRally = 5 - (heroAttackTurnCount % 5);
        if (untilRally <= 1) {
            std::cout << "Coordinated assault: \033[1;32mREADY\033[0m - your next strike rallies the fellowship!\n";
        } else {
            std::cout << "Coordinated assault: ready in \033[1;36m" << untilRally
                      << "\033[0m more hero strike(s).\n";
        }
    }
}

void Game::handleRead(const std::string& arg) {
    std::string topic = toLower(trim(arg));
    if (topic.empty() || topic == "diary" || topic == "azaelsdiary" || topic == "azaels"
        || topic == "journal") {
        if (!player->hasItem("AzaelsDiary")) {
            std::cout << "You do not have Azael's Diary to read.\n";
            return;
        }
        printDiaryContents();
        return;
    }
    std::cout << "Read what? Try \033[1;32mread diary\033[0m.\n";
}

void Game::tryBribePip(const std::string& target) {
    std::string who = toLower(trim(target));
    if (!who.empty() && who.find("pip") == std::string::npos) {
        std::cout << "You can only bribe Pip here.\n";
        return;
    }
    if (!canReachCompanion("pip")) {
        std::cout << "Pip is not here.\n";
        return;
    }
    if (pipBribed) {
        std::cout << "Pip already took your coin. She will not bargain twice.\n";
        return;
    }
    const int bribeGold = 10;
    const int goldBefore = player->getGold();
    if (goldBefore < bribeGold) {
        std::cout << "You need at least \033[1;33m" << bribeGold << "g\033[0m to bribe Pip.";
        std::cout << " You only have \033[1;33m" << goldBefore << "g\033[0m.\n";
        return;
    }
    if (!player->removeGold(bribeGold)) {
        std::cout << "You need at least \033[1;33m" << bribeGold << "g\033[0m to bribe Pip.";
        std::cout << " You only have \033[1;33m" << player->getGold() << "g\033[0m.\n";
        return;
    }
    pipBribed = true;
    pipReadyToRecruit = true;
    std::cout << "You press \033[1;33m" << bribeGold << "g\033[0m into Pip's palm.\n";
    std::cout << "Gold spent bribing Pip: \033[1;33m" << bribeGold << "g\033[0m. ";
    std::cout << "Gold remaining: \033[1;33m" << player->getGold() << "g\033[0m.\n\n";
    std::cout << "Pip snatches the coins quickly, then weighs them in her palm.\n\n";
    std::cout << "\033[1;35mPip says:\033[0m \"...Yeah. That's real.\"\n\n";
    std::cout << "She pockets them without looking away from you.\n\n";
    std::cout << "\"Don't pretend this is anything else.\"\n\n";
    std::cout << "\"You're buying a blade. Not a friend.\"\n\n";
    std::cout << "\"But I keep my word. If I'm with you, I fight.\"\n\n";
    std::cout << "\"Just don't expect me to bleed for you for free.\"\n";
    std::cout << "Say \033[1;32mrecruit pip\033[0m when you want her in your crew.\n";
}

static bool isHealingConsumableKey(const std::string& text) {
    const std::string key = normalizeItemKey(text);
    return key == "healthpotion" || key == "potion" || key == "elixir";
}

static bool findHealingItemInInventory(const Player* player, const std::string& token, Item& outItem) {
    const std::string key = normalizeItemKey(token);
    if (key == "healthpotion" || key == "potion") {
        return player->getItem("HealthPotion", outItem);
    }
    if (key == "elixir") {
        return player->getItem("Elixir", outItem);
    }
    if (player->getItem("HealthPotion", outItem)) {
        return true;
    }
    return player->getItem("Elixir", outItem);
}

static std::string healingItemLabel(const Item& item) {
    if (normalizeItemKey(item.getName()) == "elixir") {
        return "Elixir";
    }
    return "Health Potion";
}

bool Game::isSpireRoomName(const std::string& roomName) const {
    return roomName.find("Tower") != std::string::npos
        || roomName == "Throne Room"
        || roomName == "Rooftop Aviary"
        || roomName == "Tower Entrance";
}

bool Game::isCombatFleeBlocked(const Enemy* enemy) const {
    (void)enemy;
    if (!player || !player->getCurrentRoom()) {
        return false;
    }
    return isSpireRoomName(player->getCurrentRoom()->getName());
}

bool Game::topicIsArmorExam(const std::string& topic) {
    std::string t = toLower(trim(topic));
    return t.find("armor") != std::string::npos
        || t.find("armour") != std::string::npos
        || t.find("crest") != std::string::npos
        || t.find("plate") != std::string::npos
        || t.find("knight") != std::string::npos;
}

bool Game::isInParty(const std::string& nameFragment) const {
    std::string target = toLower(nameFragment);
    for (const auto& member : party) {
        if (toLower(member).find(target) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool Game::canReachCompanion(const std::string& nameFragment) const {
    return isInParty(nameFragment) || findNPCInCurrentRoom(nameFragment) != nullptr;
}

FellowshipMember Game::defaultFellowshipStats(const std::string& name) {
    const std::string key = toLower(name);
    if (key.find("pip") != std::string::npos) {
        return {"Pip", 50, 50, 9, 1};
    }
    if (key.find("bram") != std::string::npos) {
        return {"Bram", 70, 70, 13, 2};
    }
    if (key.find("lyra") != std::string::npos) {
        return {"Lyra", 45, 45, 8, 0};
    }
    if (key.find("valerie") != std::string::npos) {
        return {"Valerie", 60, 60, 11, 1};
    }
    if (key.find("garrison") != std::string::npos || key.find("sir") != std::string::npos) {
        return {"Sir Garrison", 80, 80, 12, 3};
    }
    return {name, 55, 55, 7, 1};
}

void Game::registerFellowshipMember(const std::string& name) {
    for (const FellowshipMember& member : fellowshipRoster) {
        if (toLower(member.name) == toLower(name)) {
            return;
        }
    }
    fellowshipRoster.push_back(defaultFellowshipStats(name));
}

void Game::healFellowshipRoster() {
    for (FellowshipMember& member : fellowshipRoster) {
        member.health = member.maxHealth;
    }
}

void Game::refreshRoomDescriptionsForRecruits() {
    if (isInParty("pip") && (findRoomByName("Abandoned Hut"))) {
        Room* hut = findRoomByName("Abandoned Hut");
        hut->setDescription(
            "A collapsed wooden house stands at the edge of the village.||"
            "The roof has caved in long ago, and beams of sunlight cut through the broken structure, illuminating drifting dust in the air.||"
            "The floor creaks beneath your steps.");
    }
    if (isInParty("lyra") && (findRoomByName("Ruined Archive"))) {
        Room* archive = findRoomByName("Ruined Archive");
        archive->setDescription(
            "A forgotten archive lies buried within the collapsing structure. Rows of towering bookshelves stretch into dim light, "
            "many of them tilted or half-collapsed, as if the room itself is slowly giving up on holding its shape.||"
            "The air is dry with age and dust. Faint sunlight leaks through cracks in the ceiling, falling across pages that have long since yellowed and curled.");
    }
    if (isInParty("bram") && (findRoomByName("Whispering Woods"))) {
        Room* forest = findRoomByName("Whispering Woods");
        forest->setDescription(
            "The moment you enter the forest, the sounds of the village disappear.||"
            "The trees twist overhead, their branches blocking the sunlight.||"
            "The wind whispers between the leaves like distant voices.||"
            "A sudden crash echoes deeper in the woods.");
    }
    if (isInParty("valerie") && (findRoomByName("Rooftop Aviary"))) {
        Room* aviary = findRoomByName("Rooftop Aviary");
        aviary->setDescription(
            "An exposed high-altitude ledge battered by violent mountain winds above the lowlands.");
    }
}

void Game::addToParty(const std::string& name, const std::string& roomToLeave) {
    party.push_back(name);
    registerFellowshipMember(name);
    Room* room = findRoomByName(roomToLeave);
    if (room) {
        room->removeNPC(name);
    }
    refreshRoomDescriptionsForRecruits();
    std::cout << "\033[1;32m" << name << " has joined your fellowship!\033[0m\n";
    printParty();
}

void Game::printParty() const {
    if (party.empty()) {
        std::cout << "Your fellowship is empty.\n";
        return;
    }
    std::cout << "\033[1;36mFellowship:\033[0m\n";
    std::cout << "  \033[1;32m" << player->getName() << "\033[0m "
              << player->getHealth() << "/" << player->getMaxHealth() << " HP\n";
    for (const FellowshipMember& member : fellowshipRoster) {
        std::cout << "  \033[1;36m" << member.name << "\033[0m "
                  << member.health << "/" << member.maxHealth << " HP\n";
    }
}

void Game::reinitializeWorldAfterLoop() {
    delete player;
    player = nullptr;
    for (auto room : rooms) {
        delete room;
    }
    rooms.clear();
    valerieNpc = nullptr;
    valerieNpcPlaced = false;

    resetStoryState();
    initializeMap();
}

void Game::respawnAfterDeath() {
    loopCount++;
    printGameOverScreen();
    std::cout << "\033[1;36mLoop #" << loopCount << " begins.\033[0m\n";
    if (loopCount >= 2) {
        std::cout << "\033[1;30m(\033[1;35mElder Cedric\033[0m\033[1;30m may remember more on this loop.)\033[0m\n";
    }
    std::cout << "\n";

    reinitializeWorldAfterLoop();
    std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
}

void Game::respawnAfterBetrayal() {
    loopCount++;
    std::cout << "\n\033[1;31m";
    std::cout << "===========================================================\n";
    std::cout << "            TWIST ENDING: THE CYCLE OF BETRAYAL            \n";
    std::cout << "===========================================================\n";
    std::cout << "\033[0m\n";
    std::cout << "The throne room falls silent.\n";
    std::cout << "Victory is within reach.\n";
    std::cout << "And then it is taken from you.\n\n";
    std::cout << "Pip does not hesitate.\n";
    std::cout << "\"...Sorry.\"\n\n";
    std::cout << "\"That's just better business.\"\n";
    std::cout << "The last thing you see is not Azael.\n\n";
    std::cout << "It is the Spire continuing without you.\n";
    std::cout << "\033[1;36mYou wake at the fountain.\033[0m\n\n";
    std::cout << "\033[1;36mAgain. Loop #" << loopCount << " begins.\033[0m\n";
    if (loopCount >= 2) {
        std::cout << "\033[1;30m(\033[1;35mElder Cedric\033[0m\033[1;30m may remember more on this loop.)\033[0m\n";
    }
    std::cout << "\n";

    endingType = EndingType::NONE;
    gameWon = false;
    reinitializeWorldAfterLoop();
    std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
}

void Game::initializeMap() {
    malakorTalkCount = 0;
    azaelTalkCount = 0;

    // 1. Create Rooms
    Room* villageSquare = new Room("Village Square",
        "The heart of the village. A crystal-clear fountain rests in the center of the square, "
        "its water flowing peacefully despite the darkness that hangs over the land. || "
        "An ancient hero statue watches silently over the villagers, its stone blade worn by centuries of forgotten battles. "
        "Strange runes are carved into the fountain's base. Some say they are warnings. "
        "Others believe they are a message left by the heroes who came before. || "
        "Nearby, !Elder Cedric! sits in faded robes beneath the statue, his eyes following every traveler who enters the square. "
        "He looks tired... but not surprised. || "
        "Beside the fountain, a !mysterious veiled woman! turns a golden ring between her fingers. "
        "She seems lost in thought, as if remembering something from a life long forgotten.");
    Room* itemShop = new Room("Item Shop",
        "A warm, cluttered storefront filled with the scent of dried herbs, polished iron, and old leather. "
        "Shelves line the walls, stocked with weapons, armor, supplies, and stoppered potions. "
        "Behind the counter, !Garrick the Merchant! carefully polishes a blade beneath a sign that reads Garrick's General Goods.");
    Room* abandonedHut = new Room("Abandoned Hut",
        "A collapsed wooden house stands at the edge of the village. The roof has caved in long ago, "
        "and beams of sunlight cut through the broken structure, illuminating drifting dust in the air. "
        "The floor creaks beneath your steps. In the shadows near the far wall, a !wiry teenager! watches you carefully.");
    Room* ruinedArchive = new Room("Ruined Archive",
        "A forgotten archive lies buried within the collapsing structure. Rows of towering bookshelves stretch into dim light, "
        "many of them tilted or half-collapsed, as if the room itself is slowly giving up on holding its shape. "
        "The air is dry with age and dust. Faint sunlight leaks through cracks in the ceiling, falling across pages that have long since yellowed and curled. "
        "A !scholarly woman! stands among the ruins.");
    Room* well = new Room("The Old Well",
        "An ancient stone well stands at the edge of the village, its cracked cobblestones covered in moss and age. "
        "The well is far older than Oakvale itself. A rusted iron barrier blocks the path beyond it, its metal surface marked with strange symbols worn away by time. "
        "The air feels colder here. From somewhere below, you hear the slow echo of dripping water.");
    Room* cave = new Room("The Well Depths",
        "A cold underground cavern stretches beneath Oakvale.||Water drips from the ceiling, echoing through the darkness.||"
        "The remains of an old rope hang from the well opening above, just out of reach.||"
        "A faint glow comes from deeper within the cave.");
    Room* deepCavern = new Room("Deep Cavern",
        "The narrow passage opens into a vast underground chamber.||"
        "Towering crystal formations glow softly from the cavern walls, casting strange light across a forgotten stone altar.||"
        "The air feels ancient, as if this place has been untouched for centuries.");
    Room* forest = new Room("Whispering Woods",
        "The moment you enter the forest, the sounds of the village disappear.||"
        "The trees twist overhead, their branches blocking the sunlight.||"
        "The wind whispers between the leaves like distant voices.||"
        "A sudden crash echoes deeper in the woods.||"
        "A !broad-shouldered smith! is backed against a tree, gripping his hammer.||"
        "He wipes blood from his cheek as he faces a small group of hostile creatures.||"
        "\"Not the best time to visit, friend!\"");
    Room* towerEntrance = new Room("Tower Entrance", "The ominous base of the black stone spire. Gargoyles peer down from the battlements above.");
    Room* towerF1 = new Room("Tower Floor 1", "A freezing stone hall littered with shattered shields and calcified skeletal remains.");
    Room* towerF2 = new Room("Tower Floor 2", "A circular inner corridor lit by bracketed green torches.");
    Room* towerF3 = new Room("Tower Floor 3", "A scorched library where ash blankets rows of destroyed spellbooks and old demon-realm secrets.");
    Room* towerF4 = new Room("Tower Floor 4", "A wide ritual hall etched with glowing crimson runes and dark sorcery seals.");
    Room* rooftopAviary = new Room("Rooftop Aviary",
        "An exposed high-altitude ledge battered by violent mountain winds above the lowlands. "
        "!Valerie! crouches on the ledge, wings shifting in the biting wind.");
    Room* throneRoom = new Room("Throne Room",
        "A staggering obsidian cathedral opens into an impossible vertical space. The ceiling is gone. "
        "Only a swirling open sky remains above the broken architecture. At the center sits a throne of jagged bone and fractured stone. "
        "It does not feel built. It feels grown. A presence fills the chamber before anything moves.");

    // Store room pointers for memory cleanup
    rooms = {
        villageSquare, itemShop, abandonedHut, ruinedArchive, well, cave, deepCavern,
        forest, towerEntrance, towerF1, towerF2, towerF3, towerF4, rooftopAviary, throneRoom
    };

    // 2. Set Exits (directional blurbs describe what lies ahead)
    villageSquare->addExit("north", well,
        "A cracked stone road leads toward the abandoned village well. The wind carries a strange whisper from the darkness below.");
    villageSquare->addExit("east", itemShop,
        "A warm lantern glows above the doorway of Garrick's Item Shop. The smell of herbs, leather, and old books drifts into the street.");
    villageSquare->addExit("south", forest,
        "A forgotten dirt path disappears into the Whispering Woods. Thick fog curls between the trees, and the villagers refuse to enter after sunset.");
    villageSquare->addExit("west", abandonedHut,
        "An overgrown trail leads toward an abandoned cottage at the edge of the village. The roof has collapsed, the windows are broken, and vines have consumed the walls.");

    itemShop->addExit("west", villageSquare,
        "To the west, the heavy oak shop door opens back out into the bright, open expanse of the village square.");

    abandonedHut->addExit("east", villageSquare,
        "To the east, the broken wooden doorway frames a clear view back out into the safety of the sunlit village square.");
    abandonedHut->addExit("north", ruinedArchive,
        "To the north, a rotting, mildew-covered interior door hangs loosely on its hinges, revealing a dark, hallway connecting to the ruins of the archive.");

    ruinedArchive->addExit("south", abandonedHut,
        "To the south, the claustrophobic rows of decaying bookshelves guide your path back toward the entrance of the abandoned hut.");

    well->addExit("south", villageSquare,
        "To the south, the wide gravel road smooths out as it leads you back into the familiar cobblestones of the village square.");
    well->addExit("east", towerEntrance,
        "To the east, the path is abruptly cut off by a massive, rusted iron gate barring the road toward the looming mountain peaks where the tower sits.");
    well->addExit("down", cave,
        "Looking down, the well bucket hangs over a pitch-black abyss, where the echo of dripping water rises from a dark cave below.");

    cave->addExit("up", well,
        "The well opening is far above.||Without something to climb with, returning will be difficult.");
    cave->addExit("east", deepCavern,
        "The narrow tunnel opens into a much larger cavern.||A strange blue glow flickers from somewhere deeper inside.");

    deepCavern->addExit("west", cave,
        "The crystal-lit chamber narrows back into the dark tunnels of the well.");

    forest->addExit("north", villageSquare,
        "The forest path leads back toward the village square.");

    towerEntrance->addExit("west", well,
        "To the west, the jagged stone path winds backward down the mountain ridge, returning toward the iron gate at the old well.");
    towerEntrance->addExit("east", towerF1,
        "To the east, the colossal black stone doors of the Spire loom over you, marking the threshold to Tower Floor 1.");

    towerF1->addExit("west", towerEntrance,
        "To the west, the heavy entrance threshold offers a view of the outside world, leading back out to the tower entrance.");
    towerF1->addExit("up", towerF2,
        "To the up, a sweeping, cracked spiral staircase of black stone winds upward into the green-lit gloom of Floor 2.");

    towerF2->addExit("down", towerF1,
        "To the down, the stone spiral staircase steps downward, descending back into the freezing quiet of the Floor 1 foyer.");
    towerF2->addExit("up", towerF3,
        "To the up, the narrow, curved corridor continues its steep incline, guiding you toward the flickering shadows of Floor 3.");

    towerF3->addExit("down", towerF2,
        "To the down, the steps curve steeply backward, leading down into the claustrophobic corridors of Floor 2.");
    towerF3->addExit("out", rooftopAviary,
        "To the out, a heavy, reinforced iron door stands slightly ajar, letting in the howling wind from the exposed rooftop aviary ledge.");
    towerF3->addExit("up", towerF4,
        "To the up, a grand set of double-doors leads directly toward the high-energy hum of Floor 4.");

    towerF4->addExit("down", towerF3,
        "To the down, a dark archway leaves the crackling magical energy behind, leading back down into the quiet ash of the scorched library.");
    towerF4->addExit("up", throneRoom,
        "To the up, a set of monolithic obsidian stairs rises majestically toward the grand, heavily reinforced doors of the Throne Room.");

    rooftopAviary->addExit("in", towerF3,
        "To the in, the stone doorway provides a safe, wind-shielded escape back into the sheltered ruins of the scorched library.");

    throneRoom->addExit("down", towerF4,
        "To the down, the grand obsidian staircase descends away from the bone throne, leading back down into the silent ruins of the lower floors.");

    // 3. Set locks (player may approach doors; entry requires the key)
    towerEntrance->setLock("GateKey",
        "You wrench at the black iron doors, but the demonic seal holds them shut. You need the gate key.");
    throneRoom->setLock("ThroneKey",
        "You push against the bone-carved throne doors, but the sigil flares and refuses you. You need the throne key.");

    // 4. Create and Place Items
    Item rustySword("RustySword", "An old, corroded iron sword. Better than nothing.", ItemType::WEAPON, 2, 5);
    Item steelSword("SteelSword", "A well-forged steel longsword with a sharp edge.", ItemType::WEAPON, 6, 30);
    
    Item leatherArmor("LeatherArmor", "A set of sturdy leather tunic and leggings.", ItemType::ARMOR, 2, 15);
    Item steelPlate("SteelPlate", "Heavy iron plate armor. Highly defensive.", ItemType::ARMOR, 5, 45);

    Item healthPotion("HealthPotion", "A vial filled with shimmering red liquid. Restores 40 HP.", ItemType::CONSUMABLE, 40, 10);
    Item elixir("Elixir", "An exquisite glowing gold draft. Restores 100 HP.", ItemType::CONSUMABLE, 100, 25);

    Item gateKey("GateKey", "A heavy brass key shaped like a dragon's wing.", ItemType::KEY, 0, 0);
    Item rope("Rope", "A long coil of hempen rope, sturdy enough to lower yourself into the well.", ItemType::KEY, 0, 5);
    Item starlightRibbon("StarlightRibbon", "A delicate ribbon that glimmers with pale moonlight.", ItemType::VALUABLE, 0, 20);
    Item ore("Ore", "A chunk of rare forge ore suitable for master armor work.", ItemType::VALUABLE, 0, 25);
    Item bread("Bread", "A stale loaf. Simple food, but it could still help someone hungry.", ItemType::CONSUMABLE, 10, 2);
    Item shieldOfValiance("ShieldOfValiance", "A knight's shield engraved with a forgotten royal crest.", ItemType::ARMOR, 7, 60);
    Item azaelsDiary("AzaelsDiary", "A worn journal revealing Azael's tragic past and lost humanity.", ItemType::KEY, 0, 0);

    // Hidden items (use search to reveal)
    abandonedHut->addHiddenItem(rustySword, "a corroded blade half-buried in rubble");
    abandonedHut->addHiddenItem(gateKey, "a brass key glinting beneath broken floorboards");
    ruinedArchive->addHiddenItem(starlightRibbon, "a pale ribbon tucked between mouldy pages");
    towerF3->addHiddenItem(azaelsDiary, "a charred journal wedged between ash-black tomes");
    
    // 5. Create and Place NPCs
    // Shopkeeper NPC
    NPC* merchant = new NPC("Garrick the Merchant",
        "A broad-shouldered man stands behind the counter, carefully polishing a blade. A small sign reads: \"Garrick's General Goods\".",
        true);
    merchant->addDialogue("Welcome, traveler. Heading toward the Spire, are you? Most adventurers come here searching for something that will keep them alive.");
    merchant->addDialogue("I've fresh bread too, if you plan to win hearts as well as battles.");
    merchant->addDialogue("The Demon Lord in the tower is fearsome. Do not face him unprepared!");
    merchant->addDialogue("Steel and sense keep people alive. Heroics without gear are just expensive funerals.");
    merchant->addDialogue("If you return from the tower breathing, I'll call it a discount day.");
    merchant->addShopItem(steelSword);
    merchant->addShopItem(leatherArmor);
    merchant->addShopItem(steelPlate);
    merchant->addShopItem(healthPotion);
    merchant->addShopItem(healthPotion);
    merchant->addShopItem(healthPotion);
    merchant->addShopItem(healthPotion);
    merchant->addShopItem(healthPotion);
    merchant->addShopItem(bread);
    merchant->addShopItem(bread);
    merchant->addShopItem(bread);
    itemShop->setNPC(merchant);

    // Village NPCs
    NPC* elder = new NPC("Elder Cedric",
        "An elderly man in weathered robes watches the square with tired, knowing eyes.");
    if (loopCount >= 5) {
        elder->addDialogue("...So you have found your way back again.");
        elder->addDialogue("The Spire does not always yield to the strongest hand. Some doors remember those who came before.");
        elder->addDialogue("You have faced the Demon Lord's throne before, haven't you? Yet you still look at him as an enemy before asking what created him.");
        elder->addDialogue("Azael was not always what he is now. What you choose to show him may decide what remains when the battle ends.");
        elder->addDialogue("The Spire has more than one road to its summit. The foolish climb where they are expected to.");
        elder->addDialogue("Some paths are hidden because they require trust, not strength.");
        elder->addDialogue("The greatest obstacles in the Spire do not always need to be defeated.");
        elder->addDialogue("Remember this, hero... a blade can open a path forward, but it cannot open every door.");
    } else if (loopCount >= 2) {
        elder->addDialogue("Ah... you have returned.");
        elder->addDialogue("The climb will test more than your strength. The Demon Lord is not the only thing waiting for you.");
        elder->addDialogue("Do not ignore what feels familiar, hero. Sometimes the world repeats its mistakes before we notice.");
        elder->addDialogue("Some places are abandoned for a reason, hero. Do not mistake an old door for an empty one.");
        elder->addDialogue("The village well is older than the stones around it. If you choose to descend... make sure you can climb back out.");
    } else {
        elder->addDialogue("Greetings, brave soul. The Demon Lord has plagued our lands for centuries. The path ahead is dangerous, but the village believes you can succeed.");
        elder->addDialogue("Steel your heart, hero. Many have climbed the Spire before you... though few have returned.");
        elder->addDialogue("Run along now. Destiny waits for no one.");
    }
    villageSquare->setNPC(elder);

    NPC* eleanor = new NPC("Lady Eleanor",
        "A veiled woman sits near the fountain, absently turning a simple gold ring in her hands.");
    eleanor->addDialogue("I still wait for my husband... even after all this time.");
    eleanor->addDialogue("His crest was a lion surrounded by thorns. He told me it meant a warrior who protects what he loves, even when the world turns against him.");
    eleanor->addDialogue("When he left for the Spire, he promised me he would come home. But the Demon Lord's curse has taken many things from those who enter... Memories. Names. Even the faces of the people they love.");
    eleanor->addTalkAlias("woman");
    eleanor->addTalkAlias("veiled woman");
    villageSquare->addNPC(eleanor);

    NPC* pip = new NPC("Pip",
        "In the shadows near the far wall, a wiry teenager watches you carefully.");
    pip->addDialogue("Coin, bread, praise... everyone wants something from me.");
    pip->addDialogue("You got gold? Then say it straight. If you want me around, pay me proper. I'm not your charity case.");
    pip->addDialogue("You got food? Real food? Not scraps. Bread works. Share it, don't wave it around. Or just talk. Honestly. Most people don't bother.");
    pip->addDialogue("Name's Pip. Don't forget it.");
    abandonedHut->setNPC(pip);

    NPC* lyra = new NPC("Lyra",
        "A scholarly woman stands among the ruins.");
    lyra->addDialogue("The fountain's runes and these shelves speak the same truth. Just in different forms.");
    lyra->addDialogue("This archive does not stay silent. It only waits for someone patient enough to hear it again.");
    lyra->addDialogue("Riddles are not puzzles meant to block you. They are doors disguised as questions.");
    lyra->addDialogue("Most people look for answers in steel. But some answers only exist in understanding.");
    ruinedArchive->setNPC(lyra);

    NPC* bram = new NPC("Bram",
        "A broad-shouldered smith wipes soot from his hands, a hammer resting within easy reach.");
    bram->addDialogue("You swing steel, I shape it. Bring me ore, and I'll show you real armor.");
    bram->addDialogue("I can patch dents all day, but rare ore makes legends.");
    bram->addDialogue("If you're marching on the Spire, don't march in scrap.");
    forest->setNPC(bram);

    valerieNpc = new NPC("Valerie",
        "A winged figure crouches on the high ledge, feathers lifting in the biting mountain wind.");
    valerieNpc->addDialogue("This sky is mine. Give me one reason not to throw you off this ledge.");
    valerieNpc->addDialogue("Winds up here judge truth faster than people do.");
    valerieNpc->addDialogue("Bring me something beautiful, and I might listen.");
    valerieNpcPlaced = false;
    Enemy* valerieHostile = new Enemy("Valerie", 60, 14, 4, 25);
    rooftopAviary->setEnemy(valerieHostile);
    rooftopAviary->setHideExitsWhileGuard(true);

    // 6. Create and Place Enemies
    // Forest: Goblin Scout
    Enemy* goblin = new Enemy("Goblin Scout", 30, 8, 1, 15, healthPotion);
    forest->setEnemy(goblin);

    // Cave: Cave Bat (drops Rope)
    Enemy* bat = new Enemy("Giant Cave Bat", 25, 6, 0, 10, rope);
    cave->setEnemy(bat);

    // Deep Cavern: Cave Troll (drops Ore)
    Enemy* troll = new Enemy("Cave Troll", 65, 12, 3, 30, ore);
    deepCavern->setEnemy(troll);

    // Tower Floor 1: Armored Knight (reveals identity as Sir Garrison when charmed)
    Enemy* garrison = new Enemy("Armored Knight", 70, 14, 4, 35, shieldOfValiance, true);
    towerF1->setEnemy(garrison);
    towerF1->setHideExitsWhileGuard(true);

    // Tower Floor 2: Demon Hound
    Enemy* hound = new Enemy("Demon Hound", 55, 13, 2, 25, elixir);
    towerF2->setEnemy(hound);
    towerF2->setHideExitsWhileGuard(true);

    // Tower Floor 4: Demon General (drops ThroneKey)
    Item throneKey("ThroneKey", "A dark obsidian key that feels ice-cold to the touch.", ItemType::KEY, 0, 0);
    Enemy* general = new Enemy("Demon General Malakor", 110, 19, 6, 55, throneKey, true);
    towerF4->setEnemy(general);
    towerF4->setHideExitsWhileGuard(true);

    // Throne Room: Demon Lord (Final Boss)
    Enemy* demonLord = new Enemy("Demon Lord Azael", 150, 22, 6, 100, true);
    throneRoom->setEnemy(demonLord);
    throneRoom->setHideExitsWhileGuard(true);

    // 7. Setup Player
    if (player) {
        delete player;
    }
    player = new Player("Hero", 100, 8, 1);
    player->setCurrentRoom(villageSquare);
}

void Game::printTitle() const {
    std::cout << "\033[1;35m";
    std::cout << "============================================================\n";
    std::cout << "                     THE SPIRE OF AZAEL                     \n";
    std::cout << "============================================================\n";
    std::cout << "                 A dark fantasy text adventure              \n";
    std::cout << "============================================================\n";
    std::cout << "\033[0m\n";
}

void Game::printWelcome() const {
    printTitle();
    if (loopCount >= 2) {
        std::cout << "\033[1;30mTimeline loop #" << loopCount
                  << " -- you remember dying before.\033[0m\n\n";
    }
    std::cout << "  Welcome, young adventurer.\n\n";
    std::cout << "  Long ago, the northern tower stood as a beacon of hope.\n";
    std::cout << "  Now, it has become a monument of despair.\n\n";
    std::cout << "  Since the Demon Lord Azael claimed the Spire, darkness has spread\n";
    std::cout << "  across the kingdom. Villages have fallen silent, roads have become\n";
    std::cout << "  dangerous, and whispers speak of a curse that refuses to die.\n\n";
    std::cout << "  But every legend begins with a single step.\n\n";
    std::cout << "\033[1;36mSYSTEM TUTORIAL\033[0m\n";
    std::cout << "  * Type \033[1;32mhelp\033[0m to view available commands.\n";
    std::cout << "  * \033[1;32mquit\033[0m, \033[1;32mexit\033[0m, or \033[1;32mx\033[0m to leave the adventure.\n";
    std::cout << "  * The path ahead is yours to choose.\n\n";
    std::cout << "  Climb the Spire...\n\n";
    std::cout << "  and uncover the truth behind Azael.\n";
    std::cout << "------------------------------------------------------------\n\n";
}

void Game::printHelp() const {
    std::cout << "\033[1;36m=== CORE COMMANDS ===\033[0m\n";
    std::cout << "  \033[1;32mlook\033[0m or \033[1;32ml\033[0m               : Examine the current room.\n";
    std::cout << "  \033[1;32msearch\033[0m                 : Search the area for hidden objects.\n";
    std::cout << "  \033[1;32mlook <item>\033[0m            : Inspect a specific item in your bag.\n";
    std::cout << "  \033[1;32mgo <direction>\033[0m or \033[1;32m<dir>\033[0m : Move (north/n, south/s, east/e, west/w, up/u, down/d).\n";
    std::cout << "  \033[1;32mtake <item>\033[0m or \033[1;32mget\033[0m     : Pick up an item from the current room.\n";
    std::cout << "  \033[1;32mpickup loot\033[0m           : Search a fallen foe and claim what they dropped.\n";
    std::cout << "  \033[1;32mdrop <item>\033[0m            : Drop an item from your inventory.\n";
    std::cout << "  \033[1;32minventory\033[0m or \033[1;32minv\033[0m or \033[1;32mi\033[0m   : View your inventory and equipped gear.\n";
    std::cout << "  \033[1;32mstats\033[0m                  : View player level, HP, attack, and defense.\n";
    std::cout << "  \033[1;32muse <item>\033[0m             : Drink a healing potion or use consumables.\n";
    std::cout << "  \033[1;32mequip <item>\033[0m           : Equip a weapon or armor.\n";
    std::cout << "  \033[1;32mtalk\033[0m                   : Talk to the NPC in the room.\n";
    std::cout << "  \033[1;32mtalk <name>\033[0m            : Talk to a specific NPC in a room with multiple people.\n";
    std::cout << "  \033[1;32mbuy <item>\033[0m             : Buy an item from a merchant.\n";
    std::cout << "  \033[1;32msell <item>\033[0m            : Sell an item (shows payout first).\n";
    std::cout << "  \033[1;32mwork\033[0m                   : Earn gold in the Village Square (not during combat).\n";
    std::cout << "  \033[1;32mrest\033[0m                   : Rest in the Village Square; you recover, fellowship included.\n";
    std::cout << "  \033[1;32mparty\033[0m                  : View recruited companions.\n";
    std::cout << "  \033[1;32mhelp\033[0m or \033[1;32mh\033[0m              : Display this help message.\n";
    std::cout << "  \033[1;32mquit\033[0m, \033[1;32mq\033[0m, \033[1;32mexit\033[0m, \033[1;32mx\033[0m : Leave the game.\n";
    std::cout << "\033[1;36m=====================\033[0m\n\n";

    bool hasSpecial = specialGiveUnlocked || specialPraiseUnlocked
        || specialExamineUnlocked || specialRecruitUnlocked || player->hasItem("AzaelsDiary") || lyraRecruited;
    if (!hasSpecial) {
        return;
    }

    std::cout << "\033[1;35m=== SPECIAL STORY COMMANDS ===\033[0m\n";
    if (specialGiveUnlocked) {
        std::cout << "  \033[1;32mgive <item> <name>\033[0m     : Give an item to an NPC.\n";
        std::cout << "  \033[1;32mbribe pip\033[0m              : Pay Pip 10 gold to secure her blade.\n";
    }
    if (player->hasItem("AzaelsDiary")) {
        std::cout << "  \033[1;32mread diary\033[0m             : Read Azael's Diary from your pack.\n";
    }
    if (specialPraiseUnlocked) {
        std::cout << "  \033[1;32mpraise <name>\033[0m          : Build trust with specific companions.\n";
    }
    if (specialExamineUnlocked) {
        std::cout << "  \033[1;32mexamine <topic>\033[0m        : Examine something in detail.\n";
    }
    if (specialRecruitUnlocked) {
        std::cout << "  \033[1;32mrecruit <name>\033[0m         : Invite a companion into your party.\n";
    }
    std::cout << "\033[1;35m===============================\033[0m\n\n";
}

bool Game::isPeacefulWorthItAnswer(const std::string& line) {
    const std::string s = toLower(trim(line));
    if (s.empty()) {
        return false;
    }
    static const char* peacefulWords[] = {
        "yes", "worth", "peace", "trust", "hope", "mercy", "glad", "good",
        "always", "enough", "calm", "friend", "believe", "thank", "grateful",
        "beautiful", "right", "saved", "love", "kind"
    };
    for (const char* word : peacefulWords) {
        if (s.find(word) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void Game::playRedemptionOakvaleEpilogue() {
    std::cout << "\n\033[1;36m[Village Square]\033[0m\n\n";
    std::cout << "The fountain still runs.\n\n";
    std::cout << "Children move through the streets as if nothing ever changed.\n\n";
    std::cout << "But something in the air feels different.\n\n";
    std::cout << "Quieter.\n\n";
    std::cout << "Like the world has stopped holding its breath.\n\n";
    std::cout << "\033[1;35mElder Cedric\033[0m stands by the fountain.\n\n";
    std::cout << "He does not look surprised to see you.\n\n";
    std::cout << "\"...So you returned.\"\n\n";
    std::cout << "A pause.\n\n";
    std::cout << "He studies you carefully.\n\n";
    std::cout << "\"...And the Spire still stands.\"\n\n";
    std::cout << "He nods slowly.\n\n";
    std::cout << "\"I suppose that means it was never the problem people thought it was.\"\n\n";
    std::cout << "Another pause.\n\n";
    std::cout << "\033[1;35mElder Cedric\033[0m exhales.\n\n";
    std::cout << "\"You did not bring us a kingdom of ashes.\"\n\n";
    std::cout << "\"You brought back a world that did not need saving through ruin.\"\n\n";
    std::cout << "\033[1;35mElder Cedric\033[0m looks at the fountain.\n\n";
    std::cout << "\"...Tell me, hero.\"\n\n";
    std::cout << "\"Was it worth it?\"\n\n";
    std::cout << "\033[1;34m>\033[0m ";

    std::string answer;
    if (!std::getline(std::cin, answer)) {
        answer.clear();
    }

    if (isPeacefulWorthItAnswer(answer)) {
        std::cout << "\nYou answer.\n\n";
        std::cout << "\033[1;35mElder Cedric\033[0m closes his eyes for a moment.\n\n";
        std::cout << "\"...Good.\"\n\n";
        std::cout << "\"Then perhaps we taught the Spire something it did not know before.\"\n\n";
        std::cout << "\033[1;35mElder Cedric\033[0m turns back toward the village.\n\n";
        std::cout << "\"...Rest, then.\"\n\n";
        std::cout << "\"You've earned the right to stay in one place for a while.\"\n\n";
        std::cout << "The fountain continues to flow.\n\n";
        std::cout << "Oakvale continues to live.\n\n";
        std::cout << "And somewhere far away...\n\n";
        std::cout << "The Spire remains standing.\n\n";
        std::cout << "Not as a threat.\n\n";
        std::cout << "But as a memory of what was almost lost.\n\n";
    } else {
        std::cout << "\nYou hesitate, then speak honestly.\n\n";
        std::cout << "\033[1;35mElder Cedric\033[0m listens without interrupting.\n\n";
        std::cout << "\"...Then carry that truth gently,\" he says at last.\n\n";
        std::cout << "\"The village will not ask you to be a legend every morning.\"\n\n";
        std::cout << "The fountain continues to flow.\n\n";
        std::cout << "Oakvale continues to live.\n\n";
        std::cout << "And somewhere far away, the Spire remains standing - not as a threat, but as a memory.\n\n";
    }
}

void Game::playEndingEpilogue() {
    if (endingType == EndingType::REDEMPTION_ENDING) {
        std::cout << "\n\033[1;36m";
        std::cout << "===========================================================\n";
        std::cout << "         SECRET ENDING: REDEMPTION OF THE TYRANT           \n";
        std::cout << "===========================================================\n";
        std::cout << "\033[0m\n";
        playRedemptionOakvaleEpilogue();
        return;
    }

    if (endingType == EndingType::TRUE_ENDING) {
        std::cout << "\n\033[1;32m";
        std::cout << "===========================================================\n";
        std::cout << "          TRUE ENDING: THE FELLOWSHIP ASCENDANT            \n";
        std::cout << "===========================================================\n";
        std::cout << "\033[0m\n";
        std::cout << "Azael falls in battle.\n\n";
        std::cout << "But no one with you is lost.\n\n";
        std::cout << "The Spire does not collapse.\n\n";
        std::cout << "It quiets.\n\n";
        std::cout << "You return to Oakvale together.\n\n";
        std::cout << "Alive.\n\n";
        std::cout << "Unbroken.\n\n";
        std::cout << "\033[1;36m[Village Square]\033[0m\n\n";
        std::cout << "\033[1;35mElder Cedric\033[0m bows his head as you arrive.\n\n";
        std::cout << "\"...So it was not strength alone that carried you.\"\n\n";
        std::cout << "\"It was trust.\"\n\n";
        std::cout << "The village does not call you conqueror.\n\n";
        std::cout << "It calls you witness.\n\n";
        return;
    }

    if (endingType == EndingType::BITTERSWEET_ENDING) {
        std::cout << "\n\033[1;33m";
        std::cout << "===========================================================\n";
        std::cout << "            BITTERSWEET ENDING: COST OF VICTORY            \n";
        std::cout << "===========================================================\n";
        std::cout << "\033[0m\n";
        std::cout << "Azael is defeated.\n\n";
        std::cout << "But the hall does not feel like victory.\n\n";
        std::cout << "Someone does not return to the village.\n\n";
        std::cout << "\033[1;36m[Village Square]\033[0m\n\n";
        std::cout << "The fountain runs as it always has.\n\n";
        std::cout << "But it is quieter now.\n\n";
        std::cout << "\033[1;35mElder Cedric\033[0m does not speak for a long time.\n\n";
        std::cout << "\"...You saved the world.\"\n\n";
        std::cout << "A pause.\n\n";
        std::cout << "\"...But not yourself.\"\n\n";
        std::cout << "The Spire stands unchanged.\n\n";
        std::cout << "Only you remember what it took.\n\n";
        return;
    }

    std::cout << "\n\033[1;32m";
    std::cout << "===========================================================\n";
    std::cout << "                 VICTORY! DEMON LORD SLAIN                 \n";
    std::cout << "===========================================================\n";
    std::cout << "\033[0m";
    std::cout << "With a final mighty strike, you vanquish the Demon Lord Azael!\n";
    std::cout << "His dark energy dissipates into the air, and sunlight breaks through the clouds.\n";
    std::cout << "The village of Oakvale is saved, and you will forever be remembered as its hero!\n\n";
}

void Game::printCreditsScreen() const {
    std::cout << "\033[1;36m==================== CREDITS ====================\033[0m\n";
    std::cout << "  THE SPIRE OF AZAEL\n";
    std::cout << "  A dark fantasy text adventure\n\n";
    std::cout << "  Design & Implementation: Cindy Reginia Wang\n";
    std::cout << "  Built with C++\n\n";
    std::cout << "  Thank you for playing!\n";
    std::cout << "\033[1;36m=================================================\033[0m\n\n";
    std::cout << "Type \033[1;32mexit\033[0m, \033[1;32mquit\033[0m, or \033[1;32mx\033[0m to close the game.\n\n";
}

bool Game::processExitCommand(const std::string& line) const {
    std::string cmd = toLower(trim(line));
    return cmd == "quit" || cmd == "q" || cmd == "exit" || cmd == "x"
        || cmd == "exit game" || cmd == "leave";
}

void Game::waitForExitCommand() {
    std::string line;
    while (true) {
        std::cout << "\033[1;34m>\033[0m ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (processExitCommand(line)) {
            std::cout << "Farewell, adventurer.\n";
            break;
        }
        if (!trim(line).empty()) {
            std::cout << "The tale is over. Type \033[1;32mexit\033[0m or \033[1;32mquit\033[0m to close.\n";
        }
    }
}

void Game::printGameOverScreen() const {
    std::cout << "\n\033[1;36m";
    std::cout << "The Spire rejects you.\n";
    std::cout << "Your strength was not enough.\n\n";
    std::cout << "Or your path was not ready.\n";
    std::cout << "You fall.\n";
    std::cout << "And the fountain greets you once more.\n";
    std::cout << "\033[0m\n";
}

void Game::start() {
    initializeMap();
    printWelcome();
    isRunning = true;

    // Describe initial room
    std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
    std::cout.flush();

    std::string line;
    while (isRunning && !gameWon) {
        if (applyQueuedSnapshot()) {
            std::cout << "\033[1;36mSave loaded. Your journey continues from the stored moment.\033[0m\n\n";
            std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
        }

        if (!player->isAlive()) {
            respawnAfterDeath();
            continue;
        }
        // Combat trigger: If there is a living enemy in the current room, enter combat immediately!
        Room* currentRoom = player->getCurrentRoom();
        if (currentRoom->getEnemy() && currentRoom->getEnemy()->isAlive()) {
            if (currentRoom->getName() == "Tower Floor 1" && isArmoredKnightEnemy(currentRoom->getEnemy())) {
                std::cout << "\033[1;36mAn armored knight blocks the hall. Attack, speak to him, or offer him something.\033[0m\n";
            } else if (currentRoom->getName() == "Throne Room") {
                std::cout << "\033[1;36mThe Demon Lord holds the throne hall. There is no escape. Attack when you are ready.\033[0m\n";
            } else if (isSpireRoomName(currentRoom->getName()) || currentRoom->getName() == "Rooftop Aviary") {
                startCombat(currentRoom->getEnemy());
                if (gameWon) break;
                if (!player->isAlive()) continue;
            } else {
                startCombat(currentRoom->getEnemy());
                if (gameWon) break;
                if (!player->isAlive()) continue;
            }
        }

        std::cout << "\033[1;34m>\033[0m ";
        if (!std::getline(std::cin, line)) {
            break; // EOF
        }

        line = trim(line);
        if (line.empty()) continue;

        // Parse command and argument, with natural-language normalization.
        std::string normalized = toLower(line);
        std::string verb;
        std::string arg;

        if (startsWith(normalized, "talk to ")) {
            verb = "talk";
            arg = trim(line.substr(8));
        } else if (startsWith(normalized, "speak to ")) {
            verb = "talk";
            arg = trim(line.substr(9));
        } else if (startsWith(normalized, "speak with ")) {
            verb = "talk";
            arg = trim(line.substr(11));
        } else if (startsWith(normalized, "go to ")) {
            verb = "go";
            arg = trim(line.substr(6));
        } else if (startsWith(normalized, "move to ")) {
            verb = "go";
            arg = trim(line.substr(8));
        } else if (startsWith(normalized, "walk to ")) {
            verb = "go";
            arg = trim(line.substr(8));
        } else if (startsWith(normalized, "head to ")) {
            verb = "go";
            arg = trim(line.substr(8));
        } else if (startsWith(normalized, "pick up ")) {
            verb = "take";
            arg = trim(line.substr(8));
        } else if (startsWith(normalized, "look at ")) {
            verb = "look";
            arg = trim(line.substr(8));
        } else if (startsWith(normalized, "check ")) {
            verb = "look";
            arg = trim(line.substr(6));
        } else if (startsWith(normalized, "show me ")) {
            verb = "show";
            arg = trim(line.substr(8));
        } else if (normalized == "pickup loot" || normalized == "pick up loot" || normalized == "loot") {
            verb = "pickup";
            arg = "loot";
        } else {
            size_t spacePos = line.find(' ');
            if (spacePos == std::string::npos) {
                verb = toLower(line);
            } else {
                verb = toLower(line.substr(0, spacePos));
                arg = trim(line.substr(spacePos + 1));
            }
        }

        // Single-word synonyms.
        if (verb == "speak" || verb == "chat" || verb == "converse") verb = "talk";
        else if (verb == "move" || verb == "walk" || verb == "travel" || verb == "head") verb = "go";
        else if (verb == "grab") verb = "take";
        else if (verb == "pickup" && arg != "loot") verb = "take";
        else if (verb == "inspect" && arg.empty()) verb = "search";
        else if (verb == "check" && arg.empty()) verb = "search";
        else if (verb == "inspect" || verb == "check") verb = "look";
        else if (verb == "bag") verb = "inventory";
        else if (verb == "attack" && arg == "azael") verb = "attack";

        // Typo correction for common verbs.
        const std::vector<std::string> validVerbs = {
            "north", "south", "east", "west", "up", "down", "in", "out",
            "n", "s", "e", "w", "u", "d", "l", "i", "inv", "h", "q",
            "go", "look", "take", "get", "drop", "inventory", "stats",
            "use", "equip", "attack", "talk", "buy", "sell", "help", "quit", "exit",
            "give", "praise", "recruit", "show", "read", "bribe", "examine", "search",
            "work", "rest", "party", "pickup", "loot"
        };
        std::string correctedVerb = closestWord(verb, validVerbs);
        if (!correctedVerb.empty() && correctedVerb != verb) {
            std::cout << "\033[1;30m(autocorrected '" << verb << "' -> '" << correctedVerb << "')\033[0m\n";
            verb = correctedVerb;
        }

        // Typo correction for movement directions.
        if (verb == "go" || verb == "n" || verb == "s" || verb == "e" || verb == "w" || verb == "u" || verb == "d") {
            const std::vector<std::string> dirs = {"north", "south", "east", "west", "up", "down", "in", "out", "n", "s", "e", "w", "u", "d"};
            std::string loweredArg = toLower(trim(arg));
            if (!loweredArg.empty()) {
                std::string correctedDir = closestWord(loweredArg, dirs);
                if (!correctedDir.empty() && correctedDir != loweredArg) {
                    std::cout << "\033[1;30m(autocorrected direction '" << loweredArg << "' -> '" << correctedDir << "')\033[0m\n";
                    arg = correctedDir;
                }
            }
        }

        // Direct single-letter shortcuts
        if (verb == "n" || verb == "north") { handleGo("north"); }
        else if (verb == "s" || verb == "south") { handleGo("south"); }
        else if (verb == "e" || verb == "east") { handleGo("east"); }
        else if (verb == "w" || verb == "west") { handleGo("west"); }
        else if (verb == "u" || verb == "up") { handleGo("up"); }
        else if (verb == "d" || verb == "down") { handleGo("down"); }
        else if (verb == "out") { handleGo("out"); }
        else if (verb == "in") { handleGo("in"); }
        else if (verb == "l" && arg.empty()) { handleLook(""); }
        else if (verb == "i" || verb == "inv") { player->showInventory(); }
        else if (verb == "h") { printHelp(); }
        else if (verb == "q" || verb == "exit" || verb == "x") {
            isRunning = false;
            std::cout << "Goodbye!\n";
        }
        // Standard commands
        else if (verb == "go") { handleGo(arg); }
        else if (verb == "look") { handleLook(arg); }
        else if (verb == "take" || verb == "get") { handleTake(arg); }
        else if (verb == "drop") { handleDrop(arg); }
        else if (verb == "inventory") { player->showInventory(); }
        else if (verb == "stats") { printPlayerStats(); }
        else if (verb == "read") { handleRead(arg); }
        else if (verb == "bribe") { tryBribePip(arg); }
        else if (verb == "use") { handleUse(arg); }
        else if (verb == "equip") { handleEquip(arg); }
        else if (verb == "attack" || verb == "a") {
            Enemy* enemy = player->getCurrentRoom()->getEnemy();
            if (enemy && enemy->isAlive()) {
                startCombat(enemy);
            } else {
                std::cout << "There is nothing here to attack.\n";
            }
        }
        else if (verb == "talk") {
            handleTalk(arg);
        }
        else if (verb == "buy") { handleBuy(arg); }
        else if (verb == "sell") { handleSell(arg); }
        else if (verb == "give") { handleGive(arg); }
        else if (verb == "praise") { handlePraise(arg); }
        else if (verb == "show") { handleShow(arg); }
        else if (verb == "examine") { handleExamine(arg); }
        else if (verb == "search") { handleSearch(arg); }
        else if (verb == "pickup" || (verb == "loot" && arg.empty())) { handlePickupLoot(); }
        else if (verb == "recruit") { handleRecruit(arg); }
        else if (verb == "work") { handleWork(); }
        else if (verb == "rest") { handleRest(); }
        else if (verb == "party") { printParty(); }
        else if (verb == "help") { printHelp(); }
        else if (verb == "quit") { isRunning = false; std::cout << "Goodbye!\n"; }
        else {
            std::string suggestion = closestWord(verb, validVerbs, 3);
            if (!suggestion.empty()) {
                std::cout << "I don't understand that command. Did you mean \033[1;32m'" << suggestion << "'\033[0m?\n";
            } else {
                std::cout << "I don't understand that command. Type 'help' for options.\n";
            }
        }
        std::cout << "\n";
    }

    if (gameWon) {
        playEndingEpilogue();
        printCreditsScreen();
        waitForExitCommand();
    }
}

void Game::handleLook(const std::string& arg) {
    if (arg.empty()) {
        std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
    } else {
        std::string topic = toLower(trim(arg));
        if (topic.find("fountain") != std::string::npos || topicIsArmorExam(topic)) {
            handleExamine(arg);
            return;
        }

        Item item;
        // Search inventory
        if (player->getItem(arg, item)) {
            std::cout << "\033[1;33m" << item.getName() << "\033[0m (In Inventory):\n"
                      << item.getDescription() << "\n";
            if (item.getGoldValue() > 0) {
                std::cout << "Estimated value: " << item.getGoldValue() << "g\n";
            }
        }
        else if (player->getCurrentRoom()->findItem(arg, item)) {
            std::cout << item.getDescription() << "\n";
        } else {
            std::cout << "You don't see any '" << arg << "' here.\n";
        }
    }
}

void Game::handleGo(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Go where? (north, south, east, west, up, down)\n";
        return;
    }

    std::string direction = toLower(arg);
    Room* nextRoom = player->getCurrentRoom()->getExit(direction);

    if (!nextRoom) {
        std::cout << "You can't go that way.\n";
        return;
    }

    Room* current = player->getCurrentRoom();
    if (current && (isSpireRoomName(current->getName()) || current->getName() == "Rooftop Aviary")) {
        Enemy* guard = current->getEnemy();
        if (guard && guard->isAlive()) {
            if (isArmoredKnightEnemy(guard)) {
                std::cout << "\033[1;31mThe armored knight bars every exit. Deal with him first.\033[0m\n";
                return;
            }
            if (current->getName() == "Throne Room") {
                std::cout << "\033[1;31mThe Demon Lord seals the hall. You cannot leave until this is settled.\033[0m\n";
                return;
            }
            std::cout << "\033[1;31mThe guard blocks your path. You must fight to advance!\033[0m\n";
            startCombat(guard);
            return;
        }
    }

    // Locked doors: you can approach them, but cannot enter without the key
    if (nextRoom->getIsLocked()) {
        std::string keyReq = nextRoom->getKeyRequired();
        std::cout << "You try the " << direction << "ward door into " << nextRoom->getName() << "...\n";
        if (player->hasItem(keyReq)) {
            nextRoom->unlock(keyReq);
            std::cout << "\033[1;32mThe lock yields. You open the way using your " << keyReq << ".\033[0m\n";
        } else {
            std::cout << "\033[1;31m" << nextRoom->getLockMessage() << "\033[0m\n";
            return;
        }
    }

    // Climbing up from the well depths
    if (direction == "up" && current && current->getName() == "The Well Depths"
        && nextRoom->getName() == "The Old Well") {
        if (player->hasItem("Rope")) {
            std::cout << "You secure the rope and begin climbing.\n\n";
            std::cout << "The ascent is slow, but safe.\n\n";
            std::cout << "You climb out of the well.\n";
        } else {
            std::cout << "You look up toward the well opening.\n\n";
            std::cout << "The climb is far steeper than it appeared from above.\n\n";
            std::cout << "You grip the old stones and begin pulling yourself upward.\n\n";
            std::cout << "The loose rocks crumble beneath your hands.\n\n";
            std::cout << "You take \033[1;31m5 damage\033[0m, but you manage to climb back out of the well.\n";
            player->takeDamage(5);
            std::cout << "Current HP: \033[1;32m" << player->getHealth() << "/" << player->getMaxHealth() << "\033[0m\n";
            if (!player->isAlive()) {
                std::cout << "\033[1;31mYou lose your grip and fall back into the darkness...\033[0m\n";
                return;
            }
        }
        player->setCurrentRoom(nextRoom);
        lastTalkNpcName.clear();
        std::cout << "\n" << nextRoom->getDetailedDescription() << "\n";
        return;
    }

    // Descending into the well depths
    if (direction == "down" && current && current->getName() == "The Old Well"
        && nextRoom->getName() == "The Well Depths") {
        if (player->hasItem("Rope")) {
            std::cout << "You secure the rope around the edge of the well.\n\n";
            std::cout << "The descent is slow, but controlled.\n\n";
            std::cout << "You climb down safely.\n";
        } else {
            std::cout << "You lower yourself into the darkness.\n\n";
            std::cout << "The old stones crumble beneath your hands.\n\n";
            std::cout << "You lose your grip.\n\n";
            std::cout << "You fall.\n\n";
            std::cout << "You crash onto the damp stone floor below.\n\n";
            player->takeDamage(20);
            std::cout << "You took \033[1;31m20 damage\033[0m.\n";
            std::cout << "Current HP: \033[1;32m" << player->getHealth() << "/" << player->getMaxHealth() << "\033[0m\n";
            if (!player->isAlive()) {
                std::cout << "\033[1;31mYou black out from the fall...\033[0m\n";
                return;
            }
            std::cout << "\nYou stand up slowly and look around.\n";
        }
        player->setCurrentRoom(nextRoom);
        lastTalkNpcName.clear();
        std::cout << "\n" << nextRoom->getDetailedDescription() << "\n";
        return;
    }

    // Move player
    player->setCurrentRoom(nextRoom);
    lastTalkNpcName.clear();
    std::cout << "You travel " << direction << ".\n\n";
    std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
}

void Game::handleTake(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Take what?\n";
        return;
    }
    
    if (!player->takeItem(arg)) {
        std::cout << "You don't see anything like '" << arg << "' here. Try \033[1;32msearch\033[0m first.\n";
    }
}

void Game::handleDrop(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Drop what?\n";
        return;
    }
    
    if (!player->dropItem(arg)) {
        std::cout << "You don't have a '" << arg << "' in your inventory.\n";
    }
}

void Game::handlePickupLoot() {
    Room* room = player->getCurrentRoom();
    if (!room || !room->hasUnclaimedLoot()) {
        std::cout << "There is no loot here to pick up.\n";
        return;
    }

    Item loot;
    if (!room->claimPendingLoot(loot)) {
        std::cout << "You find nothing else worth taking.\n";
        return;
    }

    if (normalizeItemKey(loot.getName()) == "rope") {
        std::cout << "You kneel down and search through the remains.\n\n";
        std::cout << "Among the bones and scattered debris, you find a worn leather satchel.\n\n";
        std::cout << "Inside is a length of sturdy rope, carefully wrapped to keep it dry.\n\n";
        player->receiveItem(loot);
        std::cout << "\nThe rope looks old, but it is still strong enough to help with a climb.\n";
        return;
    }

    std::cout << "You kneel and search the remains...\n";
    player->receiveItem(loot);
}

void Game::handleUse(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Use what?\n";
        return;
    }

    if (!player->useItem(arg)) {
        std::cout << "You don't have a '" << arg << "' to use.\n";
    }
}

void Game::handleEquip(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Equip what?\n";
        return;
    }

    if (!player->equipItem(arg)) {
        std::cout << "You don't have a '" << arg << "' in your inventory.\n";
    }
}

void Game::handleKnightTalk(const std::string& arg) {
    (void)arg;
    static const std::vector<std::string> knightLines = {
        "The knight raises his blade. \"Another fool sent to die.\"",
        "Through the visor: \"Leave, or be broken like the shields at my feet.\"",
        "\"Words?\" he snarls. \"Only steel decides who passes.\""
    };
    size_t idx = static_cast<size_t>(std::min(knightTalkCount, static_cast<int>(knightLines.size()) - 1));
    std::cout << "\033[1;31mThe armored knight says:\033[0m \"" << knightLines[idx] << "\"\n";
    knightTalkCount++;
}

void Game::handleTalk(const std::string& arg) {
    Enemy* enemy = player->getCurrentRoom()->getEnemy();
    if (enemy && enemy->isAlive()) {
        if (isArmoredKnightEnemy(enemy)) {
            handleKnightTalk(arg);
            return;
        }
        if (enemy->getName() == "Demon General Malakor") {
            static const std::vector<std::string> malakorLines = {
                "You stand in a graveyard and ask for words? Turn back, mortal.",
                "I am the last oath before the throne. Steel, not speech, decides this hall.",
                "Enough. Your tongue buys no more time."
            };
            size_t idx = static_cast<size_t>(std::min(malakorTalkCount, 2));
            std::cout << "\033[1;31mMalakor growls:\033[0m \"" << malakorLines[idx] << "\"\n";
            malakorTalkCount++;
            if (malakorTalkCount > 2) {
                int strike = 18;
                std::cout << "\033[1;31mMalakor loses patience and cleaves into you for " << strike << " damage!\033[0m\n";
                player->takeDamage(strike);
                if (!player->isAlive()) {
                    std::cout << "\033[1;31mYou collapse under Malakor's blow...\033[0m\n";
                    return;
                }
                startCombat(enemy);
            }
            return;
        }
        if (enemy->getName() == "Demon Lord Azael") {
            static const std::vector<std::string> azaelLines = {
                "Kneel, and I may grant you a quick death.",
                "I can hear fear in your breath, little hero.",
                "Then bleed. Words are over."
            };
            size_t idx = static_cast<size_t>(std::min(azaelTalkCount, 2));
            std::cout << "\033[1;31mAzael says:\033[0m \"" << azaelLines[idx] << "\"\n";
            azaelTalkCount++;
            if (azaelTalkCount > 2) {
                int strike = 25;
                std::cout << "\033[1;31mAzael lashes out with dark force for " << strike << " damage!\033[0m\n";
                player->takeDamage(strike);
                if (!player->isAlive()) {
                    std::cout << "\033[1;31mAzael's wrath overwhelms you...\033[0m\n";
                    return;
                }
                startCombat(enemy);
            }
            return;
        }
    }

    NPC* npc = selectNpcForConversation(arg);
    if (!npc) {
        if (player->getCurrentRoom()->getNPCs().empty()) {
            std::cout << "There is no one here to talk to.\n";
        } else if (arg.empty()) {
            printConversationTargetsHint();
        } else {
            std::cout << "You don't see '" << arg << "' here.\n";
        }
        return;
    }
    lastTalkNpcName = npc->getName();

    std::string npcName = toLower(npc->getName());
    if (npcName.find("eleanor") != std::string::npos) {
        knowsGarrisonStory = true;
        specialExamineUnlocked = true;
        const int scene = eleanorTalkCount;
        if (scene == 0) {
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"I still wait for my husband... even after all this time.\"\n";
            std::cout << "She looks toward the northern road.\n";
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"Everyone tells me to forget him. They say the man who left is not the man who will return.\"\n";
        } else if (scene == 1) {
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"His crest was a lion surrounded by thorns.\"\n";
            std::cout << "She touches the ring on her finger.\n";
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"He told me it meant a warrior who protects what he loves, even when the world turns against him.\"\n";
        } else if (scene == 2) {
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"When he left for the Spire, he promised me he would come home.\"\n";
            std::cout << "Her voice grows quieter.\n";
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"But the Demon Lord's curse has taken many things from those who enter...\"\n";
            std::cout << "\"Memories. Names. Even the faces of the people they love.\"\n";
        } else {
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"Please... speak with \033[1;35mElder Cedric\033[0m if you mean to climb. I have told you all I can.\"\n";
        }
        eleanorTalkCount++;
        if (eleanorTalkCount >= 3) {
            eleanorHeardAllDialogue = true;
        }
    } else if (npcName.find("bram") != std::string::npos) {
        if (bramForestFled && player->getCurrentRoom()->getName() == "Whispering Woods") {
            std::cout << "\033[1;35mBram says:\033[0m \"You ran last time... but you came back.\"\n";
        } else {
            std::cout << "\033[1;35m" << npc->getName() << " says:\033[0m \""
                      << npc->getDialogue() << "\"\n";
        }
    } else if (npcName.find("lyra") != std::string::npos) {
        if (lyraRiddleSolved || !examinedFountain || !npc->isDialogueExhausted()) {
            std::cout << "\033[1;35mLyra says:\033[0m \""
                      << npc->getDialogue() << "\"\n";
        } else {
            std::cout << "\033[1;35mLyra\033[0m glances at the fountain note you mentioned earlier.\n\n";
            std::cout << "\"The words there... and the texts here... they echo each other.\"\n\n";
            std::cout << "Lyra pauses.\n\n";
            std::cout << "\"I cannot be held,\n";
            std::cout << "yet I never leave your side.\n\n";
            std::cout << "I cannot be seen,\n";
            std::cout << "yet I shape every choice you make.\n\n";
            std::cout << "The past gives me life.\n";
            std::cout << "The future is changed by me.\n\n";
            std::cout << "What am I?\"\n\n";
            promptLyraRiddleAnswer();
        }
    } else {
        std::cout << "\033[1;35m" << npc->getName() << " says:\033[0m \""
                  << npc->getDialogue() << "\"\n";
    }

    if (npcName.find("bram") != std::string::npos) {
        specialGiveUnlocked = true;
        specialRecruitUnlocked = true;
    } else if (npcName.find("pip") != std::string::npos) {
        specialGiveUnlocked = true;
        specialPraiseUnlocked = true;
        specialRecruitUnlocked = true;
    } else if (npcName.find("lyra") != std::string::npos) {
        specialRecruitUnlocked = true;
    } else if (npcName.find("valerie") != std::string::npos) {
        specialGiveUnlocked = true;
        specialRecruitUnlocked = true;
    } else if (npcName.find("elder") != std::string::npos || npcName.find("eleanor") != std::string::npos
               || npcName.find("garrick") != std::string::npos) {
        specialRecruitUnlocked = true;
    }
              
    if (npc->getIsMerchant()) {
        std::cout << "Type \033[1;32m'buy <item>'\033[0m to purchase something, or \033[1;32m'sell <item>'\033[0m to sell your goods.\n";
        std::cout << "Merchant Inventory:\n";
        printMerchantInventory(npc->getShopItems());
    }
}

void Game::handleBuy(const std::string& arg) {
    NPC* npc = player->getCurrentRoom()->getNPC();
    if (!npc || !npc->getIsMerchant()) {
        std::cout << "There is no shopkeeper here.\n";
        return;
    }

    if (arg.empty()) {
        std::cout << "Buy what? Type 'talk' to see the merchant's wares.\n";
        return;
    }

    // Find the item in the merchant's list
    auto& shopItems = npc->getShopItems();
    auto it = std::find_if(shopItems.begin(), shopItems.end(), [&](const Item& item) {
        return normalizeItemKey(item.getName()) == normalizeItemKey(arg);
    });

    if (it == shopItems.end()) {
        std::cout << "The merchant does not sell '" << arg << "'.\n";
        return;
    }

    Item itemToBuy = *it;
    if (player->getGold() < itemToBuy.getGoldValue()) {
        std::cout << "You don't have enough gold! You need \033[1;32m" << itemToBuy.getGoldValue() 
                  << "g\033[0m but you only have \033[1;33m" << player->getGold() << "g\033[0m.\n";
        return;
    }

    // Complete transaction
    player->removeGold(itemToBuy.getGoldValue());
    npc->removeShopItem(itemToBuy.getName());
    player->receiveItem(itemToBuy);

    std::cout << "Purchased \033[1;33m" << itemToBuy.getName() << "\033[0m for \033[1;32m" 
              << itemToBuy.getGoldValue() << "g\033[0m. Remaining Gold: \033[1;33m" << player->getGold() << "g\033[0m\n";
}

void Game::handleSell(const std::string& arg) {
    NPC* npc = player->getCurrentRoom()->getNPC();
    if (!npc || !npc->getIsMerchant()) {
        std::cout << "There is no shopkeeper here.\n";
        return;
    }

    if (arg.empty()) {
        std::cout << "Sell what?\n";
        return;
    }

    Item playerItem;
    if (!player->getItem(arg, playerItem)) {
        std::cout << "You do not have a '" << arg << "' in your inventory.\n";
        return;
    }

    // Key items cannot be sold
    if (playerItem.getType() == ItemType::KEY) {
        std::cout << "The merchant won't buy key quest items.\n";
        return;
    }

    int sellValue = playerItem.getGoldValue() / 2;
    if (sellValue < 1) sellValue = 1;

    std::cout << "Garrick offers \033[1;32m" << sellValue << "g\033[0m for \033[1;33m"
              << playerItem.getName() << "\033[0m.\n";
    std::cout << "The sale completes.\n";

    player->removeItem(playerItem.getName());
    player->addGold(sellValue);
    npc->addShopItem(playerItem);

    std::cout << "You sold \033[1;33m" << playerItem.getName() << "\033[0m for \033[1;32m" 
              << sellValue << "g\033[0m. Total Gold: \033[1;33m" << player->getGold() << "g\033[0m\n";
}

static int findCombatFighterIndex(const std::vector<FellowshipMember>& fighters, const std::string& fragment) {
    if (fragment.empty()) {
        return 0;
    }
    const std::string target = toLower(fragment);
    for (size_t i = 0; i < fighters.size(); ++i) {
        if (toLower(fighters[i].name).find(target) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

static void printCombatFighterList(const std::vector<FellowshipMember>& fighters) {
    bool first = true;
    for (const FellowshipMember& fighter : fighters) {
        if (fighter.health <= 0) {
            continue;
        }
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        std::cout << fighter.name;
    }
}

static std::string combatAttackLine(const FellowshipMember& fighter, const std::string& enemyName) {
    const std::string key = toLower(fighter.name);
    const std::string foe = enemyName == "Demon Lord Azael" ? "Azael" : enemyName;
    if (enemyName == "Demon Lord Azael") {
        if (key.find("pip") != std::string::npos) {
            return "\033[1;36mPip\033[0m slips between shadows, cutting into \033[1;31mAzael\033[0m";
        }
        if (key.find("bram") != std::string::npos) {
            return "\033[1;36mBram\033[0m's hammer crashes into the stone floor beside \033[1;31mAzael\033[0m, forcing him to shift";
        }
        if (key.find("valerie") != std::string::npos) {
            return "\033[1;36mValerie\033[0m dives from above, tearing through the air toward \033[1;31mAzael\033[0m";
        }
        if (key.find("garrison") != std::string::npos || key.find("sir") != std::string::npos) {
            return "\033[1;36mSir Garrison\033[0m advances with measured steps, shield forcing space against \033[1;31mAzael\033[0m";
        }
        if (key.find("lyra") != std::string::npos) {
            return "\033[1;36mLyra\033[0m speaks a fractured word of power; arcane fire flickers across \033[1;31mAzael\033[0m";
        }
    }
    if (enemyName == "Demon General Malakor") {
        if (key.find("pip") != std::string::npos) {
            return "\033[1;36mPip\033[0m darts beneath Malakor's guard and slashes \033[1;31mMalakor\033[0m";
        }
        if (key.find("bram") != std::string::npos) {
            return "\033[1;36mBram\033[0m hammers \033[1;31mMalakor\033[0m's armor with a thunderous blow";
        }
        if (key.find("valerie") != std::string::npos) {
            return "\033[1;36mValerie\033[0m dives past the runes, raking \033[1;31mMalakor\033[0m with her talons";
        }
        if (key.find("garrison") != std::string::npos || key.find("sir") != std::string::npos) {
            return "\033[1;36mSir Garrison\033[0m drives his shield edge into \033[1;31mMalakor\033[0m";
        }
        if (key.find("lyra") != std::string::npos) {
            return "\033[1;36mLyra\033[0m hurls arcane fire at \033[1;31mMalakor\033[0m";
        }
    }
    if (key.find("pip") != std::string::npos) {
        return "\033[1;36mPip\033[0m darts in and slashes \033[1;31m" + foe + "\033[0m";
    }
    if (key.find("bram") != std::string::npos) {
        return "\033[1;36mBram\033[0m hammers \033[1;31m" + foe + "\033[0m with a thunderous blow";
    }
    if (key.find("valerie") != std::string::npos) {
        return "\033[1;36mValerie\033[0m dives in, raking \033[1;31m" + foe + "\033[0m with her talons";
    }
    if (key.find("garrison") != std::string::npos || key.find("sir") != std::string::npos) {
        return "\033[1;36mSir Garrison\033[0m drives his shield edge into \033[1;31m" + foe + "\033[0m";
    }
    if (key.find("lyra") != std::string::npos) {
        return "\033[1;36mLyra\033[0m hurls arcane fire at \033[1;31m" + foe + "\033[0m";
    }
    return "\033[1;32m" + fighter.name + "\033[0m strikes \033[1;31m" + foe + "\033[0m";
}

void Game::fellowshipCombatAssist(Enemy* enemy, const std::vector<FellowshipMember>& fighters, int primaryIndex) {
    if (!enemy || !enemy->isAlive() || primaryIndex != 0) {
        return;
    }

    for (size_t i = 1; i < fighters.size(); ++i) {
        if (fighters[i].health <= 0) {
            continue;
        }
        const FellowshipMember& ally = fighters[i];
        int damageDealt = std::max(1, ally.attackPower - enemy->getDefensePower());
        enemy->takeDamage(ally.attackPower);
        std::cout << combatAttackLine(ally, enemy->getName()) << " for \033[1;32m" << damageDealt
                  << " damage\033[0m!\n";
    }
}

void Game::startCombat(Enemy* enemy) {
    const bool fleeBlocked = isCombatFleeBlocked(enemy);
    const bool knightFight = isArmoredKnightEnemy(enemy);
    const bool azaelFight = isAzaelEnemy(enemy);
    const bool batFight = isGiantCaveBatEnemy(enemy);
    const bool trollFight = isCaveTrollEnemy(enemy);
    const bool houndFight = isDemonHoundEnemy(enemy);
    const bool goblinFight = isGoblinScoutEnemy(enemy);
    const bool malakorFight = isMalakorEnemy(enemy);
    const bool valerieFight = isValerieEnemy(enemy);
    int batAttackCount = 0;
    int trollHeroAttackCount = 0;
    int heroAttackTurnCount = 0;
    int azaelStrikeCount = 0;
    bool batUsedPotionThisTurn = false;
    bool trollRoarShown = false;
    bool heroStruckThisTurn = false;

    std::cout << "\n\033[1;31m!!! BATTLE START !!!\033[0m\n";
    printCombatBattleStart(enemy, knightFight, azaelFight, batFight, trollFight, houndFight,
        goblinFight, malakorFight, valerieFight);
    if (azaelFight) {
        if (isInParty("pip") || isInParty("bram") || isInParty("valerie") || isInParty("lyra")
            || isInParty("garrison") || isInParty("sir")) {
            std::cout << "\033[1;36mYour fellowship moves with you.\033[0m\n";
            std::cout << "The Spire itself feels like it is closing in.\n";
        }
    }
    if (fleeBlocked) {
        std::cout << "\033[1;30mThe demon castle seals the halls behind you. You cannot flee.\033[0m\n";
    }

    std::vector<FellowshipMember> fighters;
    fighters.push_back({
        player->getName(),
        player->getHealth(),
        player->getMaxHealth(),
        player->getAttackPower(),
        player->getDefensePower()
    });
    for (const FellowshipMember& member : fellowshipRoster) {
        fighters.push_back(applyFellowshipGearBonuses(member, bramMasterwork));
    }
    const bool forestGoblinFight = enemy->getName() == "Goblin Scout"
        && player->getCurrentRoom()->getName() == "Whispering Woods"
        && !bramSavedGoblin && !bramRecruited && findNPCInCurrentRoom("bram");
    if (forestGoblinFight) {
        fighters.push_back(defaultFellowshipStats("Bram"));
        std::cout << "\033[1;36mBram\033[0m raises his hammer beside you!\n";
    }
    if (fighters.size() > 1) {
        std::cout << "\033[1;36mYour fellowship stands ready.\033[0m Choose who acts each turn.\n";
    }

    auto syncFightersToGame = [&]() {
        player->setHealth(fighters[0].health);
        for (size_t i = 0; i < fellowshipRoster.size() && i + 1 < fighters.size(); ++i) {
            fellowshipRoster[i] = fighters[i + 1];
        }
    };

    auto healFighter = [&](int idx, int amount) {
        if (idx < 0 || idx >= static_cast<int>(fighters.size())) {
            return;
        }
        fighters[idx].health = std::min(fighters[idx].maxHealth, fighters[idx].health + amount);
        if (idx == 0) {
            player->setHealth(fighters[0].health);
        }
    };

    std::string line;
    while (enemy->isAlive() && fighters[0].health > 0) {
        syncFightersToGame();
        std::cout << "\n---\n";
        for (size_t i = 0; i < fighters.size(); ++i) {
            const FellowshipMember& fighter = fighters[i];
            if (fighter.health <= 0) {
                std::cout << "  \033[1;30m" << fighter.name << " (down)\033[0m\n";
            } else {
                std::cout << "  \033[1;32m" << fighter.name << "\033[0m: " << fighter.health << "/"
                          << fighter.maxHealth << " HP\n";
            }
        }
        if (batFight) {
            std::cout << "  \033[1;31mGiant Cave Bat\033[0m: " << enemy->getHealth() << "/"
                      << enemy->getMaxHealth() << " HP\n";
        } else {
            std::cout << "  \033[1;31m" << enemy->getName() << "\033[0m: " << enemy->getHealth() << "/"
                      << enemy->getMaxHealth() << " HP\n";
        }
        if (fighters.size() > 1) {
            const int untilRally = 5 - (heroAttackTurnCount % 5);
            if (untilRally <= 1) {
                std::cout << "  \033[1;32m>> Coordinated assault READY\033[0m - your next hero strike rallies all allies!\n";
            } else {
                std::cout << "  Coordinated assault in \033[1;36m" << untilRally
                          << "\033[0m more hero strike(s).\n";
            }
        }
        if (bramMasterwork && !fellowshipRoster.empty()) {
            std::cout << "  Fellowship defenses: \033[1;34m+" << kBramMasterworkDefenseBonus
                      << "\033[0m (Bram's masterwork).\n";
        }
        std::cout << "Options: \033[1;32m<name> attack\033[0m, \033[1;32mheal <name>\033[0m";
        if (isInParty("lyra")) {
            std::cout << ", \033[1;32mlyra heal <name>\033[0m";
        }
        std::cout << ", \033[1;32muse <item> <name>\033[0m, \033[1;32mstats\033[0m";
        if (knightFight) {
            std::cout << ", \033[1;32mtalk\033[0m";
        }
        if (fleeBlocked) {
            std::cout << ", \033[1;32mgive <item> <target>\033[0m";
        }
        if (!fleeBlocked) {
            std::cout << ", \033[1;32mflee\033[0m";
        }
        if (enemy->getName() == "Demon Lord Azael") {
            std::cout << ", \033[1;32mgive diary azael\033[0m";
        }
        std::cout << "\n";
        std::cout << "\033[1;31mCombat >\033[0m ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        line = trim(line);
        if (line.empty()) continue;

        std::string verb;
        std::string arg;
        
        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos) {
            verb = toLower(line);
        } else {
            verb = toLower(line.substr(0, spacePos));
            arg = trim(line.substr(spacePos + 1));
        }

        bool playerTurnTaken = false;
        batUsedPotionThisTurn = false;
        heroStruckThisTurn = false;

        bool isAttackCommand = false;
        std::string attackerName;
        if (arg == "attack" || arg == "a") {
            isAttackCommand = true;
            attackerName = verb;
        } else if ((verb == "attack" || verb == "a") && arg.empty()) {
            isAttackCommand = true;
        }

        if (isAttackCommand) {
            int attackerIdx = 0;
            if (attackerName.empty()) {
                if (fighters.size() == 1) {
                    attackerIdx = 0;
                } else {
                    std::cout << "Who attacks? (e.g. ";
                    printCombatFighterList(fighters);
                    std::cout << " attack)\n";
                    continue;
                }
            } else {
                attackerIdx = findCombatFighterIndex(fighters, attackerName);
            }
            if (attackerIdx < 0) {
                std::cout << "No one like that is fighting here.\n";
                continue;
            }
            if (fighters[attackerIdx].health <= 0) {
                std::cout << fighters[attackerIdx].name << " cannot fight right now.\n";
                continue;
            }

            if (trollFight && attackerIdx == 0 && !trollRoarShown) {
                trollRoarShown = true;
                std::cout << "The Cave Troll lets out a thunderous roar!\n\n";
                std::cout << "The cavern shakes as it raises its weapon and charges.\n\n";
            }

            const FellowshipMember& attacker = fighters[attackerIdx];
            int strikePower = attacker.attackPower;
            if (trollFight && attackerIdx == 0 && trollHeroAttackCount == 0) {
                strikePower = 8;
            }
            int damageDealt = std::max(1, strikePower - enemy->getDefensePower());
            enemy->takeDamage(strikePower);

            if (trollFight && attackerIdx == 0) {
                if (trollHeroAttackCount == 0) {
                    damageDealt = 5;
                    std::cout << "You strike the Cave Troll!\n\n";
                    std::cout << "Your attack lands, but the creature barely reacts.\n\n";
                    std::cout << "Cave Troll takes \033[1;32m" << damageDealt << " damage\033[0m.\n";
                } else {
                    std::cout << "You strike the Cave Troll for \033[1;32m" << damageDealt
                              << " damage\033[0m!\n";
                }
                ++trollHeroAttackCount;
            } else if (batFight && attackerIdx == 0) {
                if (batAttackCount == 0) {
                    std::cout << "You swing at the Giant Cave Bat!\n\n";
                    std::cout << "Your strike lands.\n\n";
                    std::cout << "The bat takes \033[1;32m" << damageDealt << " damage\033[0m.\n";
                } else {
                    std::cout << "You strike the Giant Cave Bat for \033[1;32m" << damageDealt
                              << " damage\033[0m!\n";
                }
                ++batAttackCount;
            } else if (attackerIdx == 0) {
                printCombatHeroStrike(enemy, batFight, trollFight, houndFight, goblinFight, knightFight,
                    malakorFight, azaelFight, valerieFight, damageDealt);
                heroStruckThisTurn = true;
            } else {
                std::cout << combatAttackLine(attacker, enemy->getName()) << " for \033[1;32m"
                          << damageDealt << " damage\033[0m!\n";
            }

            if (attackerIdx == 0 && enemy->isAlive() && fighters.size() > 1) {
                ++heroAttackTurnCount;
                if (heroAttackTurnCount % 5 == 0) {
                    std::cout << "\033[1;36mYour fellowship rallies for a coordinated assault!\033[0m\n";
                    fellowshipCombatAssist(enemy, fighters, attackerIdx);
                }
            }
            playerTurnTaken = true;
        }
        else if (verb == "heal" || verb == "use") {
            std::string itemToken;
            std::string targetToken;
            if (verb == "heal") {
                targetToken = arg;
                itemToken = "healthpotion";
            } else {
                size_t splitPos = arg.find(' ');
                if (splitPos == std::string::npos) {
                    if (isHealingConsumableKey(arg)) {
                        itemToken = arg;
                        targetToken = player->getName();
                    } else {
                        std::cout << "Use what on whom? Try \033[1;32muse elixir hero\033[0m or \033[1;32muse potion pip\033[0m.\n";
                        continue;
                    }
                } else {
                    itemToken = arg.substr(0, splitPos);
                    targetToken = trim(arg.substr(splitPos + 1));
                }
            }

            if (!isHealingConsumableKey(itemToken)) {
                std::cout << "You can only use healing items on allies in battle.\n";
                continue;
            }
            if (targetToken.empty()) {
                std::cout << "Heal whom? ";
                printCombatFighterList(fighters);
                std::cout << "\n";
                continue;
            }

            int targetIdx = findCombatFighterIndex(fighters, targetToken);
            if (targetIdx < 0) {
                std::cout << "No ally matches that name.\n";
                continue;
            }
            Item healItem;
            if (!findHealingItemInInventory(player, itemToken, healItem)) {
                std::cout << "You don't have that healing item.\n";
                continue;
            }

            player->removeItem(healItem.getName());
            int healAmt = healItem.getEffectValue();
            healFighter(targetIdx, healAmt);
            const std::string itemLabel = healingItemLabel(healItem);
            if (targetIdx == 0) {
                std::cout << "You drink the " << itemLabel << ".\n\n";
            } else {
                std::cout << "You give \033[1;33m" << fighters[targetIdx].name
                          << "\033[0m an " << itemLabel << ".\n\n";
            }
            if (normalizeItemKey(healItem.getName()) == "elixir") {
                std::cout << "A brilliant golden warmth floods through their body.\n\n";
            } else {
                std::cout << "A warm energy spreads through their body.\n\n";
            }
            std::cout << fighters[targetIdx].name << " recovers \033[1;32m" << healAmt << " HP\033[0m "
                      << "(" << fighters[targetIdx].health << "/" << fighters[targetIdx].maxHealth << ").\n";
            playerTurnTaken = true;
            batUsedPotionThisTurn = (batFight && targetIdx == 0);
        }
        else if (verb == "lyra") {
            if (!isInParty("lyra")) {
                std::cout << "Lyra is not in your fellowship.\n";
                continue;
            }
            std::string healTarget = arg;
            if (healTarget.find("heal") == 0) {
                healTarget = trim(healTarget.substr(4));
            }
            if (healTarget.empty()) {
                std::cout << "Lyra heal whom?\n";
                continue;
            }
            int lyraIdx = findCombatFighterIndex(fighters, "lyra");
            if (lyraIdx < 0 || fighters[lyraIdx].health <= 0) {
                std::cout << "Lyra cannot cast right now.\n";
                continue;
            }
            int targetIdx = findCombatFighterIndex(fighters, healTarget);
            if (targetIdx < 0) {
                std::cout << "No ally matches that name.\n";
                continue;
            }
            const int lyraHeal = 35;
            healFighter(targetIdx, lyraHeal);
            const bool lyraHealsSelf = (lyraIdx == targetIdx);
            if (lyraHealsSelf) {
                std::cout << "\033[1;36mLyra\033[0m murmurs a soft ward and heals herself.\n\n";
            } else {
                std::cout << "\033[1;36mLyra\033[0m murmurs a soft ward over \033[1;32m"
                          << fighters[targetIdx].name << "\033[0m.\n\n";
            }
            if (lyraHealsSelf) {
                std::cout << "Lyra recovers \033[1;32m" << lyraHeal << " HP\033[0m "
                          << "(" << fighters[targetIdx].health << "/" << fighters[targetIdx].maxHealth << ").\n";
            } else {
                std::cout << fighters[targetIdx].name << " recovers \033[1;32m" << lyraHeal << " HP\033[0m "
                          << "(" << fighters[targetIdx].health << "/" << fighters[targetIdx].maxHealth << ").\n";
            }
            playerTurnTaken = true;
        }
        else if (verb == "talk" || verb == "speak") {
            if (knightFight) {
                handleKnightTalk(arg);
                if (!enemy->isAlive()) {
                    return;
                }
            } else {
                std::cout << "You cannot talk your way through this fight.\n";
            }
        }
        else if (verb == "give" && fleeBlocked) {
            handleGive(arg);
            if (!enemy->isAlive()) {
                return;
            }
        }
        else if (verb == "flee" || verb == "run") {
            if (fleeBlocked) {
                std::cout << "\033[1;31mThere is no escape. The Spire's halls trap you in the fight.\033[0m\n";
                continue;
            }
            if (enemy->getName() == "Goblin Scout"
                && player->getCurrentRoom()->getName() == "Whispering Woods") {
                std::cout << "You break away from the fight and retreat into the trees.\n\n";
                std::cout << "The Goblin Scout snarls but does not pursue you far.\n\n";
                std::cout << "You escape the battle.\n\n";
                std::cout << "Bram is still in danger deeper in the Whispering Woods.\n";
                bramForestFled = true;
                syncFightersToGame();
                return;
            }
            if (trollFight) {
                std::cout << "You realize brute force will not be enough.\n\n";
                std::cout << "You retreat through the crystal cavern before the troll can strike again.\n\n";
                std::cout << "You escape safely.\n";
                syncFightersToGame();
                Room* westRoom = nullptr;
                for (const auto& exitPair : player->getCurrentRoom()->getExits()) {
                    if (exitPair.first == "west") {
                        westRoom = exitPair.second;
                        break;
                    }
                }
                if (westRoom) {
                    player->setCurrentRoom(westRoom);
                }
                return;
            }
            if (batFight) {
                std::cout << "You turn back and rush toward the well opening.\n\n";
                std::cout << "The creature screeches behind you, but you manage to escape.\n\n";
                std::cout << "You climb back to the surface, battered but alive.\n\n";
                player->takeDamage(5);
                std::cout << "You took \033[1;31m5 damage\033[0m escaping.\n";
                std::cout << "Current HP: \033[1;32m" << player->getHealth() << "/" << player->getMaxHealth()
                          << "\033[0m\n";
                syncFightersToGame();
                Room* wellRoom = findRoomByName("The Old Well");
                if (wellRoom) {
                    player->setCurrentRoom(wellRoom);
                }
                return;
            }

            syncFightersToGame();
            Room* escapeRoom = nullptr;
            for (const auto& exitPair : player->getCurrentRoom()->getExits()) {
                if (exitPair.second != player->getCurrentRoom()
                    && (!exitPair.second->getEnemy() || !exitPair.second->getEnemy()->isAlive())) {
                    escapeRoom = exitPair.second;
                    break;
                }
            }

            std::cout << "\033[1;33mYou flee from the battle back to safety!\033[0m\n";
            if (escapeRoom) {
                player->setCurrentRoom(escapeRoom);
            } else {
                player->setCurrentRoom(rooms[0]);
            }
            return;
        }
        else if (verb == "stats") {
            printPlayerStats(heroAttackTurnCount);
        }
        else if (verb == "work" || verb == "rest") {
            std::cout << "You cannot work or rest during combat!\n";
        }
        else {
            std::cout << "Invalid battle option. Try: <name> attack, heal <name>, use elixir <name>, stats";
            if (isInParty("lyra")) {
                std::cout << ", lyra heal <name>";
            }
            if (knightFight) {
                std::cout << ", talk";
            }
            if (fleeBlocked) {
                std::cout << ", give <item> <target>";
            }
            if (!fleeBlocked) {
                std::cout << ", flee";
            }
            if (enemy->getName() == "Demon Lord Azael") {
                std::cout << ", give diary azael";
            }
            std::cout << ".\n";
        }

        // Enemy Turn
        if (playerTurnTaken && enemy->isAlive()) {
            std::vector<int> livingTargets;
            for (size_t i = 0; i < fighters.size(); ++i) {
                if (fighters[i].health > 0) {
                    livingTargets.push_back(static_cast<int>(i));
                }
            }
            if (!livingTargets.empty()) {
                int targetIdx = livingTargets[static_cast<size_t>(rand()) % livingTargets.size()];
                int enemyDmg = std::max(1, enemy->getAttackPower() - fighters[targetIdx].defensePower);
                fighters[targetIdx].health = std::max(0, fighters[targetIdx].health - enemyDmg);
                if (targetIdx == 0) {
                    player->setHealth(fighters[0].health);
                }

                printCombatEnemyStrike(enemy, batFight, trollFight, houndFight, goblinFight, knightFight,
                    malakorFight, azaelFight, valerieFight, targetIdx, fighters[targetIdx].name, enemyDmg,
                    batUsedPotionThisTurn, batAttackCount, heroStruckThisTurn, azaelStrikeCount);
                if (azaelFight && targetIdx == 0) {
                    ++azaelStrikeCount;
                }
                if (fighters[targetIdx].health <= 0) {
                    std::cout << fighters[targetIdx].name << " is knocked down!\n";
                }
            }
        }

        if (gameWon) {
            syncFightersToGame();
            return;
        }
    }

    syncFightersToGame();

    if (fighters[0].health <= 0) {
        std::cout << "\n\033[1;31mYou have been defeated by " << enemy->getName() << "...\033[0m\n";
    } else if (!enemy->isAlive()) {
        Room* current = player->getCurrentRoom();
        const std::string& defeatedName = enemy->getName();
        printCombatVictory(enemy, batFight, trollFight, goblinFight, knightFight, malakorFight,
            houndFight, valerieFight, azaelFight);

        const bool batVictory = batFight;

        if (defeatedName == "Demon General Malakor" || defeatedName == "Armored Knight"
            || defeatedName == "Sir Garrison" || defeatedName == "Valerie") {
            spirePacifist = false;
        }

        std::cout << "You gained \033[1;33m" << enemy->getGoldReward() << " gold coins\033[0m.\n";
        player->addGold(enemy->getGoldReward());

        if (enemy->hasDropItem() && current) {
            current->setPendingLoot(enemy->getDropItem());
            if (batVictory) {
                std::cout << "\nAs the dust settles, something catches your eye beneath the remains.\n\n";
                std::cout << "A small object is half-hidden among the bones and rubble.\n\n";
                std::cout << "Type \033[1;32mpickup loot\033[0m to search the remains.\n";
            } else if (goblinFight) {
                std::cout << "\nAs the forest grows quiet again, something catches your eye.\n\n";
                std::cout << "Something glints among the fallen goblin's belongings.\n\n";
                std::cout << "Type \033[1;32mpickup loot\033[0m to search the remains.\n";
            } else if (trollFight) {
                std::cout << "\nAs the dust settles, something catches your attention.\n\n";
                std::cout << "A rough piece of mineral glimmers among the remains.\n\n";
                std::cout << "Type \033[1;32mpickup loot\033[0m to search the remains.\n";
            } else {
                std::cout << "Something glints among the remains.\n";
                std::cout << "Use \033[1;32mpickup loot\033[0m to search the body and claim it.\n";
            }
        }

        if (!bramSavedGoblin
            && enemy->getName() == "Goblin Scout"
            && player->getCurrentRoom()->getName() == "Whispering Woods") {
            bramSavedGoblin = true;
            bramReadyToRecruit = true;
            specialGiveUnlocked = true;
            specialRecruitUnlocked = true;
            std::cout << "\nBram lowers his hammer and wipes the blood from his brow.\n\n";
            std::cout << "\033[1;35mBram says:\033[0m \"Not bad.\"\n\n";
            std::cout << "He looks at the scattered goblins.\n\n";
            std::cout << "\"You saved me back there. The woods are getting more dangerous than they used to be.\"\n\n";
            std::cout << "\"If you need a smith, I'd be glad to stand with you.\"\n";
            std::cout << "Use \033[1;32mrecruit bram\033[0m when you're ready.\n";
            NPC* bramNpc = findNPCInCurrentRoom("bram");
            if (bramNpc) {
                bramNpc->addDialogue("You actually came through for me. I owe you one, no question.");
                bramNpc->addDialogue("You're marching on the Demon King? Say recruit bram and I'll march with you.");
                bramNpc->addDialogue("Bring me proper ore and I'll turn gratitude into masterwork armor.");
            }
        }

        // Win condition check
        if (enemy->getName() == "Demon Lord Azael") {
            if (endingType == EndingType::NONE) {
                resolveCombatEnding();
            }
            if (endingType == EndingType::BETRAYAL_ENDING) {
                respawnAfterBetrayal();
                return;
            }
            gameWon = true;
        }
    }
}

Room* Game::findRoomByName(const std::string& roomName) const {
    std::string lookup = roomName;
    if (lookup == "Damp Cave") {
        lookup = "The Well Depths";
    }
    for (Room* room : rooms) {
        if (room && room->getName() == lookup) {
            return room;
        }
    }
    return nullptr;
}

NPC* Game::findNPCInCurrentRoom(const std::string& nameFragment) const {
    auto npcs = player->getCurrentRoom()->getNPCs();
    for (NPC* npc : npcs) {
        if (npc->matchesTalkTarget(nameFragment)) {
            return npc;
        }
    }
    return nullptr;
}

void Game::printConversationTargetsHint() const {
    if (!player || !player->getCurrentRoom()) {
        return;
    }
    const auto npcs = player->getCurrentRoom()->getNPCs();
    if (npcs.empty()) {
        return;
    }
    std::cout << "Talk to whom? ";
    for (size_t i = 0; i < npcs.size(); ++i) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << npcs[i]->getName();
    }
    std::cout << "\n";
}

NPC* Game::selectNpcForConversation(const std::string& arg) const {
    if (!player || !player->getCurrentRoom()) {
        return nullptr;
    }
    const auto npcs = player->getCurrentRoom()->getNPCs();
    if (npcs.empty()) {
        return nullptr;
    }

    if (!arg.empty()) {
        return findNPCInCurrentRoom(arg);
    }

    if (npcs.size() == 1) {
        return npcs.front();
    }

    if (!lastTalkNpcName.empty()) {
        for (NPC* npc : npcs) {
            if (npc->getName() == lastTalkNpcName) {
                return npc;
            }
        }
    }

    return nullptr;
}

void Game::handleGive(const std::string& arg) {
    std::stringstream ss(arg);
    std::string first;
    std::string second;
    ss >> first >> second;
    if (first.empty() || second.empty()) {
        std::cout << "Usage: give <item|money> <name>  (or  give <name> <item>)\n";
        return;
    }

    std::string itemOrKeyword = first;
    std::string target = second;
    auto isGiftTarget = [](const std::string& name) {
        std::string lower = toLower(name);
        return lower == "pip" || lower == "bram" || lower == "valerie"
            || lower == "harpy" || lower == "lyra"
            || lower == "knight" || lower == "garrison" || lower == "armored";
    };
    if (isGiftTarget(first)) {
        itemOrKeyword = second;
        target = first;
    }

    std::string itemLower = toLower(itemOrKeyword);
    std::string targetLower = toLower(target);

    if (targetLower == "knight" || targetLower == "garrison" || targetLower == "armored") {
        if (itemLower == "ring" || itemLower == "eleanorsring" || itemLower == "eleanorring") {
            if (player->getCurrentRoom()->getName() != "Tower Floor 1") {
                std::cout << "There is no armored knight here to receive that.\n";
                return;
            }
            Enemy* knight = player->getCurrentRoom()->getEnemy();
            if (!knight || !knight->isAlive() || !isArmoredKnightEnemy(knight)) {
                std::cout << "There is no armored knight here to receive that.\n";
                return;
            }
            if (garrisonCharmed) {
                std::cout << "Sir Garrison already stands at your side.\n";
                return;
            }
            if (!player->hasItem("EleanorsRing")) {
                std::cout << "You do not have Eleanor's ring to offer.\n";
                return;
            }
            charmKnightWithRing();
            return;
        }
        std::cout << "The knight ignores the gift.\n";
        return;
    }

    if (targetLower == "pip") {
        if (!canReachCompanion("pip")) {
            std::cout << "Pip is not here.\n";
            return;
        }
        if (itemLower == "money") {
            tryBribePip("pip");
            return;
        }
        if (itemLower == "bread") {
            if (!player->removeItem("Bread")) {
                std::cout << "You don't have Bread to give.\n";
                return;
            }
            pipBreadGiven = true;
            std::cout << "Pip hesitates.\n\n";
            std::cout << "She takes the bread carefully, like she expects it to be taken back.\n\n";
            std::cout << "\033[1;35mPip says:\033[0m \"...You're weird.\"\n\n";
            std::cout << "A pause.\n\n";
            std::cout << "\"But not cruel. That's rarer than it should be.\"\n";
            return;
        }
    }

    if (targetLower == "bram") {
        if (!canReachCompanion("bram")) {
            std::cout << "Bram is not with you.\n";
            return;
        }
        if (itemLower == "ore") {
            if (!isInParty("bram")) {
                std::cout << "Bram shakes his head. \"Recruit me first, then I'll take your ore.\"\n";
                return;
            }
            if (bramMasterwork) {
                std::cout << "Bram already forged masterwork armor for your fellowship.\n";
                return;
            }
            if (!player->removeItem("Ore")) {
                std::cout << "You don't have Ore to give.\n";
                return;
            }
            bramMasterwork = true;
            std::cout << "Bram examines the strange ore carefully.\n\n";
            std::cout << "His expression changes.\n\n";
            std::cout << "\"Where did you find this?\"\n\n";
            std::cout << "He turns the material in his hands.\n\n";
            std::cout << "\"This isn't ordinary metal... I can work with this.\"\n\n";
            std::cout << "Bram reforges your equipment with the rare ore.\n\n";
            std::cout << "\033[1;32mYour fellowship's defenses improve (+" << kBramMasterworkDefenseBonus
                      << " defense for all companions).\033[0m\n";
            return;
        }
        std::cout << "Bram isn't interested in that.\n";
        return;
    }

    if (targetLower == "azael" || targetLower.find("demon lord") != std::string::npos) {
        if (itemLower == "diary" || itemLower == "azaelsdiary") {
            tryGiveDiaryToAzael();
            return;
        }
        std::cout << "Azael has no use for that gift.\n";
        return;
    }

    if (targetLower == "valerie" || targetLower == "harpy") {
        if (itemLower == "ribbon" || itemLower == "starlightribbon") {
            if (player->getCurrentRoom()->getName() == "Rooftop Aviary") {
                Enemy* valerieEnemy = player->getCurrentRoom()->getEnemy();
                if (valerieEnemy && valerieEnemy->isAlive() && isValerieEnemy(valerieEnemy)) {
                    charmValerieWithRibbon();
                    return;
                }
            }
            if (!canReachCompanion("valerie")) {
                std::cout << "Valerie is not here.\n";
                return;
            }
            if (!player->removeItem("StarlightRibbon")) {
                std::cout << "You need the StarlightRibbon for this.\n";
                return;
            }
            valerieCharmed = true;
            valerieReadyToRecruit = true;
            std::cout << "\033[1;32mValerie softens at the gift of the ribbon.\033[0m\n";
            std::cout << "\033[1;35mValerie says:\033[0m \"Say \033[1;32mrecruit valerie\033[0m, and I'll carry your fellowship skyward.\"\n";
            return;
        }
        std::cout << "Valerie is not interested in that.\n";
        return;
    }

    std::cout << "Nothing happens. That gift route is not valid.\n";
}

void Game::handlePraise(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Praise whom?\n";
        return;
    }

    NPC* npc = selectNpcForConversation(arg);
    if (!npc) {
        if (arg.empty() && player->getCurrentRoom() && player->getCurrentRoom()->getNPCs().size() > 1) {
            std::cout << "Praise whom? ";
            const auto npcs = player->getCurrentRoom()->getNPCs();
            for (size_t i = 0; i < npcs.size(); ++i) {
                if (i > 0) {
                    std::cout << ", ";
                }
                std::cout << npcs[i]->getName();
            }
            std::cout << "\n";
        } else {
            std::cout << "You don't see '" << arg << "' here to praise.\n";
        }
        return;
    }
    lastTalkNpcName = npc->getName();

    std::string npcName = toLower(npc->getName());
    specialPraiseUnlocked = true;

    if (npcName.find("pip") != std::string::npos) {
        std::cout << "You tell her she's sharp, fast, and better at surviving than most people you've met.\n\n";

        if (pipBribed) {
            std::cout << "Pip doesn't look impressed.\n\n";
            if (pipReadyToRecruit) {
                std::cout << "\033[1;35mPip says:\033[0m \"You already paid. Don't pretend it's something else.\"\n\n";
                std::cout << "\"If you want me marching with you, say \033[1;32mrecruit pip\033[0m. I keep my word.\"\n";
            } else {
                std::cout << "\033[1;35mPip says:\033[0m \"Flattery's free. Coin isn't.\"\n\n";
                std::cout << "\"Pay me if you want a blade, not a conversation.\"\n";
            }
            return;
        }

        if (pipBreadGiven) {
            if (pipGenuineBond) {
                std::cout << "\033[1;35mPip says:\033[0m \"You already said your piece. Do you want me with you or not?\"\n";
                return;
            }
            pipPraised = true;
            std::cout << "Pip looks away, chewing slowly.\n\n";
            std::cout << "\033[1;35mPip says:\033[0m \"Hmph. You say that like it matters.\"\n\n";
            std::cout << "But she doesn't throw the bread back.\n\n";
            std::cout << "\"...Still. Not many people notice that stuff.\"\n\n";
            pipGenuineBond = true;
            pipReadyToRecruit = true;
            std::cout << "She shifts her weight, suddenly less defensive.\n\n";
            std::cout << "\"If you're done talking big, just say it straight. Do you want me with you or not?\"\n";
            return;
        }

        if (pipPraised) {
            std::cout << "\033[1;35mPip says:\033[0m \"You already talked. Share bread or pay me if you mean it.\"\n";
            return;
        }

        pipPraised = true;
        std::cout << "Pip looks away.\n\n";
        std::cout << "\033[1;35mPip says:\033[0m \"Pretty words. Everyone says that before they want something.\"\n\n";
        std::cout << "She folds her arms.\n\n";
        std::cout << "\"Share bread if you mean it. Or pay me straight. Praise alone doesn't fill pockets.\"\n";
        return;
    }
    if (npcName.find("bram") != std::string::npos) {
        std::cout << "You praise Bram's craft and steady nerve.\n";
        std::cout << "\033[1;35mBram grunts:\033[0m \"Save the speeches. Bring ore if you want real armor.\"\n";
        return;
    }
    if (npcName.find("lyra") != std::string::npos) {
        std::cout << "You praise Lyra's wit and patience with ancient texts.\n";
        if (lyraRiddleSolved) {
            std::cout << "\033[1;35mLyra smiles faintly:\033[0m \"Flattery is wasted on me. Ask me to march, if you are ready.\"\n";
        } else if (examinedFountain) {
            std::cout << "\033[1;35mLyra smiles faintly:\033[0m \"Kind words. Speak with me when you are ready to answer.\"\n";
        } else {
            std::cout << "\033[1;35mLyra smiles faintly:\033[0m \"Words are fine. Study the fountain in the village square first.\"\n";
        }
        return;
    }
    if (npcName.find("valerie") != std::string::npos) {
        std::cout << "You praise Valerie's grace and command of the winds.\n";
        std::cout << "\033[1;35mValerie scoffs:\033[0m \"Pretty words. Bring me the Starlight Ribbon if you want more than talk.\"\n";
        return;
    }
    if (npcName.find("eleanor") != std::string::npos) {
        if (!eleanorHeardAllDialogue) {
            std::cout << "You praise Eleanor's devotion and strength.\n\n";
            std::cout << "Eleanor studies your expression.\n\n";
            std::cout << "\033[1;35mEleanor says:\033[0m \"Many have offered me comforting words...\"\n\n";
            std::cout << "\"But few have stayed long enough to hear the truth.\"\n\n";
            std::cout << "She closes her hand around the ring.\n\n";
            std::cout << "\"Perhaps one day, when you know the whole story, I will share what I carry.\"\n";
            return;
        }
        if (eleanorRingGifted) {
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"You already carry my ring. Let it guide you.\"\n";
            return;
        }
        std::cout << "You praise Eleanor's unwavering love and the hope she carried for all these years.\n";
        std::cout << "Eleanor looks at you in surprise.\n";
        std::cout << "\033[1;35mEleanor says:\033[0m \"Most people tell me I am foolish for waiting...\"\n";
        std::cout << "She smiles sadly.\n";
        std::cout << "\"But you listened.\"\n";
        std::cout << "She removes the golden ring from her hand.\n";
        std::cout << "\"If you truly plan to climb the Spire, take this.\"\n";
        std::cout << "\"Perhaps when he sees it... he will remember the promise he made.\"\n\n";
        eleanorRingGifted = true;
        specialGiveUnlocked = true;
        Item eleanorsRing("EleanorsRing", "A simple gold ring on a thin chain. It still holds a woman's warmth.",
            ItemType::KEY, 0, 0);
        player->receiveItem(eleanorsRing);
        return;
    }
    if (npcName.find("elder") != std::string::npos) {
        std::cout << "You praise \033[1;35mElder Cedric\033[0m's wisdom.\n";
        std::cout << "\033[1;35mElder Cedric chuckles:\033[0m \"Flattery won't sharpen your sword, child.\"\n";
        return;
    }
    if (npcName.find("garrick") != std::string::npos) {
        std::cout << "You praise Garrick's fair prices.\n";
        std::cout << "\033[1;35mGarrick says:\033[0m \"Praise won't discount steel, friend. Gold will.\"\n";
        return;
    }

    std::cout << "You offer kind words, but " << npc->getName() << " only nods politely.\n";
}

void Game::handleRecruit(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Recruit whom?\n";
        return;
    }

    std::string target = toLower(arg);
    specialRecruitUnlocked = true;

    if (isInParty(arg)) {
        std::cout << "They are already in your fellowship.\n";
        return;
    }

    if (target.find("pip") != std::string::npos) {
        if (!findNPCInCurrentRoom("pip")) {
            std::cout << "Pip is not here.\n";
            return;
        }
        if (pipBribed && pipReadyToRecruit) {
            std::cout << "Pip shrugs.\n\n";
            std::cout << "\033[1;35mPip says:\033[0m \"Fine. I'm in.\"\n\n";
            std::cout << "No ceremony. No warmth.\n\n";
            std::cout << "Just a deal being closed.\n\n";
            addToParty("Pip", "Abandoned Hut");
            return;
        }
        if (pipGenuineBond && pipReadyToRecruit) {
            std::cout << "Pip exhales through her nose.\n\n";
            std::cout << "\033[1;35mPip says:\033[0m \"Fine.\"\n\n";
            std::cout << "She tucks the bread away.\n\n";
            std::cout << "\"But if you slow me down, I'm gone.\"\n\n";
            addToParty("Pip", "Abandoned Hut");
            return;
        }
        std::cout << "Pip stays on guard.\n\n";
        std::cout << "\033[1;35mPip says:\033[0m \"You want me around? Show me money.\"\n\n";
        std::cout << "\"Ten gold. Straight. No stories.\"\n";
        return;
    }

    if (target.find("lyra") != std::string::npos) {
        if (!findNPCInCurrentRoom("lyra")) {
            std::cout << "Lyra is not here.\n";
            return;
        }
        if (!lyraRiddleSolved) {
            if (!examinedFountain) {
                std::cout << "\033[1;35mLyra says:\033[0m \"Study the fountain's runes in the village square, then speak with me here.\"\n";
            } else {
                std::cout << "\033[1;35mLyra says:\033[0m \"Answer my question first, if you want me at your side.\"\n";
            }
            return;
        }
        lyraRecruited = true;
        addToParty("Lyra", "Ruined Archive");
        return;
    }

    if (target.find("bram") != std::string::npos) {
        if (!findNPCInCurrentRoom("bram")) {
            std::cout << "Bram is not here.\n";
            return;
        }
        if (!bramReadyToRecruit) {
            std::cout << "\033[1;35mBram says:\033[0m \"Save me from that goblin first, then we'll talk.\"\n";
            return;
        }
        bramRecruited = true;
        addToParty("Bram", "Whispering Woods");
        return;
    }

    if (target.find("valerie") != std::string::npos) {
        if (!findNPCInCurrentRoom("valerie")) {
            std::cout << "Valerie is not here.\n";
            return;
        }
        if (!valerieReadyToRecruit) {
            std::cout << "\033[1;35mValerie says:\033[0m \"Give me the Starlight Ribbon before you ask me to fly for you.\"\n";
            return;
        }
        valerieRecruited = true;
        addToParty("Valerie", "Rooftop Aviary");
        std::cout << "Valerie's wings unfurl fully.\n\n";
        std::cout << "\033[1;35mValerie says:\033[0m \"...Hold on.\"\n\n";
        std::cout << "The wind around the rooftop bends inward.\n\n";
        std::cout << "She turns toward the open sky beyond the Spire.\n\n";
        std::cout << "\"I can take you past Malakor's hall.\"\n\n";
        std::cout << "Her gaze sharpens.\n\n";
        std::cout << "\"...Straight to the throne.\"\n\n";
        std::cout << "She lowers herself slightly.\n\n";
        std::cout << "\"Climb on.\"\n\n";
        std::cout << "------------------------------------------------------------\n\n";
        std::cout << "The world drops away.\n\n";
        std::cout << "Stone corridors blur into streaks of black and ash.\n\n";
        std::cout << "You do not walk the Spire anymore.\n\n";
        std::cout << "You cross it.\n\n";
        std::cout << "------------------------------------------------------------\n\n";
        Room* throneRoom = findRoomByName("Throne Room");
        if (throneRoom) {
            player->setCurrentRoom(throneRoom);
            std::cout << player->getCurrentRoom()->getDetailedDescription() << "\n";
        }
        return;
    }

    if (target.find("garrison") != std::string::npos || target.find("knight") != std::string::npos) {
        std::cout << "The armored figure ignores the order. Perhaps the right words will reach him first.\n";
        return;
    }

    NPC* npc = findNPCInCurrentRoom(arg);
    if (npc) {
        std::string name = npc->getName();
        if (name.find("Elder") != std::string::npos) {
            std::cout << "\033[1;35mElder Cedric says:\033[0m \"I'm too old for your warband. Guide others, not me.\"\n";
        } else if (name.find("Eleanor") != std::string::npos) {
            std::cout << "\033[1;35mLady Eleanor says:\033[0m \"I must wait here. My place is beside this fountain, not the Spire.\"\n";
        } else if (name.find("Garrick") != std::string::npos) {
            std::cout << "\033[1;35mGarrick says:\033[0m \"I sell steel, not souls. Hire a guard if you need muscle.\"\n";
        } else {
            std::cout << name << " declines to join your fellowship.\n";
        }
    } else {
        std::cout << "You don't see '" << arg << "' here.\n";
    }
}

void Game::promptLyraRiddleAnswer() {
    std::cout << "Enter your answer: ";

    std::string answer;
    if (!std::getline(std::cin, answer)) {
        return;
    }
    answer = toLower(trim(answer));

    if (answer == "memory" || answer == "memories" || answer == "remember" || answer == "the loop"
        || answer == "loop" || answer == "deja vu") {
        lyraRiddleSolved = true;
        specialRecruitUnlocked = true;
        std::cout << "Lyra's expression softens slightly.\n\n";
        std::cout << "\033[1;35mLyra says:\033[0m \"So you noticed it.\"\n\n";
        std::cout << "She closes the book gently.\n\n";
        std::cout << "\"Then you understand more than most who pass through here.\"\n\n";
        std::cout << "Lyra steps back from the shelf.\n\n";
        std::cout << "\"If you want, I will walk beside you. Not as a guide... but as someone who remembers things others forget.\"\n\n";
        std::cout << "Type \033[1;32mrecruit lyra\033[0m to ask her to join your fellowship.\n";
    } else {
        std::cout << "\033[1;35mLyra sighs.\033[0m \"Not quite. Think on what the fountain revealed, then speak with me again.\"\n";
    }
}

bool Game::canRedeemAzaelWithDiary() const {
    if (!player || !player->hasItem("AzaelsDiary")) {
        return false;
    }
    if (pipBribed || !bramMasterwork || !garrisonCharmed || !valerieRecruited || !lyraRecruited) {
        return false;
    }
    if (!isInParty("pip") || !isInParty("lyra") || !isInParty("bram") || !isInParty("valerie")) {
        return false;
    }
    return isInParty("garrison") || isInParty("sir");
}

bool Game::tryGiveDiaryToAzael() {
    if (player->getCurrentRoom()->getName() != "Throne Room") {
        std::cout << "There is no one here that would accept the diary.\n";
        return false;
    }
    Enemy* enemy = player->getCurrentRoom()->getEnemy();
    if (!enemy || !enemy->isAlive() || enemy->getName() != "Demon Lord Azael") {
        std::cout << "There is no response.\n";
        return false;
    }
    if (!player->hasItem("AzaelsDiary")) {
        std::cout << "You do not possess Azael's Diary.\n";
        return false;
    }

    std::cout << "\nYou hold out Azael's Diary.\n\n";
    std::cout << "The throne room does not react.\n\n";
    std::cout << "Azael does not take it immediately.\n\n";

    if (!canRedeemAzaelWithDiary()) {
        std::cout << "\"...You stand here unharmed.\"\n\n";
        std::cout << "His voice is quieter than before.\n\n";
        std::cout << "He finally glances at the diary, then pushes it back. \"You come stained by blood and half-made vows.\"\n";
        return false;
    }

    std::cout << "\"...You stand here unharmed.\"\n\n";
    std::cout << "His voice is quieter than before.\n\n";
    std::cout << "\"No broken knights.\"\n\n";
    std::cout << "\"No fallen guardians.\"\n\n";
    std::cout << "His gaze shifts, as if counting something only he can see.\n\n";
    std::cout << "He finally takes the diary.\n\n";
    std::cout << "He opens it.\n\n";
    std::cout << "\"...And yet Malakor stands unbroken.\"\n\n";
    std::cout << "A pause.\n\n";
    std::cout << "\"...Garrison was not forced to fall.\"\n\n";
    std::cout << "His grip loosens slightly.\n\n";
    std::cout << "\"...Valerie still flies.\"\n\n";
    std::cout << "The air in the hall changes.\n";
    std::cout << "Azael lowers the book a fraction.\n\n";
    std::cout << "\"You did not climb here by destruction.\"\n\n";
    std::cout << "His eyes narrow slightly.\n\n";
    std::cout << "\"...No one was reduced to nothing on your way to me.\"\n";
    std::cout << "A long silence.\n\n";
    std::cout << "Then:\n\n";
    std::cout << "\"...Then what is there left for me to justify?\"\n\n";
    std::cout << "The throne behind him cracks softly.\n\n";
    std::cout << "Not violently.\n\n";
    std::cout << "Like something releasing tension it no longer needs to hold.\n";
    std::cout << "Azael closes the diary.\n\n";
    std::cout << "\"...If no one was broken to reach this moment...\"\n\n";
    std::cout << "\"...Then I do not need to remain what I am.\"\n";
    std::cout << "He turns away from the throne.\n\n";
    std::cout << "It does not resist him.\n\n";
    std::cout << "It simply stops holding its shape.\n";
    std::cout << "Azael exhales.\n\n";
    std::cout << "For the first time, there is no weight in it.\n\n";
    std::cout << "\"I will not rule what no longer needs ruling.\"\n\n";
    std::cout << "He steps down from the throne.\n\n";
    std::cout << "\033[1;36mDemon Lord Azael has stepped down.\033[0m\n\n";
    std::cout << "The Spire remains intact.\n\n";
    std::cout << "No guardians were lost.\n";
    std::cout << "No allies were sacrificed.\n";
    std::cout << "No path was forced through ruin.\n";
    std::cout << "The Spire is behind you now.\n";
    std::cout << "No one chases you down the mountain path.\n";
    std::cout << "The wind feels lighter than it ever did before.\n\n";

    player->removeItem("AzaelsDiary");
    enemy->setHealth(0);
    endingType = EndingType::REDEMPTION_ENDING;
    gameWon = true;
    return true;
}

void Game::handleShow(const std::string& arg) {
    std::string shown = toLower(trim(arg));
    if (shown == "diary" || shown == "azaelsdiary" || shown == "journal") {
        if (player->getCurrentRoom()->getName() == "Throne Room") {
            tryGiveDiaryToAzael();
            return;
        }
        handleRead(arg);
        return;
    }
    std::cout << "Show what?\n";
}

void Game::handleExamine(const std::string& arg) {
    std::string what = toLower(trim(arg));
    specialExamineUnlocked = true;

    if (what.empty()) {
        std::cout << "Examine what? Use \033[1;32mexamine <topic>\033[0m.\n";
        return;
    }

    if (what.find("fountain") != std::string::npos) {
        if (player->getCurrentRoom()->getName() != "Village Square") {
            std::cout << "There is no fountain here to examine.\n";
            return;
        }
        examinedFountain = true;
        std::cout << "The ancient fountain sits silently in the village square.\n\n";
        std::cout << "Strange runes are carved into the stone:\n\n";
        std::cout << "\"That which falls returns.\n";
        std::cout << "That which is lost may yet remain.\"\n\n";
        std::cout << "The water ripples despite the wind being still.\n\n";
        std::cout << "The stone is worn smooth, as if countless hands have reached for the same answer before you.\n\n";
        std::cout << "A faded inscription beneath the runes reads:\n\n";
        std::cout << "\"The first lesson of the cycle is not victory...\n";
        std::cout << "but what you carry forward.\"\n";
        return;
    }

    if (topicIsArmorExam(what)) {
        if (player->getCurrentRoom()->getName() != "Tower Floor 1") {
            std::cout << "Nothing notable about armor here.\n";
            return;
        }
        Enemy* knight = player->getCurrentRoom()->getEnemy();
        if (!knight || !knight->isAlive() || !isArmoredKnightEnemy(knight)) {
            std::cout << "Only shattered scraps of plate remain here. Nothing to study.\n";
            return;
        }
        examinedGarrisonArmor = true;
        specialExamineUnlocked = true;
        if (knowsGarrisonStory) {
            std::cout << "The battered crest shows a lion entwined with a ring of thorns.\n";
            std::cout << "The veiled woman in the village square described this very emblem.\n";
        } else {
            std::cout << "The battered crest shows a lion entwined with a ring of thorns.\n";
            std::cout << "The knight wears it as though it were once dearly loved.\n";
        }
        return;
    }

    std::cout << "You examine the area but find nothing special about \"" << arg << "\".\n";
    std::cout << "Try \033[1;32msearch\033[0m to hunt for hidden objects.\n";
}

void Game::handleSearch(const std::string& arg) {
  std::string topic = toLower(trim(arg));
  if (!topic.empty() && topic.find("fountain") != std::string::npos) {
    handleExamine("fountain");
    return;
  }

  Room* room = player->getCurrentRoom();
  if (room->getName() == "Abandoned Hut" && !pipRevealedBySearch) {
      pipRevealedBySearch = true;
      std::cout << "\"Pff... looking for scraps too?\"\n\n";
      std::cout << "She watches you carefully, arms folded.\n\n";
  }

  std::vector<std::string> found;
  int count = room->searchUndiscovered(found);
  if (count == 0) {
    std::cout << "You search carefully but find nothing new.\n";
    return;
  }
  std::cout << "You search the area and notice:\n";
  for (const auto& desc : found) {
    std::cout << "  - \033[1;33m" << desc << "\033[0m\n";
  }
  std::cout << "Use \033[1;32mtake <description>\033[0m to pick something up.\n";
}

bool Game::hasLivingEnemyInRoom() const {
    if (!player || !player->getCurrentRoom()) {
        return false;
    }
    Enemy* enemy = player->getCurrentRoom()->getEnemy();
    return enemy && enemy->isAlive();
}

void Game::handleWork() {
    if (hasLivingEnemyInRoom()) {
        std::cout << "You cannot work while fighting!\n";
        return;
    }
    if (player->getCurrentRoom()->getName() != "Village Square") {
        std::cout << "You can only work in the Village Square.\n";
        return;
    }
    if (player->getHealth() <= 3) {
        std::cout << "You are too worn down to work safely. \033[1;32mrest\033[0m first.\n";
        return;
    }
    int pay = 8;
    player->addGold(pay);
    player->takeDamage(3);
    std::cout << "You spend the afternoon helping the villagers.\n\n";
    std::cout << "You carry supply crates, repair worn tools, and prepare equipment for\n";
    std::cout << "the road ahead.\n\n";
    std::cout << "The work is tiring, but the villagers appreciate your effort.\n\n";
    std::cout << "You earned \033[1;33m" << pay << "g\033[0m.\n";
    std::cout << "Your body feels a little worn (\033[1;31m-3 HP\033[0m).\n\n";
    std::cout << "HP: \033[1;32m" << player->getHealth() << "/" << player->getMaxHealth() << "\033[0m\n";
}

void Game::handleRest() {
    if (hasLivingEnemyInRoom()) {
        std::cout << "You cannot rest while fighting!\n";
        return;
    }
    if (player->getCurrentRoom()->getName() != "Village Square") {
        std::cout << "You can only rest in the Village Square.\n";
        return;
    }
    player->setHealth(player->getMaxHealth());
    healFellowshipRoster();
    std::cout << "Your fellowship rests beside the old fountain in the village square.\n\n";
    std::cout << "The sound of flowing water slowly eases everyone's exhaustion.\n\n";
    std::cout << "For a moment, the weight of the journey feels distant.\n\n";
    std::cout << "You recover all HP. Your companions are fully healed as well.\n\n";
    std::cout << "HP: \033[1;32m" << player->getHealth() << "/" << player->getMaxHealth() << "\033[0m\n";
    if (!fellowshipRoster.empty()) {
        std::cout << "Fellowship restored to full health.\n";
    }
}

bool Game::isValerieEnemy(const Enemy* enemy) {
    return enemy && enemy->getName() == "Valerie";
}

bool Game::charmValerieWithRibbon() {
    if (player->getCurrentRoom()->getName() != "Rooftop Aviary") {
        return false;
    }
    Enemy* enemy = player->getCurrentRoom()->getEnemy();
    if (!enemy || !enemy->isAlive() || !isValerieEnemy(enemy)) {
        return false;
    }
    if (!player->removeItem("StarlightRibbon")) {
        std::cout << "You need the StarlightRibbon for this.\n";
        return false;
    }

    valerieCharmed = true;
    valerieReadyToRecruit = true;
    specialGiveUnlocked = true;
    specialRecruitUnlocked = true;
    enemy->setHealth(0);

    if (valerieNpc && !valerieNpcPlaced) {
        player->getCurrentRoom()->addNPC(valerieNpc);
        valerieNpcPlaced = true;
    }

    std::cout << "\nYou hold out the Starlight Ribbon.\n\n";
    std::cout << "The wind falters.\n\n";
    std::cout << "Valerie stops mid-motion.\n\n";
    std::cout << "Her eyes fix on it.\n\n";
    std::cout << "\033[1;35mValerie says:\033[0m \"...Where did you find that?\"\n\n";
    std::cout << "Her wings slowly fold inward.\n\n";
    std::cout << "The hostility in the air breaks.\n\n";
    std::cout << "For a moment, she says nothing.\n\n";
    std::cout << "Then:\n\n";
    std::cout << "\"...I thought this was lost.\"\n\n";
    std::cout << "She steps back from you.\n\n";
    std::cout << "\"You are not my enemy.\"\n\n";
    std::cout << "The storm around the ledge quiets slightly.\n\n";
    std::cout << "Valerie lowers her head.\n\n";
    std::cout << "\"If you wish to pass... I will not stop you.\"\n\n";
    std::cout << "She hesitates.\n\n";
    std::cout << "\"...I can take you beyond the walls.\"\n";
    std::cout << "\"Where the stairs no longer matter.\"\n\n";
    std::cout << "Say \033[1;32mrecruit valerie\033[0m if you want passage skyward.\n\n";
    return true;
}

bool Game::charmKnightWithRing() {
    if (player->getCurrentRoom()->getName() != "Tower Floor 1") {
        return false;
    }
    Enemy* enemy = player->getCurrentRoom()->getEnemy();
    if (!enemy || !enemy->isAlive() || !isArmoredKnightEnemy(enemy)) {
        return false;
    }
    if (!player->removeItem("EleanorsRing")) {
        std::cout << "You do not have Eleanor's ring.\n";
        return false;
    }

    garrisonCharmed = true;
    enemy->setName("Sir Garrison");
    enemy->setHealth(0);
    if (!isInParty("garrison")) {
        party.push_back("Sir Garrison");
        registerFellowshipMember("Sir Garrison");
    }

    std::cout << "\nYou offer the gold ring. The knight freezes.\n";
    std::cout << "He turns it in the torchlight. \"This crest... Eleanor's ring.\"\n";
    std::cout << "His weapon clatters. \"Garrison... I remember. I will not raise steel against you.\"\n\n";
    printParty();
    return true;
}

bool Game::getIsRunning() const { return isRunning; }
bool Game::hasGameWon() const { return gameWon; }
const Player* Game::getPlayer() const { return player; }
const std::vector<std::string>& Game::getFellowship() const { return party; }
int Game::getLoopCount() const { return loopCount; }

const std::string& Game::getLastTalkNpcName() const { return lastTalkNpcName; }

Item Game::createItemByName(const std::string& name) {
    const std::string key = normalizeItemKey(name);
    if (key == "rustysword") {
        return Item("RustySword", "An old, corroded iron sword. Better than nothing.", ItemType::WEAPON, 2, 5);
    }
    if (key == "steelsword") {
        return Item("SteelSword", "A well-forged steel longsword with a sharp edge.", ItemType::WEAPON, 6, 30);
    }
    if (key == "leatherarmor") {
        return Item("LeatherArmor", "A set of sturdy leather tunic and leggings.", ItemType::ARMOR, 2, 15);
    }
    if (key == "steelplate") {
        return Item("SteelPlate", "Heavy iron plate armor. Highly defensive.", ItemType::ARMOR, 5, 45);
    }
    if (key == "healthpotion") {
        return Item("HealthPotion", "A vial filled with shimmering red liquid. Restores 40 HP.", ItemType::CONSUMABLE, 40, 10);
    }
    if (key == "elixir") {
        return Item("Elixir", "An exquisite glowing gold draft. Restores 100 HP.", ItemType::CONSUMABLE, 100, 25);
    }
    if (key == "gatekey") {
        return Item("GateKey", "A heavy brass key shaped like a dragon's wing.", ItemType::KEY, 0, 0);
    }
    if (key == "rope") {
        return Item("Rope", "A long coil of hempen rope, sturdy enough to lower yourself into the well.", ItemType::KEY, 0, 5);
    }
    if (key == "starlightribbon") {
        return Item("StarlightRibbon", "A delicate ribbon that glimmers with pale moonlight.", ItemType::VALUABLE, 0, 20);
    }
    if (key == "ore") {
        return Item("Ore", "A chunk of rare forge ore suitable for master armor work.", ItemType::VALUABLE, 0, 25);
    }
    if (key == "bread") {
        return Item("Bread", "A stale loaf. Simple food, but it could still help someone hungry.", ItemType::CONSUMABLE, 10, 2);
    }
    if (key == "shieldofvaliance") {
        return Item("ShieldOfValiance", "A knight's shield engraved with a forgotten royal crest.", ItemType::ARMOR, 7, 60);
    }
    if (key == "azaelsdiary") {
        return Item("AzaelsDiary", "A worn journal revealing Azael's tragic past and lost humanity.", ItemType::KEY, 0, 0);
    }
    if (key == "thronekey") {
        return Item("ThroneKey", "A dark obsidian key that feels ice-cold to the touch.", ItemType::KEY, 0, 0);
    }
    if (key == "eleanorsring") {
        return Item("EleanorsRing", "A simple gold ring on a thin chain. It still holds a woman's warmth.", ItemType::KEY, 0, 0);
    }
    return Item();
}

void Game::applyStoryFlags(const GameSnapshot& snapshot) {
    knowsGarrisonStory = snapshot.knowsGarrisonStory;
    examinedGarrisonArmor = snapshot.examinedGarrisonArmor;
    eleanorTalkCount = snapshot.eleanorTalkCount;
    eleanorHeardAllDialogue = snapshot.eleanorHeardAllDialogue;
    eleanorRingGifted = snapshot.eleanorRingGifted;
    examinedFountain = snapshot.examinedFountain;
    lyraRiddleSolved = snapshot.lyraRiddleSolved;
    pipBreadGiven = snapshot.pipBreadGiven;
    pipPraised = snapshot.pipPraised;
    pipGenuineBond = snapshot.pipGenuineBond;
    pipBribed = snapshot.pipBribed;
    pipReadyToRecruit = snapshot.pipReadyToRecruit;
    pipRevealedBySearch = snapshot.pipRevealedBySearch;
    lyraRecruited = snapshot.lyraRecruited;
    bramRecruited = snapshot.bramRecruited;
    bramMasterwork = snapshot.bramMasterwork;
    bramReadyToRecruit = snapshot.bramReadyToRecruit;
    garrisonCharmed = snapshot.garrisonCharmed;
    valerieCharmed = snapshot.valerieCharmed;
    valerieReadyToRecruit = snapshot.valerieReadyToRecruit;
    valerieRecruited = snapshot.valerieRecruited;
    spirePacifist = snapshot.spirePacifist;
    lyraDisruptUsed = snapshot.lyraDisruptUsed;
    specialGiveUnlocked = snapshot.specialGiveUnlocked;
    specialPraiseUnlocked = snapshot.specialPraiseUnlocked;
    specialExamineUnlocked = snapshot.specialExamineUnlocked;
    specialRecruitUnlocked = snapshot.specialRecruitUnlocked;
    bramSavedGoblin = snapshot.bramSavedGoblin;
}

bool Game::captureSnapshot(GameSnapshot& out) const {
    if (!player || !player->getCurrentRoom()) {
        return false;
    }

    GameSnapshot snapshot;
    snapshot.loopCount = loopCount;
    snapshot.roomName = player->getCurrentRoom()->getName();
    snapshot.health = player->getHealth();
    snapshot.maxHealth = player->getMaxHealth();
    snapshot.gold = player->getGold();
    snapshot.baseAttack = 8;
    snapshot.baseDefense = 1;
    if (player->hasEquippedWeapon()) {
        snapshot.equippedWeapon = player->getEquippedWeapon().getName();
    }
    if (player->hasEquippedArmor()) {
        snapshot.equippedArmor = player->getEquippedArmor().getName();
    }
    for (const Item& item : player->getInventory()) {
        snapshot.inventory.push_back(item.getName());
    }
    snapshot.party = party;
    snapshot.knowsGarrisonStory = knowsGarrisonStory;
    snapshot.examinedGarrisonArmor = examinedGarrisonArmor;
    snapshot.eleanorTalkCount = eleanorTalkCount;
    snapshot.eleanorHeardAllDialogue = eleanorHeardAllDialogue;
    snapshot.eleanorRingGifted = eleanorRingGifted;
    snapshot.examinedFountain = examinedFountain;
    snapshot.lyraRiddleSolved = lyraRiddleSolved;
    snapshot.pipBreadGiven = pipBreadGiven;
    snapshot.pipPraised = pipPraised;
    snapshot.pipGenuineBond = pipGenuineBond;
    snapshot.pipBribed = pipBribed;
    snapshot.pipReadyToRecruit = pipReadyToRecruit;
    snapshot.pipRevealedBySearch = pipRevealedBySearch;
    snapshot.lyraRecruited = lyraRecruited;
    snapshot.bramRecruited = bramRecruited;
    snapshot.bramMasterwork = bramMasterwork;
    snapshot.bramReadyToRecruit = bramReadyToRecruit;
    snapshot.garrisonCharmed = garrisonCharmed;
    snapshot.valerieCharmed = valerieCharmed;
    snapshot.valerieReadyToRecruit = valerieReadyToRecruit;
    snapshot.valerieRecruited = valerieRecruited;
    snapshot.spirePacifist = spirePacifist;
    snapshot.lyraDisruptUsed = lyraDisruptUsed;
    snapshot.specialGiveUnlocked = specialGiveUnlocked;
    snapshot.specialPraiseUnlocked = specialPraiseUnlocked;
    snapshot.specialExamineUnlocked = specialExamineUnlocked;
    snapshot.specialRecruitUnlocked = specialRecruitUnlocked;
    snapshot.bramSavedGoblin = bramSavedGoblin;

    out = std::move(snapshot);
    return true;
}

void Game::queueSnapshotRestore(const GameSnapshot& snapshot) {
    pendingSnapshot_ = snapshot;
    hasPendingSnapshot_ = true;
}

bool Game::applyQueuedSnapshot() {
    if (!hasPendingSnapshot_) {
        return false;
    }

    const GameSnapshot snapshot = pendingSnapshot_;
    hasPendingSnapshot_ = false;

    delete player;
    player = nullptr;
    if (valerieNpc && !valerieNpcPlaced) {
        delete valerieNpc;
    }
    for (Room* room : rooms) {
        delete room;
    }
    rooms.clear();
    valerieNpc = nullptr;
    valerieNpcPlaced = false;

    resetStoryState();
    loopCount = snapshot.loopCount;
    applyStoryFlags(snapshot);
    party = snapshot.party;
    for (const std::string& memberName : party) {
        registerFellowshipMember(memberName);
    }

    initializeMap();

    player->applyPersistentState(
        snapshot.health,
        snapshot.maxHealth,
        snapshot.gold,
        snapshot.baseAttack,
        snapshot.baseDefense);

    Room* room = findRoomByName(snapshot.roomName);
    if (room) {
        player->setCurrentRoom(room);
    }

    for (const std::string& itemName : snapshot.inventory) {
        Item item = createItemByName(itemName);
        if (!item.getName().empty()) {
            player->addInventoryItem(item);
        }
    }
    if (!snapshot.equippedWeapon.empty()) {
        player->equipItem(snapshot.equippedWeapon);
    }
    if (!snapshot.equippedArmor.empty()) {
        player->equipItem(snapshot.equippedArmor);
    }

    return true;
}

void Game::resolveCombatEnding() {
    if (pipBribed && isInParty("pip")) {
        endingType = EndingType::BETRAYAL_ENDING;
        return;
    }

    bool fullFellowship = isInParty("pip") && pipGenuineBond && lyraRecruited && bramRecruited
        && bramMasterwork && garrisonCharmed && valerieRecruited;
    if (fullFellowship) {
        endingType = EndingType::TRUE_ENDING;
    } else {
        endingType = EndingType::BITTERSWEET_ENDING;
    }
}
