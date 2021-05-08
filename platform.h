#pragma once

#include <windows.h>
#include <winuser.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "renderer/inputs.h"

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
    bool open;
    bool key_down[INPUT_KEY_COUNT];
};

struct Platform {
    CTK_Allocator *module_mem;
    HINSTANCE instance;
    Window *window;
    s32 key_map[INPUT_KEY_COUNT];
};

static WindowInfo const PLATFORM_DEFAULT_WINDOW_INFO = {
    .x = CW_USEDEFAULT,
    .y = CW_USEDEFAULT,
    .width = CW_USEDEFAULT,
    .height = CW_USEDEFAULT,
    .title = L"win32 window",
};

////////////////////////////////////////////////////////////
/// Key Mapping
////////////////////////////////////////////////////////////
#include "renderer/win32_keymap.h"
// Windows used by window_callback().
static CTK_FixedArray<CTK_Pair<HWND, Window *>, 4> active_windows;

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static LRESULT CALLBACK window_callback(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM w_param, _In_ LPARAM l_param) {
    Window *window = ctk_find_value(active_windows.data, active_windows.count, hwnd);
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

static Window *create_window(Platform *platform, WindowInfo info) {
    static wchar_t const *CLASS_NAME = L"win32_window";

    WNDCLASS win_class = {};
    win_class.lpfnWndProc = window_callback;
    win_class.hInstance = platform->instance;
    win_class.lpszClassName = CLASS_NAME;
    RegisterClass(&win_class);

    auto window = ctk_alloc<Window>(platform->module_mem, 1);
    window->open = true;
    window->handle = CreateWindowEx(0,                       // Optional window styles.
                                    CLASS_NAME,              // Window class
                                    info.title,              // Window text
                                    WS_OVERLAPPEDWINDOW,     // Window style
                                    info.x,     info.y,      // Position
                                    info.width, info.height, // Size
                                    NULL,                    // Parent window
                                    NULL,                    // Menu
                                    platform->instance,      // Instance handle
                                    NULL);                   // Additional application data

    if (window->handle == NULL)
        CTK_FATAL("failed to create window")

    ShowWindow(window->handle, SW_SHOW);
    ctk_push(&active_windows, { window->handle, window });
    return window;
}

static Platform *create_platform(CTK_Allocator *module_mem) {
    auto platform = ctk_alloc<Platform>(module_mem, 1);
    platform->module_mem = module_mem;

    // Instance
    platform->instance = GetModuleHandle(NULL);

    // Window
    WindowInfo window_info = PLATFORM_DEFAULT_WINDOW_INFO;
    window_info.title = L"Renderer";
    platform->window = create_window(platform, window_info);

    // Map Keys to WIN32 Keys
    map_keys(platform);


    return platform;
}

static void process_events(Window *window) {
    MSG msg;
    window->open = GetMessage(&msg, window->handle, 0, 0); // Stay open as long as WM_QUIT message isn't generated.
    if (!window->open)
        return;

    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

static bool key_down(Platform *platform, s32 key) {
    return platform->window->key_down[platform->key_map[key]];
}
