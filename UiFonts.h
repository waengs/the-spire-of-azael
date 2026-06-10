#pragma once

struct ImFont;

// Pixel UI fonts (Press Start 2P from assets, or ProggyClean fallback).
bool setupSpirePixelFonts(
    ImFont*& outUiFont,
    ImFont*& outMenuTitleFont,
    ImFont*& outGameplayFont,
    ImFont*& outMenuBodyFont,
    float userScale = 1.0f);

// ImGui bakes fonts with linear filtering by default; call after font atlas upload.
void applyPixelFontTextureFilter();
