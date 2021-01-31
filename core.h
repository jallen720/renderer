#pragma once

#include "ctk/ctk.h"

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
struct Core {
    struct {
        CTK_Stack perma;
        CTK_Stack temp;
    } mem;
};

enum {
    INPUT_KEY_0,
    INPUT_KEY_1,
    INPUT_KEY_2,
    INPUT_KEY_3,
    INPUT_KEY_4,
    INPUT_KEY_5,
    INPUT_KEY_6,
    INPUT_KEY_7,
    INPUT_KEY_8,
    INPUT_KEY_9,
    INPUT_KEY_A,
    INPUT_KEY_B,
    INPUT_KEY_C,
    INPUT_KEY_D,
    INPUT_KEY_E,
    INPUT_KEY_F,
    INPUT_KEY_G,
    INPUT_KEY_H,
    INPUT_KEY_I,
    INPUT_KEY_J,
    INPUT_KEY_K,
    INPUT_KEY_L,
    INPUT_KEY_M,
    INPUT_KEY_N,
    INPUT_KEY_O,
    INPUT_KEY_P,
    INPUT_KEY_Q,
    INPUT_KEY_R,
    INPUT_KEY_S,
    INPUT_KEY_T,
    INPUT_KEY_U,
    INPUT_KEY_V,
    INPUT_KEY_W,
    INPUT_KEY_X,
    INPUT_KEY_Y,
    INPUT_KEY_Z,
    INPUT_KEY_MOUSE_0,
    INPUT_KEY_MOUSE_1,
    INPUT_KEY_CANCEL,
    INPUT_KEY_MOUSE_2,
    INPUT_KEY_MOUSE_3,
    INPUT_KEY_MOUSE_4,
    INPUT_KEY_BACK,
    INPUT_KEY_TAB,
    INPUT_KEY_CLEAR,
    INPUT_KEY_RETURN,
    INPUT_KEY_SHIFT,
    INPUT_KEY_CONTROL,
    INPUT_KEY_MENU,
    INPUT_KEY_PAUSE,
    INPUT_KEY_CAPITAL,
    INPUT_KEY_KANA,
    INPUT_KEY_HANGEUL,
    INPUT_KEY_HANGUL,
    INPUT_KEY_JUNJA,
    INPUT_KEY_FINAL,
    INPUT_KEY_HANJA,
    INPUT_KEY_KANJI,
    INPUT_KEY_ESCAPE,
    INPUT_KEY_CONVERT,
    INPUT_KEY_NONCONVERT,
    INPUT_KEY_ACCEPT,
    INPUT_KEY_MODECHANGE,
    INPUT_KEY_SPACE,
    INPUT_KEY_PRIOR,
    INPUT_KEY_NEXT,
    INPUT_KEY_END,
    INPUT_KEY_HOME,
    INPUT_KEY_LEFT,
    INPUT_KEY_UP,
    INPUT_KEY_RIGHT,
    INPUT_KEY_DOWN,
    INPUT_KEY_SELECT,
    INPUT_KEY_PRINT,
    INPUT_KEY_EXECUTE,
    INPUT_KEY_SNAPSHOT,
    INPUT_KEY_INSERT,
    INPUT_KEY_DELETE,
    INPUT_KEY_HELP,
    INPUT_KEY_LWIN,
    INPUT_KEY_RWIN,
    INPUT_KEY_APPS,
    INPUT_KEY_SLEEP,
    INPUT_KEY_NUMPAD_0,
    INPUT_KEY_NUMPAD_1,
    INPUT_KEY_NUMPAD_2,
    INPUT_KEY_NUMPAD_3,
    INPUT_KEY_NUMPAD_4,
    INPUT_KEY_NUMPAD_5,
    INPUT_KEY_NUMPAD_6,
    INPUT_KEY_NUMPAD_7,
    INPUT_KEY_NUMPAD_8,
    INPUT_KEY_NUMPAD_9,
    INPUT_KEY_MULTIPLY,
    INPUT_KEY_ADD,
    INPUT_KEY_SEPARATOR,
    INPUT_KEY_SUBTRACT,
    INPUT_KEY_DECIMAL,
    INPUT_KEY_DIVIDE,
    INPUT_KEY_F1,
    INPUT_KEY_F2,
    INPUT_KEY_F3,
    INPUT_KEY_F4,
    INPUT_KEY_F5,
    INPUT_KEY_F6,
    INPUT_KEY_F7,
    INPUT_KEY_F8,
    INPUT_KEY_F9,
    INPUT_KEY_F10,
    INPUT_KEY_F11,
    INPUT_KEY_F12,
    INPUT_KEY_F13,
    INPUT_KEY_F14,
    INPUT_KEY_F15,
    INPUT_KEY_F16,
    INPUT_KEY_F17,
    INPUT_KEY_F18,
    INPUT_KEY_F19,
    INPUT_KEY_F20,
    INPUT_KEY_F21,
    INPUT_KEY_F22,
    INPUT_KEY_F23,
    INPUT_KEY_F24,
    INPUT_KEY_NAVIGATION_VIEW,
    INPUT_KEY_NAVIGATION_MENU,
    INPUT_KEY_NAVIGATION_UP,
    INPUT_KEY_NAVIGATION_DOWN,
    INPUT_KEY_NAVIGATION_LEFT,
    INPUT_KEY_NAVIGATION_RIGHT,
    INPUT_KEY_NAVIGATION_ACCEPT,
    INPUT_KEY_NAVIGATION_CANCEL,
    INPUT_KEY_NUMLOCK,
    INPUT_KEY_SCROLL,
    INPUT_KEY_NUMPAD_EQUAL,
    INPUT_KEY_FJ_JISHO,
    INPUT_KEY_FJ_MASSHOU,
    INPUT_KEY_FJ_TOUROKU,
    INPUT_KEY_FJ_LOYA,
    INPUT_KEY_FJ_ROYA,
    INPUT_KEY_LSHIFT,
    INPUT_KEY_RSHIFT,
    INPUT_KEY_LCONTROL,
    INPUT_KEY_RCONTROL,
    INPUT_KEY_LMENU,
    INPUT_KEY_RMENU,
    INPUT_KEY_BROWSER_BACK,
    INPUT_KEY_BROWSER_FORWARD,
    INPUT_KEY_BROWSER_REFRESH,
    INPUT_KEY_BROWSER_STOP,
    INPUT_KEY_BROWSER_SEARCH,
    INPUT_KEY_BROWSER_FAVORITES,
    INPUT_KEY_BROWSER_HOME,
    INPUT_KEY_VOLUME_MUTE,
    INPUT_KEY_VOLUME_DOWN,
    INPUT_KEY_VOLUME_UP,
    INPUT_KEY_MEDIA_NEXT_TRACK,
    INPUT_KEY_MEDIA_PREV_TRACK,
    INPUT_KEY_MEDIA_STOP,
    INPUT_KEY_MEDIA_PLAY_PAUSE,
    INPUT_KEY_LAUNCH_MAIL,
    INPUT_KEY_LAUNCH_MEDIA_SELECT,
    INPUT_KEY_LAUNCH_APP1,
    INPUT_KEY_LAUNCH_APP2,
    INPUT_KEY_SEMICOLON_COLON,
    INPUT_KEY_PLUS,
    INPUT_KEY_COMMA,
    INPUT_KEY_MINUS,
    INPUT_KEY_PERIOD,
    INPUT_KEY_SLASH_QUESTION,
    INPUT_KEY_BACKTICK_TILDE,
    INPUT_KEY_GAMEPAD_A,
    INPUT_KEY_GAMEPAD_B,
    INPUT_KEY_GAMEPAD_X,
    INPUT_KEY_GAMEPAD_Y,
    INPUT_KEY_GAMEPAD_RIGHT_SHOULDER,
    INPUT_KEY_GAMEPAD_LEFT_SHOULDER,
    INPUT_KEY_GAMEPAD_LEFT_TRIGGER,
    INPUT_KEY_GAMEPAD_RIGHT_TRIGGER,
    INPUT_KEY_GAMEPAD_DPAD_UP,
    INPUT_KEY_GAMEPAD_DPAD_DOWN,
    INPUT_KEY_GAMEPAD_DPAD_LEFT,
    INPUT_KEY_GAMEPAD_DPAD_RIGHT,
    INPUT_KEY_GAMEPAD_MENU,
    INPUT_KEY_GAMEPAD_VIEW,
    INPUT_KEY_GAMEPAD_LEFT_THUMBSTICK_BUTTON,
    INPUT_KEY_GAMEPAD_RIGHT_THUMBSTICK_BUTTON,
    INPUT_KEY_GAMEPAD_LEFT_THUMBSTICK_UP,
    INPUT_KEY_GAMEPAD_LEFT_THUMBSTICK_DOWN,
    INPUT_KEY_GAMEPAD_LEFT_THUMBSTICK_RIGHT,
    INPUT_KEY_GAMEPAD_LEFT_THUMBSTICK_LEFT,
    INPUT_KEY_GAMEPAD_RIGHT_THUMBSTICK_UP,
    INPUT_KEY_GAMEPAD_RIGHT_THUMBSTICK_DOWN,
    INPUT_KEY_GAMEPAD_RIGHT_THUMBSTICK_RIGHT,
    INPUT_KEY_GAMEPAD_RIGHT_THUMBSTICK_LEFT,
    INPUT_KEY_OPEN_BRACKET,
    INPUT_KEY_BACKSLASH_PIPE,
    INPUT_KEY_CLOSE_BRACKET,
    INPUT_KEY_APOSTROPHE_QUOTE,
    INPUT_KEY_OEM_8,
    INPUT_KEY_AX,
    INPUT_KEY_102,
    INPUT_KEY_ICO_HELP,
    INPUT_KEY_ICO_00,
    INPUT_KEY_COUNT,
};

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static Core *create_core() {
    // Memory
    CTK_Stack perma_stack = ctk_create_stack(4 * CTK_KILOBYTE);
    auto core = ctk_alloc<Core>(&perma_stack, 1);
    core->mem.perma = perma_stack;
    core->mem.temp = ctk_create_stack(&perma_stack, CTK_KILOBYTE);

    return core;
}
