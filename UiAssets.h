#ifndef UIASSETS_H
#define UIASSETS_H

#include "imgui.h"

#include <string>
#include <unordered_map>

class UiAssets {
public:
    bool init();
    void shutdown();

    ImTextureID texture(const std::string& assetId) const;
    ImTextureID portraitForNpc(const std::string& npcName) const;
    static std::string portraitAssetId(const std::string& npcName);

private:
    unsigned loadImageFile(const std::string& relativePath);
    unsigned createSolidTexture(unsigned char r, unsigned char g, unsigned char b);

    std::unordered_map<std::string, unsigned> textures_;
};

void applySpireUiTheme();

#endif
