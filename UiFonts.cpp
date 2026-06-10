#include "UiFonts.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>

namespace {

constexpr const char* kPixelFontRelPath = "fonts/PressStart2P-Regular.ttf";

std::vector<std::filesystem::path> fontSearchRoots() {
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

std::filesystem::path resolvePixelFontPath() {
    for (const std::filesystem::path& root : fontSearchRoots()) {
        const std::filesystem::path candidate = root / kPixelFontRelPath;
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

int pixelDpiScale() {
    const ImGuiIO& io = ImGui::GetIO();
    const float scale = std::max(io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    return std::max(1, static_cast<int>(std::lround(scale)));
}

int pixelSize(float logicalPx, float userScale) {
    const float scaled = logicalPx * std::max(0.5f, userScale);
    const int logical = std::max(8, static_cast<int>(std::lround(scaled)));
    return logical * pixelDpiScale();
}

ImFont* addPixelFont(
    ImFontAtlas& atlas,
    const char* path,
    float logicalPx,
    float userScale,
    ImFontConfig& sharedCfg) {
    const int sizePx = pixelSize(logicalPx, userScale);
    if (path && path[0]) {
        return atlas.AddFontFromFileTTF(path, static_cast<float>(sizePx), &sharedCfg);
    }

    ImFontConfig proggyCfg = sharedCfg;
    proggyCfg.SizePixels = static_cast<float>(sizePx);
    return atlas.AddFontDefault(&proggyCfg);
}

} // namespace

bool setupSpirePixelFonts(
    ImFont*& outUiFont,
    ImFont*& outMenuTitleFont,
    ImFont*& outGameplayFont,
    ImFont*& outMenuBodyFont,
    float userScale) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.FontGlobalScale = 1.0f;
    io.FontAllowUserScaling = false;

    ImFontConfig cfg;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = true;
    cfg.RasterizerMultiply = 1.0f;

    const std::filesystem::path fontPath = resolvePixelFontPath();
    const std::string fontPathStr = fontPath.empty() ? std::string() : fontPath.string();

    const float scale = std::clamp(userScale, 0.75f, 2.5f);

    // Integer pixel sizes (Press Start 2P is sharpest at multiples of 8).
    outGameplayFont = addPixelFont(*io.Fonts, fontPathStr.c_str(), 8.0f, scale, cfg);
    outUiFont = addPixelFont(*io.Fonts, fontPathStr.c_str(), 8.0f, scale, cfg);
    outMenuBodyFont = addPixelFont(*io.Fonts, fontPathStr.c_str(), 16.0f, scale, cfg);
    outMenuTitleFont = addPixelFont(*io.Fonts, fontPathStr.c_str(), 32.0f, scale, cfg);

    if (!outUiFont || !outMenuTitleFont || !outGameplayFont || !outMenuBodyFont) {
        outUiFont = io.Fonts->AddFontDefault();
        outGameplayFont = outUiFont;
        outMenuBodyFont = outUiFont;
        outMenuTitleFont = outUiFont;
    }

    io.FontDefault = outUiFont;
    io.Fonts->Build();

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplOpenGL3_CreateDeviceObjects();
    applyPixelFontTextureFilter();
    return outUiFont != nullptr;
}

void applyPixelFontTextureFilter() {
    const ImTextureID texId = ImGui::GetIO().Fonts->TexID;
    if (texId == ImTextureID(0)) {
        return;
    }

    const GLuint texture = static_cast<GLuint>(texId);
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
}
