#include "Widgets.hpp"

namespace odyssee::debugclient {

namespace {

constexpr int kTextSize = 20;
constexpr Color kFieldBack{30, 34, 40, 255};
constexpr Color kFocus{230, 160, 60, 255};

/// Retire le dernier caractère UTF-8 (un ou plusieurs octets).
void eraseLastCharacter(std::string& text) {
    while (!text.empty()) {
        const auto byte = static_cast<unsigned char>(text.back());
        text.pop_back();
        if ((byte & 0xC0U) != 0x80U) {
            return; // premier octet du caractère : fini
        }
    }
}

bool accepts(const TextField& field, int codepoint) {
    if (codepoint < 32 || codepoint == 127) {
        return false;
    }
    return !field.digitsOnly || (codepoint >= '0' && codepoint <= '9');
}

} // namespace

void typeInto(const TextField& field) {
    for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
        int bytes = 0;
        const char* utf8 = CodepointToUTF8(codepoint, &bytes);
        const auto size = static_cast<std::size_t>(bytes);
        if (accepts(field, codepoint) && field.text->size() + size <= field.maxBytes) {
            field.text->append(utf8, size);
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        eraseLastCharacter(*field.text);
    }
}

void drawField(const TextField& field, Rectangle box, bool focused) {
    DrawText(field.label, static_cast<int>(box.x), static_cast<int>(box.y) - 24, 18, LIGHTGRAY);
    DrawRectangleRec(box, kFieldBack);
    DrawRectangleLinesEx(box, 2.0f, focused ? kFocus : GRAY);

    const bool blink = static_cast<int>(GetTime() * 2.0) % 2 == 0;
    const std::string shown = *field.text + (focused && blink ? "_" : "");
    DrawText(shown.c_str(), static_cast<int>(box.x) + 10,
             static_cast<int>(box.y + (box.height - kTextSize) / 2), kTextSize, RAYWHITE);
}

bool button(Rectangle box, const char* label) {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), box);
    DrawRectangleRec(box, hovered ? Color{70, 90, 110, 255} : Color{50, 60, 72, 255});
    DrawRectangleLinesEx(box, 2.0f, hovered ? kFocus : GRAY);
    const int width = MeasureText(label, kTextSize);
    DrawText(label, static_cast<int>(box.x + (box.width - static_cast<float>(width)) / 2),
             static_cast<int>(box.y + (box.height - kTextSize) / 2), kTextSize, RAYWHITE);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void drawCentered(const char* text, int y, int size, Color color) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, size)) / 2, y, size, color);
}

} // namespace odyssee::debugclient
