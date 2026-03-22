#pragma once
#include "Libs/ImGui/imgui.h"

#include <string>

namespace CustomGUI {
    bool Button(const char* label, float scale = 0.50f, bool isDisabled = false, const ImVec2& size = ImVec2(0, 0)) {
        ImFont* font = ImGui::GetFont();
        int fontSize = font->FontSize * scale;
        float padding = 15 * (scale * 2);
        ImVec2 ButtonSize = size;

        ImVec2 TextSize = font->CalcTextSizeA(fontSize, 5000, 5000, label);


        if (size.x == 0 && size.y == 0) {
            ButtonSize = ImVec2(TextSize.x + (padding * 2), TextSize.y + padding);
        }

        ImVec2 CursorPos = ImGui::GetCursorScreenPos();

        bool realButtonValue = ImGui::InvisibleButton(label, ButtonSize);

        ImU32 buttonColor = ImGui::GetColorU32(ImVec4(0.8, 0.8, 0.8, 0.9));
        ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.1, 0.1, 0.1, 0.9));

        ImU32 textColor = ImGui::GetColorU32(ImVec4(0.2, 0.2, 0.2, 0.9));
        if (isDisabled) {
            buttonColor = ImGui::GetColorU32(ImVec4(0.2, 0.2, 0.2, 0.9));
            textColor = ImGui::GetColorU32(ImVec4(0.7, 0.7, 0.7, 0.9));

        } else {
            if (realButtonValue || ImGui::IsItemHovered()) {
                buttonColor = ImGui::GetColorU32(ImVec4(0, 0.8, 0, 0.9));
                borderColor = ImGui::GetColorU32(ImVec4(0.9, 0.9, 0.9, 0.9));

                textColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 0.9));
            }
        }

        ImDrawList* drawList = ImGui::GetForegroundDrawList();

        drawList->AddRectFilled(ImVec2(CursorPos.x, CursorPos.y), ImVec2(CursorPos.x + ButtonSize.x, CursorPos.y + ButtonSize.y), buttonColor);
        drawList->AddRect(ImVec2(CursorPos.x, CursorPos.y), ImVec2(CursorPos.x + ButtonSize.x, CursorPos.y + ButtonSize.y), borderColor, 0, 0, 1);

        drawList->AddText(font, fontSize, ImVec2(CursorPos.x + (ButtonSize.x - TextSize.x) * 0.5f, CursorPos.y + (ButtonSize.y - TextSize.y) * 0.5f), textColor, label);

        return realButtonValue && !isDisabled;
    }

    bool ClickableText(const char* label, float scale = 0.50f, bool forceClicked = false, const ImVec2& size = ImVec2(0, 0)) {
        ImFont* font = ImGui::GetFont();
        int fontSize = font->FontSize * scale;
        float padding = 2 * (scale * 2);
        ImVec2 ButtonSize = size;

        if (size.x == 0 && size.y == 0) {
            ImVec2 TextSize = font->CalcTextSizeA(fontSize, 5000, 5000, label);

            ButtonSize = ImVec2(TextSize.x + (padding * 3), TextSize.y + padding);
        }

        ImVec2 CursorPos = ImGui::GetCursorScreenPos();

        bool realButtonValue = ImGui::InvisibleButton(label, ButtonSize);

        ImU32 textColor = ImGui::GetColorU32(ImVec4(0.9, 0.9, 0.9, 0.9));

        if (realButtonValue || ImGui::IsItemHovered() || forceClicked) {

            if (ImGui::IsItemHovered()) {
                textColor = ImGui::GetColorU32(ImVec4(0, 0.9, 0, 0.9));
            } else {
                textColor = ImGui::GetColorU32(ImVec4(0, 0.85, 0, 0.9));
            }
        }

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        drawList->AddText(font, fontSize, ImVec2(CursorPos.x + (padding * 1.5f), CursorPos.y + (padding / 2)), textColor, label);

        return realButtonValue || forceClicked;
    }
}

namespace RichText {

    struct Style {
        ImVec4 color = ImVec4(1, 1, 1, 1);
        ImFont* font = nullptr;
    };

    static bool StartsWith(const std::string& str, size_t pos, const std::string& token) {
        return str.compare(pos, token.length(), token) == 0;
    }

    static ImVec4 ParseColor(const std::string& str) {
        float r, g, b;
        sscanf_s(str.c_str(), "%f,%f,%f", &r, &g, &b);
        return ImVec4(r, g, b, 1.0f);
    }

    void Render(const std::string& text, float wrapWidth) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        float x = cursor.x;
        float y = cursor.y;

        float startX = x;

        Style style;
        style.color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        style.font = ImGui::GetFont(); //todo: maybe add more fonts

        std::vector<Style> stack;
        stack.push_back(style);

        std::string word;

        auto flushWord = [&](const std::string& w) {
            if (w.empty()) return;

            ImFont* font = style.font;
            float fontSize = font->FontSize;

            ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, w.c_str());

            if (x + size.x > startX + wrapWidth) {
                x = startX;
                y += fontSize + 2;
            }

            draw->AddText(font, fontSize, ImVec2(x, y), ImGui::GetColorU32(style.color), w.c_str());

            x += size.x;
            };

        for (size_t i = 0; i < text.size(); i++) {

            if (text[i] == '\n') {
                flushWord(word);
                word.clear();

                x = startX;
                y += style.font->FontSize + 2;
                continue;
            }

            if (text[i] == '[') {
                flushWord(word);
                word.clear();

                size_t end = text.find(']', i);
                if (end == std::string::npos) continue;

                std::string tag = text.substr(i + 1, end - i - 1);

                if (tag == "/color" || tag == "/b") {
                    if (stack.size() > 1) {
                        stack.pop_back();
                        style = stack.back();
                    }
                } else if (tag.starts_with("color=")) {
                    Style newStyle = style;
                    newStyle.color = ParseColor(tag.substr(6));
                    stack.push_back(newStyle);
                    style = newStyle;
                } /*else if (tag == "b") {
                    Style newStyle = style;
                    newStyle.font = bold;
                    stack.push_back(newStyle);
                    style = newStyle;
                }*/

                i = end;
                continue;
            }

            if (text[i] == ' ') {
                word += text[i];
                flushWord(word);
                word.clear();
                continue;
            }

            word += text[i];
        }

        flushWord(word);

        ImGui::Dummy(ImVec2(wrapWidth, y - cursor.y + style.font->FontSize));
    }
}