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
    struct {
        s32 x;
        s32 y;
        s32 width;
        s32 height;
    } surface;
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

static Platform *_instance;

////////////////////////////////////////////////////////////
/// Key Mapping
////////////////////////////////////////////////////////////
#include "renderer/win32_keymap.h"

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static LRESULT CALLBACK window_callback(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM w_param, _In_ LPARAM l_param) {
    if (_instance && _instance->window->handle == hwnd) {
        switch (msg) {
            case WM_QUIT: {
                ctk_print_line("WM_QUIT");
                break;
            }
            case WM_DESTROY: {
                _instance->window->open = false;
                PostQuitMessage(0);
                break;
            }
            case WM_PAINT: {
                PAINTSTRUCT paint_struct = {};
                HDC device_context = BeginPaint(hwnd, &paint_struct);
                FillRect(device_context, &paint_struct.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
                EndPaint(hwnd, &paint_struct);
                break;
            }
            case WM_KEYDOWN: {
                ctk_print_line("WM_KEYDOWN");
                _instance->window->key_down[w_param] = true;
                break;
            }
            case WM_KEYUP: {
                ctk_print_line("WM_KEYUP");
                _instance->window->key_down[w_param] = false;
                break;
            }
            case WM_SYSKEYDOWN: {
                ctk_print_line("WM_SYSKEYDOWN");
                _instance->window->key_down[w_param] = true;
                break; // System keys should still be processed via DefWindowProc().
            }
            case WM_SYSKEYUP: {
                ctk_print_line("WM_SYSKEYUP");
                _instance->window->key_down[w_param] = false;
                break; // System keys should still be processed via DefWindowProc().
            }
        }
    }

    return DefWindowProc(hwnd, msg, w_param, l_param);
}

static void create_window(Platform *platform, WindowInfo info) {
    static wchar_t const *CLASS_NAME = L"win32_window";
    static DWORD const WINDOW_STYLE = WS_OVERLAPPEDWINDOW;

    // Calculate window rect based on surface rect.
    RECT window_rect = {
        .left   = info.surface.x,
        .top    = info.surface.y,
        .right  = info.surface.x + info.surface.width,
        .bottom = info.surface.y + info.surface.height,
    };

    AdjustWindowRectEx(&window_rect, WINDOW_STYLE, 0, 0);

    WNDCLASS win_class = {};
    win_class.lpfnWndProc = window_callback;
    win_class.hInstance = platform->instance;
    win_class.lpszClassName = CLASS_NAME;
    RegisterClass(&win_class);

    platform->window = ctk_alloc<Window>(platform->module_mem, 1);
    platform->window->open = true;
    platform->window->handle = CreateWindowEx(0,                                    // Optional Styles
                                              CLASS_NAME,                           // Class
                                              info.title,                           // Text
                                              WINDOW_STYLE,                         // Style
                                              window_rect.left,                     // X
                                              window_rect.top,                      // Y
                                              window_rect.right - window_rect.left, // Width
                                              window_rect.bottom - window_rect.top, // Height
                                              NULL,                                 // Parent
                                              NULL,                                 // Menu
                                              platform->instance,                   // Instance Handle
                                              NULL);                                // App Data

    if (platform->window->handle == NULL)
        CTK_FATAL("failed to create window");

    ShowWindow(platform->window->handle, SW_SHOW);
}

static Platform *create_platform(CTK_Allocator *module_mem, WindowInfo window_info) {
    if (_instance)
        CTK_FATAL("a Platform instance has already been created");

    auto platform = ctk_alloc<Platform>(module_mem, 1);
    platform->module_mem = module_mem;

    platform->instance = GetModuleHandle(NULL);

    // Window
    create_window(platform, window_info);

    // Map Keys to WIN32 Keys
    map_keys(platform);

    // Store platform instance for use with window callbacks.
    _instance = platform;

    return platform;
}

static void process_events(Window *window) {
    MSG msg;
    if (PeekMessage(&msg, window->handle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static bool key_down(Platform *platform, s32 key) {
    return platform->window->key_down[platform->key_map[key]];
}
