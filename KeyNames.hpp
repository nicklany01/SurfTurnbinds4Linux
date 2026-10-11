#pragma once

#include <string>
#include <unordered_map>
#include <linux/input.h>

class KeyNames {
public:
    static std::string getName(int keycode) {
        if (keycode == 0) return "None";
        static const std::unordered_map<int, std::string> names = {
            // Mouse buttons
            {BTN_LEFT, "Mouse 1 (Left Click)"},
            {BTN_RIGHT, "Mouse 2 (Right Click)"},
            {BTN_MIDDLE, "Mouse 3 (Middle Click)"},
            {BTN_SIDE, "Mouse 4 (Side / Back)"},
            {BTN_EXTRA, "Mouse 5 (Side / Forward)"},

            // Letters
            {KEY_A, "A"}, {KEY_B, "B"}, {KEY_C, "C"}, {KEY_D, "D"},
            {KEY_E, "E"}, {KEY_F, "F"}, {KEY_G, "G"}, {KEY_H, "H"},
            {KEY_I, "I"}, {KEY_J, "J"}, {KEY_K, "K"}, {KEY_L, "L"},
            {KEY_M, "M"}, {KEY_N, "N"}, {KEY_O, "O"}, {KEY_P, "P"},
            {KEY_Q, "Q"}, {KEY_R, "R"}, {KEY_S, "S"}, {KEY_T, "T"},
            {KEY_U, "U"}, {KEY_V, "V"}, {KEY_W, "W"}, {KEY_X, "X"},
            {KEY_Y, "Y"}, {KEY_Z, "Z"},

            // Numbers
            {KEY_0, "0"}, {KEY_1, "1"}, {KEY_2, "2"}, {KEY_3, "3"},
            {KEY_4, "4"}, {KEY_5, "5"}, {KEY_6, "6"}, {KEY_7, "7"},
            {KEY_8, "8"}, {KEY_9, "9"},

            // Modifiers & Controls
            {KEY_SPACE, "Space"},
            {KEY_LEFTSHIFT, "Left Shift"},
            {KEY_RIGHTSHIFT, "Right Shift"},
            {KEY_LEFTCTRL, "Left Ctrl"},
            {KEY_RIGHTCTRL, "Right Ctrl"},
            {KEY_LEFTALT, "Left Alt"},
            {KEY_RIGHTALT, "Right Alt"},
            {KEY_TAB, "Tab"},
            {KEY_CAPSLOCK, "Caps Lock"},
            {KEY_ENTER, "Enter"},
            {KEY_BACKSPACE, "Backspace"},
            {KEY_ESC, "Escape"},

            // Arrows
            {KEY_LEFT, "Left Arrow"},
            {KEY_RIGHT, "Right Arrow"},
            {KEY_UP, "Up Arrow"},
            {KEY_DOWN, "Down Arrow"},

            // Navigation
            {KEY_PAGEUP, "Page Up"},
            {KEY_PAGEDOWN, "Page Down"},
            {KEY_HOME, "Home"},
            {KEY_END, "End"},
            {KEY_INSERT, "Insert"},
            {KEY_DELETE, "Delete"},

            // Function keys
            {KEY_F1, "F1"}, {KEY_F2, "F2"}, {KEY_F3, "F3"}, {KEY_F4, "F4"},
            {KEY_F5, "F5"}, {KEY_F6, "F6"}, {KEY_F7, "F7"}, {KEY_F8, "F8"},
            {KEY_F9, "F9"}, {KEY_F10, "F10"}, {KEY_F11, "F11"}, {KEY_F12, "F12"},

            // Symbols
            {KEY_GRAVE, "` (Tilde)"},
            {KEY_MINUS, "-"},
            {KEY_EQUAL, "="},
            {KEY_LEFTBRACE, "["},
            {KEY_RIGHTBRACE, "]"},
            {KEY_SEMICOLON, ";"},
            {KEY_APOSTROPHE, "'"},
            {KEY_COMMA, ","},
            {KEY_DOT, "."},
            {KEY_SLASH, "/"},
            {KEY_BACKSLASH, "\\"}
        };

        auto it = names.find(keycode);
        if (it != names.end()) {
            return it->second;
        }
        return "Key #" + std::to_string(keycode);
    }
};
