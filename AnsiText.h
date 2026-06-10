#pragma once

#include "imgui.h"

#include <string>

// Renders console-style ANSI SGR text (e.g. \033[1;32m) with word wrap.
void drawAnsiTextWrapped(const std::string& text, float wrapWidth, const ImVec4& defaultColor);
