#include "AnsiText.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

namespace {

struct AnsiState {
    ImVec4 color;
    bool bold = false;
};

struct ColoredPiece {
    std::string text;
    ImVec4 color;
};

ImVec4 baseForeground(int code) {
    // Standard ANSI foreground colors tuned for the dark story panel.
    switch (code) {
    case 0:
        return ImVec4(0.58f, 0.54f, 0.66f, 1.0f);
    case 1:
        return ImVec4(0.95f, 0.38f, 0.38f, 1.0f);
    case 2:
        return ImVec4(0.42f, 0.92f, 0.52f, 1.0f);
    case 3:
        return ImVec4(0.98f, 0.84f, 0.38f, 1.0f);
    case 4:
        return ImVec4(0.48f, 0.72f, 1.0f, 1.0f);
    case 5:
        return ImVec4(0.86f, 0.52f, 0.98f, 1.0f);
    case 6:
        return ImVec4(0.42f, 0.92f, 0.96f, 1.0f);
    case 7:
        return ImVec4(0.96f, 0.94f, 0.99f, 1.0f);
    default:
        return ImVec4(0.96f, 0.94f, 0.99f, 1.0f);
    }
}

ImVec4 brighten(ImVec4 color) {
    return ImVec4(
        std::min(color.x * 1.15f + 0.08f, 1.0f),
        std::min(color.y * 1.15f + 0.08f, 1.0f),
        std::min(color.z * 1.15f + 0.08f, 1.0f),
        color.w);
}

void applySgrCode(int code, AnsiState& state, const ImVec4& defaultColor) {
    if (code == 0) {
        state.color = defaultColor;
        state.bold = false;
        return;
    }
    if (code == 1) {
        state.bold = true;
        state.color = brighten(state.color);
        return;
    }
    if (code >= 30 && code <= 37) {
        state.color = baseForeground(code - 30);
        if (state.bold) {
            state.color = brighten(state.color);
        }
        return;
    }
    if (code >= 90 && code <= 97) {
        state.color = brighten(baseForeground(code - 90));
        state.bold = true;
    }
}

void applySgrSequence(const std::string& codes, AnsiState& state, const ImVec4& defaultColor) {
    int value = 0;
    bool hasValue = false;
    for (char ch : codes) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            value = value * 10 + (ch - '0');
            hasValue = true;
            continue;
        }
        if (ch == ';') {
            if (hasValue) {
                applySgrCode(value, state, defaultColor);
            }
            value = 0;
            hasValue = false;
            continue;
        }
    }
    if (hasValue) {
        applySgrCode(value, state, defaultColor);
    }
}

std::vector<ColoredPiece> tokenizeAnsi(const std::string& text, const ImVec4& defaultColor) {
    std::vector<ColoredPiece> pieces;
    AnsiState state{defaultColor, false};

    size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\x1B' && index + 1 < text.size() && text[index + 1] == '[') {
            index += 2;
            std::string codes;
            while (index < text.size() && text[index] != 'm') {
                codes.push_back(text[index++]);
            }
            if (index < text.size()) {
                ++index;
            }
            applySgrSequence(codes, state, defaultColor);
            continue;
        }

        if (text[index] == '\n') {
            pieces.push_back({"\n", state.color});
            ++index;
            continue;
        }

        size_t end = index;
        while (end < text.size() && text[end] != '\x1B' && text[end] != '\n') {
            ++end;
        }
        if (end > index) {
            pieces.push_back({text.substr(index, end - index), state.color});
        }
        index = end;
    }

    return pieces;
}

std::vector<std::vector<ColoredPiece>> wrapPieces(const std::vector<ColoredPiece>& pieces, float wrapWidth) {
    std::vector<std::vector<ColoredPiece>> lines;
    lines.emplace_back();

    auto startNewLine = [&]() {
        if (!lines.back().empty()) {
            lines.emplace_back();
        }
    };

    auto lineWidth = [&]() {
        float width = 0.0f;
        for (const ColoredPiece& piece : lines.back()) {
            width += ImGui::CalcTextSize(piece.text.c_str()).x;
        }
        return width;
    };

    for (const ColoredPiece& piece : pieces) {
        if (piece.text == "\n") {
            startNewLine();
            continue;
        }

        std::istringstream words(piece.text);
        std::string word;
        while (words >> word) {
            const float wordWidth = ImGui::CalcTextSize(word.c_str()).x;
            if (wordWidth > wrapWidth) {
                for (char ch : word) {
                    const std::string letter(1, ch);
                    const float letterWidth = ImGui::CalcTextSize(letter.c_str()).x;
                    if (!lines.back().empty() && lineWidth() + letterWidth > wrapWidth) {
                        startNewLine();
                    }
                    lines.back().push_back({letter, piece.color});
                }
                continue;
            }

            const float spaceWidth = lines.back().empty() ? 0.0f : ImGui::CalcTextSize(" ").x;
            if (!lines.back().empty() && lineWidth() + spaceWidth + wordWidth > wrapWidth) {
                startNewLine();
            }

            if (!lines.back().empty()) {
                lines.back().push_back({" ", piece.color});
            }
            lines.back().push_back({word, piece.color});
        }
    }

    if (lines.size() == 1 && lines[0].empty()) {
        lines.clear();
    }
    return lines;
}

void drawLine(const std::vector<ColoredPiece>& line) {
    bool first = true;
    for (const ColoredPiece& piece : line) {
        if (!first) {
            ImGui::SameLine(0.0f, 0.0f);
        }
        ImGui::PushStyleColor(ImGuiCol_Text, piece.color);
        ImGui::TextUnformatted(piece.text.c_str());
        ImGui::PopStyleColor();
        first = false;
    }
}

} // namespace

void drawAnsiTextWrapped(const std::string& text, float wrapWidth, const ImVec4& defaultColor) {
    if (text.empty()) {
        return;
    }

    const std::vector<ColoredPiece> pieces = tokenizeAnsi(text, defaultColor);
    const std::vector<std::vector<ColoredPiece>> lines = wrapPieces(pieces, wrapWidth);
    for (const std::vector<ColoredPiece>& line : lines) {
        if (line.empty()) {
            ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() * 0.35f));
            continue;
        }
        drawLine(line);
    }
}
