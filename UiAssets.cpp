#include "UiAssets.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <vector>

namespace {

std::string toLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::vector<std::filesystem::path> assetSearchRoots() {
    std::vector<std::filesystem::path> roots;
    roots.emplace_back("assets");
    roots.emplace_back("../assets");
    roots.emplace_back("../../assets");
    roots.emplace_back("../../../assets");
    char* base = SDL_GetBasePath();
    if (base) {
        std::filesystem::path exeDir(base);
        SDL_free(base);
        roots.push_back(exeDir / "assets");
        roots.push_back(exeDir.parent_path() / "assets");
    }
    return roots;
}

} // namespace

void applySpireUiTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Reference palette: deep violet backdrop, soft lavender text, medium purple buttons.
    const ImVec4 bgDeep(0.102f, 0.051f, 0.180f, 1.00f);       // #1a0d2e
    const ImVec4 panel(0.14f, 0.08f, 0.20f, 0.96f);
    const ImVec4 panelHover(0.18f, 0.11f, 0.26f, 1.00f);
    const ImVec4 accent(0.482f, 0.357f, 0.655f, 1.00f);        // #7b5ba7
    const ImVec4 accentHover(0.55f, 0.42f, 0.73f, 1.00f);
    const ImVec4 textSoft(0.95f, 0.93f, 0.98f, 1.00f);
    const ImVec4 textDim(0.72f, 0.66f, 0.82f, 1.00f);

    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.WindowRounding = 14.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 10.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarSize = 10.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 8.0f;

    // Crisp pixel font rendering (no softening on glyphs or vector fills).
    style.AntiAliasedFill = false;
    style.AntiAliasedLines = false;
    style.AntiAliasedLinesUseTex = false;

    colors[ImGuiCol_Text] = textSoft;
    colors[ImGuiCol_TextDisabled] = textDim;
    colors[ImGuiCol_WindowBg] = bgDeep;
    colors[ImGuiCol_ChildBg] = panel;
    colors[ImGuiCol_PopupBg] = panel;
    colors[ImGuiCol_Border] = ImVec4(0.34f, 0.22f, 0.46f, 0.55f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.13f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = panelHover;
    colors[ImGuiCol_FrameBgActive] = accent;
    colors[ImGuiCol_TitleBg] = bgDeep;
    colors[ImGuiCol_TitleBgActive] = panel;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.08f, 0.17f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab] = accent;
    colors[ImGuiCol_ScrollbarGrabHovered] = accentHover;
    colors[ImGuiCol_ScrollbarGrabActive] = accentHover;
    colors[ImGuiCol_Button] = accent;
    colors[ImGuiCol_ButtonHovered] = accentHover;
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.26f, 0.54f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.28f, 0.18f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = accentHover;
    colors[ImGuiCol_HeaderActive] = accent;
    colors[ImGuiCol_SliderGrab] = accentHover;
    colors[ImGuiCol_SliderGrabActive] = accent;
    colors[ImGuiCol_Separator] = ImVec4(0.34f, 0.22f, 0.46f, 0.65f);
}

unsigned UiAssets::createSolidTexture(unsigned char r, unsigned char g, unsigned char b) {
    unsigned char pixel[4] = {r, g, b, 255};
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return tex;
}

unsigned UiAssets::loadImageFile(const std::string& relativePath) {
    for (const std::filesystem::path& root : assetSearchRoots()) {
        const std::filesystem::path full = root / relativePath;
        if (!std::filesystem::exists(full)) {
            continue;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* pixels = stbi_load(full.string().c_str(), &width, &height, &channels, 4);
        if (!pixels || width <= 0 || height <= 0) {
            stbi_image_free(pixels);
            continue;
        }

        unsigned int tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
        return tex;
    }
    return 0;
}

bool UiAssets::init() {
    const std::vector<std::string> preload = {
        "ui/bg_story_panel.png",
        "ui/bg_main_menu.png",
        "icons/icon_gold.png",
        "icons/icon_atk.png",
        "icons/icon_def.png",
        "icons/icon_placeholder.png",
        "portraits/portrait_hero.png",
        "portraits/portrait_none.png",
        "portraits/portrait_elder_cedric.png",
        "portraits/portrait_pip.png",
        "portraits/portrait_lyra.png",
        "portraits/portrait_bram.png",
        "portraits/portrait_valerie.png",
        "portraits/portrait_garrison.png",
        "portraits/portrait_eleanor.png",
        "portraits/portrait_garrick.png",
        "portraits/portrait_malakor.png",
        "portraits/portrait_azael.png",
    };

    for (const std::string& path : preload) {
        const size_t slash = path.find_last_of('/');
        const std::string id = slash == std::string::npos ? path : path.substr(slash + 1);
        const std::string stem = id.substr(0, id.find_last_of('.'));
        textures_[stem] = loadImageFile(path);
    }

    if (textures_["bg_story_panel"] == 0) {
        textures_["bg_story_panel"] = createSolidTexture(26, 13, 46);
    }
    if (textures_["portrait_none"] == 0) {
        textures_["portrait_none"] = createSolidTexture(72, 56, 96);
    }
    if (textures_["portrait_hero"] == 0) {
        textures_["portrait_hero"] = textures_["portrait_none"];
    }
    if (textures_["icon_placeholder"] == 0) {
        textures_["icon_placeholder"] = createSolidTexture(90, 70, 120);
    }

    return true;
}

void UiAssets::shutdown() {
    std::vector<GLuint> unique;
    for (const auto& entry : textures_) {
        if (entry.second == 0) {
            continue;
        }
        const GLuint id = entry.second;
        if (std::find(unique.begin(), unique.end(), id) == unique.end()) {
            unique.push_back(id);
        }
    }
    for (GLuint id : unique) {
        glDeleteTextures(1, &id);
    }
    textures_.clear();
}

ImTextureID UiAssets::texture(const std::string& assetId) const {
    const auto it = textures_.find(assetId);
    if (it == textures_.end() || it->second == 0) {
        return ImTextureID(0);
    }
    return ImTextureID(static_cast<intptr_t>(it->second));
}

std::string UiAssets::portraitAssetId(const std::string& npcName) {
    const std::string key = toLower(npcName);
    if (key.empty()) {
        return "portrait_none";
    }
    if (key.find("cedric") != std::string::npos || key.find("elder") != std::string::npos) {
        return "portrait_elder_cedric";
    }
    if (key.find("pip") != std::string::npos) {
        return "portrait_pip";
    }
    if (key.find("lyra") != std::string::npos) {
        return "portrait_lyra";
    }
    if (key.find("bram") != std::string::npos) {
        return "portrait_bram";
    }
    if (key.find("valerie") != std::string::npos) {
        return "portrait_valerie";
    }
    if (key.find("garrison") != std::string::npos || key.find("knight") != std::string::npos
        || key.find("armored") != std::string::npos) {
        return "portrait_garrison";
    }
    if (key.find("eleanor") != std::string::npos || key.find("veiled woman") != std::string::npos) {
        return "portrait_eleanor";
    }
    if (key.find("garrick") != std::string::npos || key.find("merchant") != std::string::npos) {
        return "portrait_garrick";
    }
    if (key.find("malakor") != std::string::npos || key.find("makalor") != std::string::npos) {
        return "portrait_malakor";
    }
    if (key.find("azael") != std::string::npos) {
        return "portrait_azael";
    }
    return "portrait_none";
}

ImTextureID UiAssets::portraitForNpc(const std::string& npcName) const {
    const std::string id = portraitAssetId(npcName);
    ImTextureID tex = texture(id);
    if (tex) {
        return tex;
    }
    return texture("portrait_none");
}
