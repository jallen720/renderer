#pragma once

#include <windows.h>
#include <winuser.h>
#include <vulkan/vulkan.h>
#include "ctk/ctk.h"

// Defined because WinUser.h doesn't define these.
#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39
#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_N 0x4E
#define VK_O 0x4F
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x5A

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
struct WindowInfo {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    wchar_t const *title;
};

struct Window {
    HWND handle;
    MSG msg;
    bool open;
    bool key_down[KEY_COUNT];
};

struct Platform {
    Window *window;
    s32 key_map[KEY_COUNT];
};

static WindowInfo const PLATFORM_DEFAULT_WINDOW_INFO = {
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    L"win32 window",
};

// Windows used by window_callback().
static CTK_StaticArray<CTK_Pair<HWND, Window *>, 4> active_windows;

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static LRESULT CALLBACK window_callback(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM w_param, _In_ LPARAM l_param) {
    Window *window = ctk_find_value(hwnd, active_windows.data, active_windows.count);
    if (window != NULL) {
        switch (msg) {
            case WM_DESTROY: {
                PostQuitMessage(0);
                return 0;
            }
            case WM_PAINT: {
                PAINTSTRUCT paint_struct;
                HDC device_context = BeginPaint(hwnd, &paint_struct);
                FillRect(device_context, &paint_struct.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                EndPaint(hwnd, &paint_struct);
                return 0;
            }
            case WM_KEYDOWN: {
                ctk_print_line("WM_KEYDOWN");
                window->key_down[w_param] = true;
                return 0;
            }
            case WM_KEYUP: {
                ctk_print_line("WM_KEYUP");
                window->key_down[w_param] = false;
                return 0;
            }
            case WM_SYSKEYDOWN: {
                ctk_print_line("WM_SYSKEYDOWN");
                window->key_down[w_param] = true;
                break; // System keys should still be processed via DefWindowProc().
            }
            case WM_SYSKEYUP: {
                ctk_print_line("WM_SYSKEYUP");
                window->key_down[w_param] = false;
                break; // System keys should still be processed via DefWindowProc().
            }
        }
    }
    return DefWindowProc(hwnd, msg, w_param, l_param);
}

static Window *create_window(CTK_Stack *stack, WindowInfo info) {
    auto window = ctk_alloc<Window>(stack);
    window->open = true;
    HINSTANCE h_instance = GetModuleHandle(NULL);
    WNDCLASS win_class = {};
    win_class.lpfnWndProc = window_callback;
    win_class.hInstance = h_instance;
    win_class.lpszClassName = L"win32_window";
    RegisterClass(&win_class);
    window->handle = CreateWindowEx(0,                       // Optional window styles.
                                    L"win32_window",         // Window class
                                    info.title,              // Window text
                                    WS_OVERLAPPEDWINDOW,     // Window style
                                    info.x, info.y,          // Position
                                    info.width, info.height, // Size
                                    NULL,                    // Parent window
                                    NULL,                    // Menu
                                    h_instance,              // Instance handle
                                    NULL);                   // Additional application data
    if (window->handle == NULL)
        CTK_FATAL("failed to create window")
    ShowWindow(window->handle, SW_SHOW);
    ctk_push(&active_windows, { window->handle, window });
    return window;
}

static Platform *create_platform(CTK_Stack *stack) {
    auto platform = ctk_alloc<Platform>(stack);

    // Window
    WindowInfo window_info = PLATFORM_DEFAULT_WINDOW_INFO;
    window_info.title = L"Renderer";
    platform->window = create_window(stack, window_info);

    // Map Keys

    // Alpha-numeric Keys (defined in platform.h for consistency)
    platform->key_map[KEY_0] = VK_0;
    platform->key_map[KEY_1] = VK_1;
    platform->key_map[KEY_2] = VK_2;
    platform->key_map[KEY_3] = VK_3;
    platform->key_map[KEY_4] = VK_4;
    platform->key_map[KEY_5] = VK_5;
    platform->key_map[KEY_6] = VK_6;
    platform->key_map[KEY_7] = VK_7;
    platform->key_map[KEY_8] = VK_8;
    platform->key_map[KEY_9] = VK_9;
    platform->key_map[KEY_A] = VK_A;
    platform->key_map[KEY_B] = VK_B;
    platform->key_map[KEY_C] = VK_C;
    platform->key_map[KEY_D] = VK_D;
    platform->key_map[KEY_E] = VK_E;
    platform->key_map[KEY_F] = VK_F;
    platform->key_map[KEY_G] = VK_G;
    platform->key_map[KEY_H] = VK_H;
    platform->key_map[KEY_I] = VK_I;
    platform->key_map[KEY_J] = VK_J;
    platform->key_map[KEY_K] = VK_K;
    platform->key_map[KEY_L] = VK_L;
    platform->key_map[KEY_M] = VK_M;
    platform->key_map[KEY_N] = VK_N;
    platform->key_map[KEY_O] = VK_O;
    platform->key_map[KEY_P] = VK_P;
    platform->key_map[KEY_Q] = VK_Q;
    platform->key_map[KEY_R] = VK_R;
    platform->key_map[KEY_S] = VK_S;
    platform->key_map[KEY_T] = VK_T;
    platform->key_map[KEY_U] = VK_U;
    platform->key_map[KEY_V] = VK_V;
    platform->key_map[KEY_W] = VK_W;
    platform->key_map[KEY_X] = VK_X;
    platform->key_map[KEY_Y] = VK_Y;
    platform->key_map[KEY_Z] = VK_Z;

    // Mouse Buttons
    platform->key_map[KEY_MOUSE_0] = VK_LBUTTON;
    platform->key_map[KEY_MOUSE_1] = VK_RBUTTON;
    platform->key_map[KEY_CANCEL]  = VK_CANCEL;
    platform->key_map[KEY_MOUSE_2] = VK_MBUTTON; /* NOT contiguous with L & RBUTTON */
    #if(_WIN32_WINNT >= 0x0500)
        platform->key_map[KEY_MOUSE_3] = VK_XBUTTON1; /* NOT contiguous with L & RBUTTON */
        platform->key_map[KEY_MOUSE_4] = VK_XBUTTON2; /* NOT contiguous with L & RBUTTON */
    #endif /* _WIN32_WINNT >= 0x0500 */

    platform->key_map[KEY_BACK]       = VK_BACK;
    platform->key_map[KEY_TAB]        = VK_TAB;
    platform->key_map[KEY_CLEAR]      = VK_CLEAR;
    platform->key_map[KEY_RETURN]     = VK_RETURN;
    platform->key_map[KEY_SHIFT]      = VK_SHIFT;
    platform->key_map[KEY_CONTROL]    = VK_CONTROL;
    platform->key_map[KEY_MENU]       = VK_MENU;
    platform->key_map[KEY_PAUSE]      = VK_PAUSE;
    platform->key_map[KEY_CAPITAL]    = VK_CAPITAL;
    platform->key_map[KEY_KANA]       = VK_KANA;
    platform->key_map[KEY_HANGEUL]    = VK_HANGEUL;    /* old name - should be here for compatibility */
    platform->key_map[KEY_HANGUL]     = VK_HANGUL;
    platform->key_map[KEY_JUNJA]      = VK_JUNJA;
    platform->key_map[KEY_FINAL]      = VK_FINAL;
    platform->key_map[KEY_HANJA]      = VK_HANJA;
    platform->key_map[KEY_KANJI]      = VK_KANJI;
    platform->key_map[KEY_ESCAPE]     = VK_ESCAPE;
    platform->key_map[KEY_CONVERT]    = VK_CONVERT;
    platform->key_map[KEY_NONCONVERT] = VK_NONCONVERT;
    platform->key_map[KEY_ACCEPT]     = VK_ACCEPT;
    platform->key_map[KEY_MODECHANGE] = VK_MODECHANGE;
    platform->key_map[KEY_SPACE]      = VK_SPACE;
    platform->key_map[KEY_PRIOR]      = VK_PRIOR;
    platform->key_map[KEY_NEXT]       = VK_NEXT;
    platform->key_map[KEY_END]        = VK_END;
    platform->key_map[KEY_HOME]       = VK_HOME;
    platform->key_map[KEY_LEFT]       = VK_LEFT;
    platform->key_map[KEY_UP]         = VK_UP;
    platform->key_map[KEY_RIGHT]      = VK_RIGHT;
    platform->key_map[KEY_DOWN]       = VK_DOWN;
    platform->key_map[KEY_SELECT]     = VK_SELECT;
    platform->key_map[KEY_PRINT]      = VK_PRINT;
    platform->key_map[KEY_EXECUTE]    = VK_EXECUTE;
    platform->key_map[KEY_SNAPSHOT]   = VK_SNAPSHOT;
    platform->key_map[KEY_INSERT]     = VK_INSERT;
    platform->key_map[KEY_DELETE]     = VK_DELETE;
    platform->key_map[KEY_HELP]       = VK_HELP;
    platform->key_map[KEY_LWIN]       = VK_LWIN;
    platform->key_map[KEY_RWIN]       = VK_RWIN;
    platform->key_map[KEY_APPS]       = VK_APPS;
    platform->key_map[KEY_SLEEP]      = VK_SLEEP;
    platform->key_map[KEY_NUMPAD_0]   = VK_NUMPAD0;
    platform->key_map[KEY_NUMPAD_1]   = VK_NUMPAD1;
    platform->key_map[KEY_NUMPAD_2]   = VK_NUMPAD2;
    platform->key_map[KEY_NUMPAD_3]   = VK_NUMPAD3;
    platform->key_map[KEY_NUMPAD_4]   = VK_NUMPAD4;
    platform->key_map[KEY_NUMPAD_5]   = VK_NUMPAD5;
    platform->key_map[KEY_NUMPAD_6]   = VK_NUMPAD6;
    platform->key_map[KEY_NUMPAD_7]   = VK_NUMPAD7;
    platform->key_map[KEY_NUMPAD_8]   = VK_NUMPAD8;
    platform->key_map[KEY_NUMPAD_9]   = VK_NUMPAD9;
    platform->key_map[KEY_MULTIPLY]   = VK_MULTIPLY;
    platform->key_map[KEY_ADD]        = VK_ADD;
    platform->key_map[KEY_SEPARATOR]  = VK_SEPARATOR;
    platform->key_map[KEY_SUBTRACT]   = VK_SUBTRACT;
    platform->key_map[KEY_DECIMAL]    = VK_DECIMAL;
    platform->key_map[KEY_DIVIDE]     = VK_DIVIDE;
    platform->key_map[KEY_F1]         = VK_F1;
    platform->key_map[KEY_F2]         = VK_F2;
    platform->key_map[KEY_F3]         = VK_F3;
    platform->key_map[KEY_F4]         = VK_F4;
    platform->key_map[KEY_F5]         = VK_F5;
    platform->key_map[KEY_F6]         = VK_F6;
    platform->key_map[KEY_F7]         = VK_F7;
    platform->key_map[KEY_F8]         = VK_F8;
    platform->key_map[KEY_F9]         = VK_F9;
    platform->key_map[KEY_F10]        = VK_F10;
    platform->key_map[KEY_F11]        = VK_F11;
    platform->key_map[KEY_F12]        = VK_F12;
    platform->key_map[KEY_F13]        = VK_F13;
    platform->key_map[KEY_F14]        = VK_F14;
    platform->key_map[KEY_F15]        = VK_F15;
    platform->key_map[KEY_F16]        = VK_F16;
    platform->key_map[KEY_F17]        = VK_F17;
    platform->key_map[KEY_F18]        = VK_F18;
    platform->key_map[KEY_F19]        = VK_F19;
    platform->key_map[KEY_F20]        = VK_F20;
    platform->key_map[KEY_F21]        = VK_F21;
    platform->key_map[KEY_F22]        = VK_F22;
    platform->key_map[KEY_F23]        = VK_F23;
    platform->key_map[KEY_F24]        = VK_F24;
    #if(_WIN32_WINNT >= 0x0604)
        platform->key_map[KEY_NAVIGATION_VIEW]   = VK_NAVIGATION_VIEW;   // reserved
        platform->key_map[KEY_NAVIGATION_MENU]   = VK_NAVIGATION_MENU;   // reserved
        platform->key_map[KEY_NAVIGATION_UP]     = VK_NAVIGATION_UP;     // reserved
        platform->key_map[KEY_NAVIGATION_DOWN]   = VK_NAVIGATION_DOWN;   // reserved
        platform->key_map[KEY_NAVIGATION_LEFT]   = VK_NAVIGATION_LEFT;   // reserved
        platform->key_map[KEY_NAVIGATION_RIGHT]  = VK_NAVIGATION_RIGHT;  // reserved
        platform->key_map[KEY_NAVIGATION_ACCEPT] = VK_NAVIGATION_ACCEPT; // reserved
        platform->key_map[KEY_NAVIGATION_CANCEL] = VK_NAVIGATION_CANCEL; // reserved
    #endif /* _WIN32_WINNT >= 0x0604 */
    platform->key_map[KEY_NUMLOCK]      = VK_NUMLOCK;
    platform->key_map[KEY_SCROLL]       = VK_SCROLL;
    platform->key_map[KEY_NUMPAD_EQUAL] = VK_OEM_NEC_EQUAL;  // '='          key      on  numpad
    platform->key_map[KEY_FJ_JISHO]     = VK_OEM_FJ_JISHO;   // 'Dictionary' key
    platform->key_map[KEY_FJ_MASSHOU]   = VK_OEM_FJ_MASSHOU; // 'Unregister  word'    key
    platform->key_map[KEY_FJ_TOUROKU]   = VK_OEM_FJ_TOUROKU; // 'Register    word'    key
    platform->key_map[KEY_FJ_LOYA]      = VK_OEM_FJ_LOYA;    // 'Left        OYAYUBI' key
    platform->key_map[KEY_FJ_ROYA]      = VK_OEM_FJ_ROYA;    // 'Right       OYAYUBI' key
    platform->key_map[KEY_LSHIFT]       = VK_LSHIFT;
    platform->key_map[KEY_RSHIFT]       = VK_RSHIFT;
    platform->key_map[KEY_LCONTROL]     = VK_LCONTROL;
    platform->key_map[KEY_RCONTROL]     = VK_RCONTROL;
    platform->key_map[KEY_LMENU]        = VK_LMENU;
    platform->key_map[KEY_RMENU]        = VK_RMENU;
    #if(_WIN32_WINNT >= 0x0500)
        platform->key_map[KEY_BROWSER_BACK]        = VK_BROWSER_BACK;
        platform->key_map[KEY_BROWSER_FORWARD]     = VK_BROWSER_FORWARD;
        platform->key_map[KEY_BROWSER_REFRESH]     = VK_BROWSER_REFRESH;
        platform->key_map[KEY_BROWSER_STOP]        = VK_BROWSER_STOP;
        platform->key_map[KEY_BROWSER_SEARCH]      = VK_BROWSER_SEARCH;
        platform->key_map[KEY_BROWSER_FAVORITES]   = VK_BROWSER_FAVORITES;
        platform->key_map[KEY_BROWSER_HOME]        = VK_BROWSER_HOME;
        platform->key_map[KEY_VOLUME_MUTE]         = VK_VOLUME_MUTE;
        platform->key_map[KEY_VOLUME_DOWN]         = VK_VOLUME_DOWN;
        platform->key_map[KEY_VOLUME_UP]           = VK_VOLUME_UP;
        platform->key_map[KEY_MEDIA_NEXT_TRACK]    = VK_MEDIA_NEXT_TRACK;
        platform->key_map[KEY_MEDIA_PREV_TRACK]    = VK_MEDIA_PREV_TRACK;
        platform->key_map[KEY_MEDIA_STOP]          = VK_MEDIA_STOP;
        platform->key_map[KEY_MEDIA_PLAY_PAUSE]    = VK_MEDIA_PLAY_PAUSE;
        platform->key_map[KEY_LAUNCH_MAIL]         = VK_LAUNCH_MAIL;
        platform->key_map[KEY_LAUNCH_MEDIA_SELECT] = VK_LAUNCH_MEDIA_SELECT;
        platform->key_map[KEY_LAUNCH_APP1]         = VK_LAUNCH_APP1;
        platform->key_map[KEY_LAUNCH_APP2]         = VK_LAUNCH_APP2;
    #endif /* _WIN32_WINNT >= 0x0500 */
    platform->key_map[KEY_SEMICOLON_COLON] = VK_OEM_1;      // ';:' for US
    platform->key_map[KEY_PLUS]            = VK_OEM_PLUS;   // '+'  any country
    platform->key_map[KEY_COMMA]           = VK_OEM_COMMA;  // ','  any country
    platform->key_map[KEY_MINUS]           = VK_OEM_MINUS;  // '-'  any country
    platform->key_map[KEY_PERIOD]          = VK_OEM_PERIOD; // '.'  any country
    platform->key_map[KEY_SLASH_QUESTION]  = VK_OEM_2;      // '/?' for US
    platform->key_map[KEY_BACKTICK_TILDE]  = VK_OEM_3;      // '`~' for US
    #if(_WIN32_WINNT >= 0x0604)
        // Gamepad Input
        platform->key_map[KEY_GAMEPAD_A]                       = VK_GAMEPAD_A;                       // reserved
        platform->key_map[KEY_GAMEPAD_B]                       = VK_GAMEPAD_B;                       // reserved
        platform->key_map[KEY_GAMEPAD_X]                       = VK_GAMEPAD_X;                       // reserved
        platform->key_map[KEY_GAMEPAD_Y]                       = VK_GAMEPAD_Y;                       // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_SHOULDER]          = VK_GAMEPAD_RIGHT_SHOULDER;          // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_SHOULDER]           = VK_GAMEPAD_LEFT_SHOULDER;           // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_TRIGGER]            = VK_GAMEPAD_LEFT_TRIGGER;            // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_TRIGGER]           = VK_GAMEPAD_RIGHT_TRIGGER;           // reserved
        platform->key_map[KEY_GAMEPAD_DPAD_UP]                 = VK_GAMEPAD_DPAD_UP;                 // reserved
        platform->key_map[KEY_GAMEPAD_DPAD_DOWN]               = VK_GAMEPAD_DPAD_DOWN;               // reserved
        platform->key_map[KEY_GAMEPAD_DPAD_LEFT]               = VK_GAMEPAD_DPAD_LEFT;               // reserved
        platform->key_map[KEY_GAMEPAD_DPAD_RIGHT]              = VK_GAMEPAD_DPAD_RIGHT;              // reserved
        platform->key_map[KEY_GAMEPAD_MENU]                    = VK_GAMEPAD_MENU;                    // reserved
        platform->key_map[KEY_GAMEPAD_VIEW]                    = VK_GAMEPAD_VIEW;                    // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_THUMBSTICK_BUTTON]  = VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON;  // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_THUMBSTICK_BUTTON] = VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON; // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_THUMBSTICK_UP]      = VK_GAMEPAD_LEFT_THUMBSTICK_UP;      // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_THUMBSTICK_DOWN]    = VK_GAMEPAD_LEFT_THUMBSTICK_DOWN;    // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_THUMBSTICK_RIGHT]   = VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT;   // reserved
        platform->key_map[KEY_GAMEPAD_LEFT_THUMBSTICK_LEFT]    = VK_GAMEPAD_LEFT_THUMBSTICK_LEFT;    // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_THUMBSTICK_UP]     = VK_GAMEPAD_RIGHT_THUMBSTICK_UP;     // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_THUMBSTICK_DOWN]   = VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN;   // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_THUMBSTICK_RIGHT]  = VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT;  // reserved
        platform->key_map[KEY_GAMEPAD_RIGHT_THUMBSTICK_LEFT]   = VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT;   // reserved
    #endif /* _WIN32_WINNT >= 0x0604 */

    platform->key_map[KEY_OPEN_BRACKET]     = VK_OEM_4; // '[{' for US
    platform->key_map[KEY_BACKSLASH_PIPE]   = VK_OEM_5; // '\|' for US
    platform->key_map[KEY_CLOSE_BRACKET]    = VK_OEM_6; // ']}' for US
    platform->key_map[KEY_APOSTROPHE_QUOTE] = VK_OEM_7; // ''"' for US
    platform->key_map[KEY_OEM_8]            = VK_OEM_8;

    // Various Extended or Enhanced Keyboards
    platform->key_map[KEY_AX]       = VK_OEM_AX;   // 'AX' key on  Japanese AX kbd
    platform->key_map[KEY_102]      = VK_OEM_102;  // "<>" or  "\| "        on RT  102-key kbd.
    platform->key_map[KEY_ICO_HELP] = VK_ICO_HELP; // Help key on  ICO
    platform->key_map[KEY_ICO_00]   = VK_ICO_00;   // 00   key on  ICO

    return platform;
}

static void process_events(Window *window) {
    window->open = GetMessage(&window->msg, NULL, 0, 0); // Stay open as long as WM_QUIT message isn't generated.
    if (!window->open)
        return;

    TranslateMessage(&window->msg);
    DispatchMessage(&window->msg);
}

static bool key_down(Platform *platform, s32 core_key) {
    return platform->window->key_down[platform->key_map[core_key]];
}

static cstr get_vulkan_extensions() {
    return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}
