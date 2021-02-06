#pragma once

#include <windows.h>
#include <winuser.h>
#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"

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
    bool open;
    bool key_down[INPUT_KEY_COUNT];
};

struct Platform {
    HINSTANCE instance;
    Window *window;
    s32 key_map[INPUT_KEY_COUNT];
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

static cstr PLATFORM_VULKAN_EXTENSIONS[] = { VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
static constexpr u32 PLATFORM_VULKAN_EXTENSION_COUNT = CTK_ARRAY_COUNT(PLATFORM_VULKAN_EXTENSIONS);

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

static Window *create_window(CTK_Stack *stack, Platform *platform, WindowInfo info) {
    static wchar_t const *CLASS_NAME = L"win32_window";

    WNDCLASS win_class = {};
    win_class.lpfnWndProc = window_callback;
    win_class.hInstance = platform->instance;
    win_class.lpszClassName = CLASS_NAME;
    RegisterClass(&win_class);

    auto window = ctk_alloc<Window>(stack, 1);
    window->open = true;
    window->handle = CreateWindowEx(0,                       // Optional window styles.
                                    CLASS_NAME,              // Window class
                                    info.title,              // Window text
                                    WS_OVERLAPPEDWINDOW,     // Window style
                                    info.x, info.y,          // Position
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

static Platform *create_platform(CTK_Stack *stack) {
    auto platform = ctk_alloc<Platform>(stack, 1);
    platform->instance = GetModuleHandle(NULL);

    // Window
    WindowInfo window_info = PLATFORM_DEFAULT_WINDOW_INFO;
    window_info.title = L"Renderer";
    platform->window = create_window(stack, platform, window_info);

    // Map Keys
    #include "renderer/win32_keymap.h"

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

static bool key_down(Platform *platform, s32 core_key) {
    return platform->window->key_down[platform->key_map[core_key]];
}
