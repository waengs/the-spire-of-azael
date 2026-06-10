#ifndef GAMEAPPLICATION_H
#define GAMEAPPLICATION_H

#include "Game.h"
#include "SaveDatabase.h"
#include "UiAssets.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct SDL_Window;
struct ImFont;

class GameApplication {
public:
    GameApplication();
    ~GameApplication();

    int run();

private:
    bool initSdlGl();
    void shutdownSdlGl();
    void updateDisplayMetrics();
    void startGameThread();
    void stopGameThread();

    void appendOutput(const std::string& chunk);
    std::string waitForInput(const std::string& prompt);
    void submitCommand(const std::string& command);
    static std::string stripAnsi(const std::string& text);

    struct StatusCache {
        std::string location;
        int health = 0;
        int maxHealth = 0;
        int attack = 0;
        int defense = 0;
        int gold = 0;
        int loopCount = 1;
        std::vector<std::string> fellowship;
        std::vector<std::string> inventory;
        std::vector<std::string> exits;
        std::string speakingNpc;
    };

    void drawUi();
    void drawMainMenu();
    void drawGameplay();
    void drawSavePanel();
    void drawMapPanel(const StatusCache& cache);
    void drawSpeakerPanel(const StatusCache& cache);
    void refreshStatusCache();
    void loadUiSettings();
    void saveUiSettings() const;
    void queueFontReload();
    void applyPendingFontScale();
    void updateLogReveal();
    void snapLogRevealToEnd();
    void drawDisplaySettings();
    void adjustFontScale(float delta);
    float defaultFontScaleForDisplay() const;

    bool saveToSlot(int slot);
    bool loadFromSlot(int slot);

    SDL_Window* window_ = nullptr;
    void* glContext_ = nullptr;
    ImFont* fontUi_ = nullptr;
    ImFont* fontMenuTitle_ = nullptr;
    ImFont* fontGameplay_ = nullptr;
    ImFont* fontMenuBody_ = nullptr;
    float fontUserScale_ = 1.0f;
    bool typewriterEnabled_ = true;
    bool pendingFontReload_ = false;

    Game game_;
    SaveDatabase database_;
    UiAssets uiAssets_;

    std::thread gameThread_;
    std::atomic<bool> gameThreadRunning_{false};
    std::atomic<bool> gameFinished_{false};
    std::atomic<bool> shutdownRequested_{false};

    std::mutex logMutex_;
    std::string logText_;
    size_t logRevealPos_ = 0;
    std::uint64_t logRevealPauseUntil_ = 0;
    std::uint64_t lastRevealTick_ = 0;
    bool logScrollToBottom_ = true;

    std::mutex inputMutex_;
    std::condition_variable inputReady_;
    std::string pendingInput_;
    bool hasInput_ = false;
    bool waitingForInput_ = false;
    std::atomic<bool> requestCommandFocus_{false};
    std::string promptText_ = ">";

    char commandBuffer_[512] = {};

    enum class AppState { MainMenu, Playing };
    AppState appState_ = AppState::MainMenu;

    GameSnapshot bootSnapshot_;
    bool bootWithSnapshot_ = false;

    int selectedSaveSlot_ = 1;
    char saveLabelBuffer_[64] = "Adventure";

    StatusCache status_;
    std::mutex statusMutex_;
};

#endif
