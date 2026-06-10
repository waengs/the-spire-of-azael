#include "GameApplication.h"
#include "GameIo.h"
#include "AnsiText.h"
#include "UiFonts.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

#include "third_party/stb_image.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

static void drawInlineHpBar(float ratio) {
    const float barW = 72.0f;
    const float barH = 5.0f;
    const float lineH = ImGui::GetTextLineHeight();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.y += (lineH - barH) * 0.5f;

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 bg = IM_COL32(40, 24, 58, 255);
    const ImU32 fill = IM_COL32(70, 190, 95, 255);
    const ImU32 border = IM_COL32(90, 60, 120, 255);
    const ImVec2 end(pos.x + barW, pos.y + barH);
    draw->AddRectFilled(pos, end, bg, 1.0f);
    if (ratio > 0.0f) {
        draw->AddRectFilled(pos, ImVec2(pos.x + barW * ratio, end.y), fill, 1.0f);
    }
    draw->AddRect(pos, end, border, 1.0f);
    ImGui::Dummy(ImVec2(barW, lineH));
}

static void centerTextInWidth(const char* text, float containerWidth = -1.0f) {
    if (containerWidth < 0.0f) {
        containerWidth = ImGui::GetContentRegionAvail().x;
    }
    const float baseX = ImGui::GetCursorPosX();
    const float textWidth = ImGui::CalcTextSize(text).x;
    if (textWidth > containerWidth - 2.0f) {
        ImGui::PushTextWrapPos(baseX + containerWidth);
        ImGui::TextWrapped("%s", text);
        ImGui::PopTextWrapPos();
        return;
    }
    ImGui::SetCursorPosX(baseX + std::max(0.0f, (containerWidth - textWidth) * 0.5f));
    ImGui::TextUnformatted(text);
}

std::string trimCopy(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

} // namespace

namespace {

std::vector<std::filesystem::path> assetSearchRoots() {
    std::vector<std::filesystem::path> roots;
    roots.emplace_back("assets");
    roots.emplace_back("../assets");
    roots.emplace_back("../../assets");
    char* base = SDL_GetBasePath();
    if (base) {
        std::filesystem::path exeDir(base);
        SDL_free(base);
        roots.push_back(exeDir / "assets");
        roots.push_back(exeDir.parent_path() / "assets");
    }
    return roots;
}

void applyWindowIcon(SDL_Window* window) {
    if (!window) {
        return;
    }

    for (const std::filesystem::path& root : assetSearchRoots()) {
        const std::filesystem::path logoPath = root / "logo.png";
        if (!std::filesystem::exists(logoPath)) {
            continue;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load(logoPath.string().c_str(), &width, &height, &channels, 4);
        if (!pixels || width <= 0 || height <= 0) {
            stbi_image_free(pixels);
            return;
        }

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
            0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
        if (surface && surface->pixels) {
            std::memcpy(surface->pixels, pixels, static_cast<size_t>(width) * height * 4);
            SDL_SetWindowIcon(window, surface);
            SDL_FreeSurface(surface);
        }
        stbi_image_free(pixels);
        return;
    }
}

std::filesystem::path uiSettingsPath() {
    char* base = SDL_GetBasePath();
    if (!base) {
        return "spire_ui.ini";
    }
    std::filesystem::path path(base);
    SDL_free(base);
    return path / "spire_ui.ini";
}

} // namespace

GameApplication::GameApplication()
    : database_("spire_save.db") {}

GameApplication::~GameApplication() {
    stopGameThread();
    uiAssets_.shutdown();
    shutdownSdlGl();
}

bool GameApplication::initSdlGl() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        return false;
    }

#ifdef _WIN32
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window_ = SDL_CreateWindow(
        "The Spire of Azael",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        return false;
    }

    applyWindowIcon(window_);

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_) {
        return false;
    }
    SDL_GL_MakeCurrent(window_, glContext_);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "imgui.ini";

    applySpireUiTheme();

    ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 330");

    updateDisplayMetrics();
    loadUiSettings();
    if (!setupSpirePixelFonts(fontUi_, fontMenuTitle_, fontGameplay_, fontMenuBody_, fontUserScale_)) {
        return false;
    }

    uiAssets_.init();
    return true;
}

float GameApplication::defaultFontScaleForDisplay() const {
    if (!window_) {
        return 1.0f;
    }

    const int displayIndex = SDL_GetWindowDisplayIndex(window_);
    float ddpi = 96.0f;
    if (SDL_GetDisplayDPI(displayIndex, &ddpi, nullptr, nullptr) != 0) {
        return 1.0f;
    }

    return std::clamp(ddpi / 96.0f, 1.0f, 2.0f);
}

void GameApplication::loadUiSettings() {
    const std::filesystem::path path = uiSettingsPath();
    std::ifstream input(path);
    if (!input) {
        fontUserScale_ = defaultFontScaleForDisplay();
        return;
    }

    bool foundScale = false;
    std::string line;
    while (std::getline(input, line)) {
        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        const std::string key = trimCopy(line.substr(0, eq));
        const std::string value = trimCopy(line.substr(eq + 1));
        if (key == "font_scale") {
            try {
                fontUserScale_ = std::clamp(std::stof(value), 0.75f, 2.5f);
                foundScale = true;
            } catch (...) {
            }
        } else if (key == "typewriter") {
            typewriterEnabled_ = (value == "1" || value == "true" || value == "yes" || value == "on");
        }
    }

    if (!foundScale) {
        fontUserScale_ = defaultFontScaleForDisplay();
    }
}

void GameApplication::saveUiSettings() const {
    const std::filesystem::path path = uiSettingsPath();
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        return;
    }
    output << "font_scale=" << fontUserScale_ << '\n';
    output << "typewriter=" << (typewriterEnabled_ ? 1 : 0) << '\n';
}

void GameApplication::queueFontReload() {
    pendingFontReload_ = true;
}

void GameApplication::applyPendingFontScale() {
    if (!pendingFontReload_) {
        return;
    }
    pendingFontReload_ = false;

    SDL_GL_MakeCurrent(window_, glContext_);
    setupSpirePixelFonts(fontUi_, fontMenuTitle_, fontGameplay_, fontMenuBody_, fontUserScale_);
}

void GameApplication::adjustFontScale(float delta) {
    const float next = std::clamp(fontUserScale_ + delta, 0.75f, 2.5f);
    if (std::fabs(next - fontUserScale_) < 0.01f) {
        return;
    }
    fontUserScale_ = next;
    queueFontReload();
    saveUiSettings();
}

void GameApplication::drawDisplaySettings() {
    if (ImGui::CollapsingHeader("Display")) {
        char scaleLabel[16];
        std::snprintf(scaleLabel, sizeof(scaleLabel), "%.2fx", fontUserScale_);
        ImGui::TextUnformatted("Text size");
        ImGui::SameLine();
        ImGui::TextUnformatted(scaleLabel);

        float scale = fontUserScale_;
        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::SliderFloat("##text_size", &scale, 0.75f, 2.5f, "")) {
            fontUserScale_ = std::clamp(scale, 0.75f, 2.5f);
            queueFontReload();
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            saveUiSettings();
        }

        if (ImGui::Checkbox("Line-by-line text reveal", &typewriterEnabled_)) {
            if (!typewriterEnabled_) {
                snapLogRevealToEnd();
            }
            saveUiSettings();
        }

        ImGui::TextWrapped("Ctrl + mouse wheel to resize text.");
    }
}

void GameApplication::updateLogReveal() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (!typewriterEnabled_) {
        if (logRevealPos_ < logText_.size()) {
            logRevealPos_ = logText_.size();
            logScrollToBottom_ = true;
        }
        return;
    }

    const Uint64 now = SDL_GetTicks64();
    if (now < logRevealPauseUntil_ || logRevealPos_ >= logText_.size()) {
        return;
    }

    constexpr Uint64 kLineRevealMs = 110;
    if (lastRevealTick_ != 0 && now - lastRevealTick_ < kLineRevealMs) {
        return;
    }
    lastRevealTick_ = now;

    size_t next = logRevealPos_;
    while (next < logText_.size() && logText_[next] != '\n') {
        ++next;
    }
    if (next < logText_.size()) {
        ++next;
    } else {
        next = logText_.size();
    }
    logRevealPos_ = next;

    if (logRevealPos_ >= 2 && logText_[logRevealPos_ - 1] == '\n'
        && logText_[logRevealPos_ - 2] == '\n') {
        logRevealPauseUntil_ = now + 650;
    }
    logScrollToBottom_ = true;
}

void GameApplication::snapLogRevealToEnd() {
    std::lock_guard<std::mutex> lock(logMutex_);
    logRevealPos_ = logText_.size();
    logRevealPauseUntil_ = 0;
}

void GameApplication::updateDisplayMetrics() {
    if (!window_) {
        return;
    }

    int windowW = 0;
    int windowH = 0;
    int drawableW = 0;
    int drawableH = 0;
    SDL_GetWindowSize(window_, &windowW, &windowH);
    SDL_GL_GetDrawableSize(window_, &drawableW, &drawableH);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(windowW), static_cast<float>(windowH));
    if (windowW > 0 && windowH > 0) {
        io.DisplayFramebufferScale = ImVec2(
            static_cast<float>(drawableW) / static_cast<float>(windowW),
            static_cast<float>(drawableH) / static_cast<float>(windowH));
    } else {
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    }
}

void GameApplication::shutdownSdlGl() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    if (ImGui::GetCurrentContext()) {
        ImGui::DestroyContext();
    }

    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void GameApplication::startGameThread() {
    if (gameThreadRunning_.load()) {
        return;
    }

    if (gameThread_.joinable()) {
        gameThread_.join();
    }

    gameFinished_ = false;
    gameThreadRunning_ = true;

    GameIo::installGui(
        [this](const std::string& chunk) { appendOutput(chunk); },
        [this](const std::string& prompt) { return waitForInput(prompt); });

    gameThread_ = std::thread([this]() {
        if (bootWithSnapshot_) {
            game_.queueSnapshotRestore(bootSnapshot_);
            bootWithSnapshot_ = false;
        }
        game_.start();
        GameIo::shutdown();
        gameFinished_ = true;
        gameThreadRunning_ = false;
    });
}

void GameApplication::stopGameThread() {
    if (!gameThread_.joinable()) {
        gameThreadRunning_ = false;
        waitingForInput_ = false;
        return;
    }

    if (gameThreadRunning_.load()) {
        {
            std::lock_guard<std::mutex> lock(inputMutex_);
            if (waitingForInput_) {
                pendingInput_ = "quit";
                hasInput_ = true;
                inputReady_.notify_one();
            }
        }
        gameThread_.join();
    } else if (gameThread_.joinable()) {
        gameThread_.join();
    }

    GameIo::shutdown();
    gameThreadRunning_ = false;
    waitingForInput_ = false;
}

std::string GameApplication::stripAnsi(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        if (text[i] == '\x1B' && i + 1 < text.size() && text[i + 1] == '[') {
            i += 2;
            while (i < text.size() && !std::isalpha(static_cast<unsigned char>(text[i]))) {
                ++i;
            }
            if (i < text.size()) {
                ++i;
            }
            continue;
        }
        out.push_back(text[i]);
        ++i;
    }
    return out;
}

void GameApplication::appendOutput(const std::string& chunk) {
    if (chunk.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(logMutex_);
    const bool wasCaughtUp = logRevealPos_ >= logText_.size();
    logText_ += chunk;
    if (!typewriterEnabled_) {
        logRevealPos_ = logText_.size();
        logRevealPauseUntil_ = 0;
    } else if (wasCaughtUp) {
        lastRevealTick_ = 0;
        logRevealPauseUntil_ = 0;
    }
    logScrollToBottom_ = true;
}

std::string GameApplication::waitForInput(const std::string& prompt) {
    GameIo::flushOutput();

    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        hasInput_ = false;
        pendingInput_.clear();
        waitingForInput_ = true;
        requestCommandFocus_ = true;
        promptText_ = trimCopy(prompt);
        if (promptText_.empty()) {
            promptText_ = ">";
        }
    }

    std::unique_lock<std::mutex> lock(inputMutex_);
    inputReady_.wait(lock, [this]() { return hasInput_ || gameFinished_.load(); });

    waitingForInput_ = false;
    if (gameFinished_.load()) {
        return "quit";
    }

    const std::string line = pendingInput_;
    hasInput_ = false;
    pendingInput_.clear();
    return line;
}

void GameApplication::submitCommand(const std::string& command) {
    const std::string trimmed = trimCopy(command);
    if (trimmed.empty()) {
        return;
    }

    appendOutput("\033[1;34m> \033[1;33m" + trimmed + "\033[0m\n");
    snapLogRevealToEnd();

    std::lock_guard<std::mutex> lock(inputMutex_);
    if (!waitingForInput_) {
        appendOutput("(Wait for the game prompt before sending a command.)\n");
        return;
    }

    pendingInput_ = trimmed;
    hasInput_ = true;
    inputReady_.notify_one();
}

void GameApplication::refreshStatusCache() {
    StatusCache cache;
    const Player* player = game_.getPlayer();
    if (player && player->getCurrentRoom()) {
        cache.location = player->getCurrentRoom()->getName();
        cache.health = player->getHealth();
        cache.maxHealth = player->getMaxHealth();
        cache.attack = player->getAttackPower();
        cache.defense = player->getDefensePower();
        cache.gold = player->getGold();
        cache.loopCount = game_.getLoopCount();
        cache.fellowship = game_.getFellowship();
        cache.speakingNpc = game_.getLastTalkNpcName();

        for (const auto& exitPair : player->getCurrentRoom()->getExits()) {
            cache.exits.push_back(exitPair.first);
        }

        if (player->hasEquippedWeapon()) {
            cache.inventory.push_back("[W] " + player->getEquippedWeapon().getName());
        }
        if (player->hasEquippedArmor()) {
            cache.inventory.push_back("[A] " + player->getEquippedArmor().getName());
        }
        for (const Item& item : player->getInventory()) {
            cache.inventory.push_back(item.getName());
        }
    }

    std::lock_guard<std::mutex> lock(statusMutex_);
    status_ = std::move(cache);
}

bool GameApplication::saveToSlot(int slot) {
    GameSnapshot snapshot;
    if (!game_.captureSnapshot(snapshot)) {
        appendOutput("Could not capture a save snapshot right now.\n");
        return false;
    }

    const std::string label = trimCopy(saveLabelBuffer_);
    if (!database_.saveSlot(slot, snapshot, label.empty() ? "Adventure" : label)) {
        appendOutput("Save failed: " + database_.lastError() + "\n");
        return false;
    }

    database_.setMetaLoopCount(snapshot.loopCount);
    appendOutput("Saved to slot " + std::to_string(slot) + ".\n");
    return true;
}

bool GameApplication::loadFromSlot(int slot) {
    const std::optional<GameSnapshot> snapshot = database_.loadSlot(slot);
    if (!snapshot) {
        appendOutput("No save found in slot " + std::to_string(slot) + ".\n");
        return false;
    }

    game_.queueSnapshotRestore(*snapshot);
    appendOutput("Queued slot " + std::to_string(slot) + " — applies at the next command prompt.\n");
    return true;
}

void GameApplication::drawMainMenu() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float layoutScale = std::clamp(display.x / 1280.0f, 1.0f, 1.6f);

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(display);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(
        "MainMenu",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetWindowPos();
    const ImVec2 p1 = ImVec2(p0.x + display.x, p0.y + display.y);
    draw->AddRectFilled(p0, p1, IM_COL32(26, 13, 46, 255));

    ImTextureID menuBg = uiAssets_.texture("bg_main_menu");
    if (menuBg) {
        draw->AddImage(menuBg, p0, p1);
    }

    const char* title = "The Spire Of Azael";
    const char* subtitle = "A journey through darkness, destiny, and forgotten memories.";

    if (fontMenuTitle_) {
        ImGui::PushFont(fontMenuTitle_);
    }
    const float titleWidth = ImGui::CalcTextSize(title).x + 40.0f * layoutScale;
    if (fontMenuTitle_) {
        ImGui::PopFont();
    }
    if (fontMenuBody_) {
        ImGui::PushFont(fontMenuBody_);
    }
    const float subtitleMinWidth = std::min(ImGui::CalcTextSize(subtitle).x + 24.0f, display.x * 0.9f);
    if (fontMenuBody_) {
        ImGui::PopFont();
    }

    float panelWidth = std::max({display.x * 0.88f, titleWidth, subtitleMinWidth, 640.0f * layoutScale});
    panelWidth = std::min(panelWidth, display.x - 24.0f);
    const float buttonWidth = std::min(420.0f * layoutScale, panelWidth - 48.0f * layoutScale);
    const float newGameHeight = 52.0f * layoutScale;
    const float slotHeight = 40.0f * layoutScale;
    const float startX = (display.x - panelWidth) * 0.5f;
    const float startY = display.y * 0.11f;

    ImGui::SetCursorPos(ImVec2(startX, startY));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 14.0f * layoutScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(18.0f * layoutScale, 12.0f * layoutScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f * layoutScale);

    ImGui::BeginChild("MenuPanel", ImVec2(panelWidth, display.y * 0.86f), false,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    if (fontMenuBody_) {
        ImGui::PushFont(fontMenuBody_);
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.93f, 0.98f, 1.0f));
    if (fontMenuTitle_) {
        ImGui::PushFont(fontMenuTitle_);
    }
    centerTextInWidth(title, panelWidth);
    if (fontMenuTitle_) {
        ImGui::PopFont();
    }
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 8.0f * layoutScale));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.78f, 0.92f, 1.0f));
    centerTextInWidth(subtitle, panelWidth);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 28.0f * layoutScale));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.482f, 0.357f, 0.655f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.42f, 0.73f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.30f, 0.58f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.97f, 1.0f, 1.0f));

    ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
    if (ImGui::Button("New Game", ImVec2(buttonWidth, newGameHeight))) {
        stopGameThread();
        bootWithSnapshot_ = false;
        game_.resetForNewAdventure();
        {
            std::lock_guard<std::mutex> lock(logMutex_);
            logText_.clear();
            logRevealPos_ = 0;
            logRevealPauseUntil_ = 0;
            lastRevealTick_ = 0;
        }
        appState_ = AppState::Playing;
        startGameThread();
    }

    ImGui::Dummy(ImVec2(0, 22.0f * layoutScale));
    ImGui::PopStyleColor(4);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.86f, 0.96f, 1.0f));
    centerTextInWidth("Load save slot", panelWidth);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 6.0f * layoutScale));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.482f, 0.357f, 0.655f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.42f, 0.73f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.30f, 0.58f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.97f, 1.0f, 1.0f));

    for (int slot = 1; slot <= 3; ++slot) {
        ImGui::PushID(slot);
        const std::string label = database_.slotLabel(slot);
        const std::string buttonText = "Slot " + std::to_string(slot) + ": " + label;
        ImGui::SetCursorPosX((panelWidth - buttonWidth) * 0.5f);
        if (ImGui::Button(buttonText.c_str(), ImVec2(buttonWidth, slotHeight))) {
            const std::optional<GameSnapshot> snapshot = database_.loadSlot(slot);
            if (!snapshot) {
                appendOutput("Slot is empty.\n");
            } else {
                stopGameThread();
                {
                    std::lock_guard<std::mutex> lock(logMutex_);
                    logText_.clear();
                    logRevealPos_ = 0;
                    logRevealPauseUntil_ = 0;
                    lastRevealTick_ = 0;
                }
                bootSnapshot_ = *snapshot;
                bootWithSnapshot_ = true;
                appState_ = AppState::Playing;
                startGameThread();
            }
        }
        ImGui::Dummy(ImVec2(0, 8.0f * layoutScale));
        ImGui::PopID();
    }
    ImGui::PopStyleColor(4);

    ImGui::Dummy(ImVec2(0, 12.0f * layoutScale));
    drawDisplaySettings();

    if (fontMenuBody_) {
        ImGui::PopFont();
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleVar();
    ImGui::End();
}

void GameApplication::drawMapPanel(const StatusCache& cache) {
    ImGui::TextUnformatted("Map");
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const ImVec2 center(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
    const float arm = (std::min)(canvasSize.x, canvasSize.y) * 0.30f;
    const float nodeRadius = 4.0f;
    const ImU32 lineActive = IM_COL32(235, 235, 245, 255);
    const ImU32 lineDim = IM_COL32(70, 50, 95, 140);
    const ImU32 nodeDim = IM_COL32(55, 38, 78, 255);
    const ImU32 youDotColor = IM_COL32(235, 55, 75, 255);
    const ImU32 youTextColor = IM_COL32(255, 252, 255, 255);
    const ImU32 youTextOutline = IM_COL32(18, 8, 32, 255);
    const ImU32 labelDim = IM_COL32(140, 125, 165, 255);
    const ImU32 northColor = IM_COL32(95, 220, 245, 255);
    const ImU32 southColor = IM_COL32(245, 205, 85, 255);
    const ImU32 eastColor = IM_COL32(85, 225, 115, 255);
    const ImU32 westColor = IM_COL32(205, 130, 245, 255);

    auto hasExit = [&](const char* dir) {
        return std::find(cache.exits.begin(), cache.exits.end(), dir) != cache.exits.end();
    };

    const ImVec2 north(center.x, center.y - arm);
    const ImVec2 south(center.x, center.y + arm);
    const ImVec2 east(center.x + arm, center.y);
    const ImVec2 west(center.x - arm, center.y);

    auto drawArm = [&](ImVec2 tip, bool active) {
        draw->AddLine(center, tip, active ? lineActive : lineDim, active ? 2.0f : 1.0f);
    };
    drawArm(north, hasExit("north"));
    drawArm(south, hasExit("south"));
    drawArm(east, hasExit("east"));
    drawArm(west, hasExit("west"));

    auto drawLabel = [&](ImVec2 anchor, const char* label, ImU32 color) {
        const ImVec2 size = ImGui::CalcTextSize(label);
        draw->AddText(ImVec2(anchor.x - size.x * 0.5f, anchor.y - size.y * 0.5f), color, label);
    };

    auto drawOutlinedLabel = [&](ImVec2 anchor, const char* label, ImU32 fill, ImU32 outline) {
        const ImVec2 size = ImGui::CalcTextSize(label);
        const ImVec2 pos(anchor.x - size.x * 0.5f, anchor.y - size.y * 0.5f);
        static constexpr ImVec2 kOutlineOffsets[] = {
            ImVec2(-1.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(0.0f, -1.0f), ImVec2(0.0f, 1.0f),
            ImVec2(-1.0f, -1.0f), ImVec2(1.0f, -1.0f), ImVec2(-1.0f, 1.0f), ImVec2(1.0f, 1.0f),
        };
        for (const ImVec2& offset : kOutlineOffsets) {
            draw->AddText(ImVec2(pos.x + offset.x, pos.y + offset.y), outline, label);
        }
        draw->AddText(pos, fill, label);
    };

    draw->AddCircleFilled(center, nodeRadius + 1.0f, youDotColor, 12);
    drawOutlinedLabel(
        ImVec2(center.x, center.y + nodeRadius + 9.0f), "YOU", youTextColor, youTextOutline);

    auto drawDirectionExit = [&](ImVec2 pos, const char* label, ImVec2 labelPos, bool active, ImU32 color) {
        draw->AddCircleFilled(pos, nodeRadius, active ? color : nodeDim, 10);
        drawLabel(labelPos, label, active ? color : labelDim);
    };

    drawDirectionExit(north, "N", ImVec2(north.x, north.y - 11.0f), hasExit("north"), northColor);
    drawDirectionExit(south, "S", ImVec2(south.x, south.y + 11.0f), hasExit("south"), southColor);
    drawDirectionExit(east, "E", ImVec2(east.x + 11.0f, east.y), hasExit("east"), eastColor);
    drawDirectionExit(west, "W", ImVec2(west.x - 11.0f, west.y), hasExit("west"), westColor);

    ImGui::Dummy(canvasSize);
}

void GameApplication::drawSpeakerPanel(const StatusCache& cache) {
    const float panelWidth = ImGui::GetContentRegionAvail().x;
    centerTextInWidth("Last Spoke", panelWidth);
    const float panelHeight = ImGui::GetContentRegionAvail().y;

    constexpr float kPortraitSize = 68.0f;
    const float lineH = ImGui::GetTextLineHeightWithSpacing();
    const float contentH = cache.speakingNpc.empty()
        ? lineH * 2.0f
        : lineH + 6.0f + kPortraitSize + 4.0f + ImGui::GetTextLineHeight();
    const float padY = std::max(0.0f, (panelHeight - contentH) * 0.5f);

    ImGui::Dummy(ImVec2(0.0f, padY));

    if (cache.speakingNpc.empty()) {
        centerTextInWidth("—", panelWidth);
        return;
    }

    ImTextureID speaker = uiAssets_.portraitForNpc(cache.speakingNpc);
    if (speaker) {
        const float portraitX = std::max(0.0f, (panelWidth - kPortraitSize) * 0.5f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + portraitX);
        ImGui::Image(speaker, ImVec2(kPortraitSize, kPortraitSize));
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    centerTextInWidth(cache.speakingNpc.c_str(), panelWidth);
}

void GameApplication::drawSavePanel() {
    ImGui::TextUnformatted("Save");
    ImGui::InputText("Label", saveLabelBuffer_, sizeof(saveLabelBuffer_));
    ImGui::SliderInt("Slot", &selectedSaveSlot_, 1, 3);

    const float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
    if (ImGui::Button("Save", ImVec2(buttonWidth, 0))) {
        saveToSlot(selectedSaveSlot_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", ImVec2(buttonWidth, 0))) {
        loadFromSlot(selectedSaveSlot_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete", ImVec2(buttonWidth, 0))) {
        if (database_.deleteSlot(selectedSaveSlot_)) {
            appendOutput("Deleted slot " + std::to_string(selectedSaveSlot_) + ".\n");
        }
    }

    ImGui::TextWrapped("%s", database_.slotLabel(selectedSaveSlot_).c_str());
}

void GameApplication::drawGameplay() {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float leftWidth = display.x * 0.66f;
    const float rightWidth = display.x - leftWidth;
    const bool canType = waitingForInput_ && !gameFinished_.load();

    StatusCache cache;
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        cache = status_;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.015f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.02f, 0.08f, 1.0f));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(leftWidth, display.y));
    ImGui::Begin(
        "LogPanel",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    if (fontGameplay_) {
        ImGui::PushFont(fontGameplay_);
    }

    const ImVec2 storySize = ImVec2(0, display.y - 86.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.01f, 0.05f, 1.0f));
    ImGui::BeginChild("StoryFrame", storySize, true, ImGuiWindowFlags_NoScrollbar);
    const ImVec2 bgPos = ImGui::GetCursorScreenPos();
    const ImVec2 bgSize = ImGui::GetContentRegionAvail();
    const ImVec2 bgEnd(bgPos.x + bgSize.x, bgPos.y + bgSize.y);
    ImDrawList* storyDraw = ImGui::GetWindowDrawList();
    storyDraw->AddRectFilled(bgPos, bgEnd, IM_COL32(6, 3, 14, 255));
    ImTextureID storyBg = uiAssets_.texture("bg_story_panel");
    if (storyBg) {
        storyDraw->AddImage(storyBg, bgPos, bgEnd);
    }
    storyDraw->AddRectFilled(bgPos, bgEnd, IM_COL32(0, 0, 0, 150));

    ImGui::SetCursorScreenPos(
        ImVec2(std::floor(bgPos.x + 12.0f), std::floor(bgPos.y + 12.0f)));
    ImGui::BeginChild(
        "StoryText",
        ImVec2(bgSize.x - 24.0f, bgSize.y - 24.0f),
        false,
        ImGuiWindowFlags_NoBackground);
    std::string visibleLog;
    bool scrollToBottom = false;
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        visibleLog = logText_.substr(0, logRevealPos_);
        scrollToBottom = logScrollToBottom_;
        if (scrollToBottom) {
            logScrollToBottom_ = false;
        }
    }
    const ImVec4 defaultLogColor(0.98f, 0.97f, 1.0f, 1.0f);
    const float wrapWidth = std::max(32.0f, ImGui::GetContentRegionAvail().x);
    drawAnsiTextWrapped(visibleLog, wrapWidth, defaultLogColor);
    if (scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    if (!canType) {
        ImGui::BeginDisabled();
    } else if (requestCommandFocus_.load()) {
        ImGui::SetKeyboardFocusHere();
        requestCommandFocus_ = false;
    }
    const bool submit = ImGui::InputTextWithHint(
        "##command",
        "What will you do?",
        commandBuffer_,
        sizeof(commandBuffer_),
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if (ImGui::Button("Send", ImVec2(72, 0)) || submit) {
        submitCommand(commandBuffer_);
        commandBuffer_[0] = '\0';
        requestCommandFocus_ = true;
    }
    if (!canType) {
        ImGui::EndDisabled();
    }

    if (fontGameplay_) {
        ImGui::PopFont();
    }
    ImGui::End();
    ImGui::PopStyleColor(2);

    ImGui::SetNextWindowPos(ImVec2(leftWidth, 0));
    ImGui::SetNextWindowSize(ImVec2(rightWidth, display.y));
    ImGui::Begin(
        "SidePanel",
        nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse);

    if (fontUi_) {
        ImGui::PushFont(fontUi_);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.76f, 0.98f, 1.0f));
    const char* recordTitle = "ADVENTURER'S RECORD";
    centerTextInWidth(recordTitle, ImGui::GetContentRegionAvail().x);
    ImGui::PopStyleColor();
    ImGui::Separator();

    constexpr ImGuiWindowFlags kHudChildFlags =
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("RecordRow", ImVec2(0, 108), true, kHudChildFlags);
    const float hpRatio = cache.maxHealth > 0
        ? static_cast<float>(cache.health) / static_cast<float>(cache.maxHealth)
        : 0.0f;

    constexpr float kHeroPortrait = 56.0f;
    char hpText[32];
    std::snprintf(hpText, sizeof(hpText), "%d / %d", cache.health, cache.maxHealth);
    char goldText[32];
    std::snprintf(goldText, sizeof(goldText), "Gold: %dg", cache.gold);
    char atkDefText[48];
    std::snprintf(atkDefText, sizeof(atkDefText), "ATK: %d  DEF: %d", cache.attack, cache.defense);

    const float statsW = std::max({
        ImGui::CalcTextSize("Hero").x,
        ImGui::CalcTextSize(goldText).x,
        ImGui::CalcTextSize("HP").x + 4.0f + ImGui::CalcTextSize(hpText).x + 8.0f + 72.0f,
        ImGui::CalcTextSize(atkDefText).x});
    const float blockW = kHeroPortrait + ImGui::GetStyle().ItemSpacing.x + statsW;
    const float lineH = ImGui::GetTextLineHeightWithSpacing();
    const float blockH = std::max(kHeroPortrait, lineH * 3.0f + ImGui::GetTextLineHeight());
    const ImVec2 recordAvail = ImGui::GetContentRegionAvail();
    const float padX = std::max(0.0f, (recordAvail.x - blockW) * 0.5f);
    const float padY = std::max(0.0f, (recordAvail.y - blockH) * 0.5f);

    ImGui::Dummy(ImVec2(0.0f, padY));
    ImGui::Indent(padX);
    ImTextureID heroPortrait = uiAssets_.texture("portrait_hero");
    if (heroPortrait) {
        ImGui::Image(heroPortrait, ImVec2(kHeroPortrait, kHeroPortrait));
    } else {
        ImGui::Dummy(ImVec2(kHeroPortrait, kHeroPortrait));
    }
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Hero");
    ImGui::TextUnformatted(goldText);
    ImGui::TextUnformatted("HP");
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::TextUnformatted(hpText);
    ImGui::SameLine(0.0f, 8.0f);
    drawInlineHpBar(hpRatio);
    ImGui::TextUnformatted(atkDefText);
    ImGui::EndGroup();
    ImGui::Unindent(padX);
    ImGui::EndChild();

    const float gridGap = ImGui::GetStyle().ItemSpacing.x;
    const float halfW = (ImGui::GetContentRegionAvail().x - gridGap) * 0.5f;
    const float gridRowH = 128.0f;

    ImGui::BeginChild("PartyBox", ImVec2(halfW, gridRowH), true, kHudChildFlags);
    ImGui::TextUnformatted("Party Members");
    if (cache.fellowship.empty()) {
        ImGui::TextUnformatted("No companions yet");
    } else {
        for (const std::string& member : cache.fellowship) {
            ImGui::BulletText("%s", member.c_str());
        }
    }
    ImGui::EndChild();
    ImGui::SameLine(0.0f, gridGap);
    ImGui::BeginChild("InventoryBox", ImVec2(halfW, gridRowH), true, kHudChildFlags);
    ImGui::TextUnformatted("Inventory");
    if (cache.inventory.empty()) {
        ImGui::TextUnformatted("No items yet");
    } else {
        for (const std::string& item : cache.inventory) {
            ImGui::BulletText("%s", item.c_str());
        }
    }
    ImGui::EndChild();

    ImGui::BeginChild("MapBox", ImVec2(halfW, gridRowH), true, kHudChildFlags);
    drawMapPanel(cache);
    ImGui::EndChild();
    ImGui::SameLine(0.0f, gridGap);
    ImGui::BeginChild("SpeakerBox", ImVec2(halfW, gridRowH), true, kHudChildFlags);
    drawSpeakerPanel(cache);
    ImGui::EndChild();

    if (ImGui::CollapsingHeader("Quick commands", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!canType) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Look")) {
            submitCommand("look");
        }
        ImGui::SameLine();
        if (ImGui::Button("Inv")) {
            submitCommand("inventory");
        }
        ImGui::SameLine();
        if (ImGui::Button("Stats")) {
            submitCommand("stats");
        }
        if (ImGui::Button("N")) {
            submitCommand("north");
        }
        ImGui::SameLine();
        if (ImGui::Button("S")) {
            submitCommand("south");
        }
        ImGui::SameLine();
        if (ImGui::Button("E")) {
            submitCommand("east");
        }
        ImGui::SameLine();
        if (ImGui::Button("W")) {
            submitCommand("west");
        }
        if (!canType) {
            ImGui::EndDisabled();
        }
    }

    drawDisplaySettings();
    drawSavePanel();

    ImGui::PopStyleVar(2);

    if (fontUi_) {
        ImGui::PopFont();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void GameApplication::drawUi() {
    if (appState_ == AppState::MainMenu) {
        drawMainMenu();
    } else {
        drawGameplay();
    }
}

int GameApplication::run() {
    if (!initSdlGl()) {
        return 1;
    }

    bool running = true;
    Uint64 lastStatusRefresh = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEWHEEL && (SDL_GetModState() & KMOD_CTRL)) {
                adjustFontScale(static_cast<float>(event.wheel.y) * 0.1f);
                continue;
            }

            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE
                && event.window.windowID == SDL_GetWindowID(window_)) {
                running = false;
            }
        }

        const Uint64 now = SDL_GetTicks64();
        if (appState_ == AppState::Playing
            && (waitingForInput_ || gameFinished_.load())
            && now - lastStatusRefresh > 200) {
            refreshStatusCache();
            lastStatusRefresh = now;
        }

        if (gameFinished_.load() && appState_ == AppState::Playing) {
            refreshStatusCache();
        }

        applyPendingFontScale();
        if (appState_ == AppState::Playing) {
            updateLogReveal();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        applyPixelFontTextureFilter();

        drawUi();

        ImGui::Render();
        applyPixelFontTextureFilter();
        int viewportW = 0;
        int viewportH = 0;
        SDL_GL_GetDrawableSize(window_, &viewportW, &viewportH);
        glViewport(0, 0, viewportW, viewportH);
        glClearColor(0.102f, 0.051f, 0.180f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window_);
    }

    shutdownRequested_ = true;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        if (waitingForInput_) {
            pendingInput_ = "quit";
            hasInput_ = true;
            inputReady_.notify_one();
        }
    }
    stopGameThread();
    GameIo::shutdown();
    return 0;
}
