#include "ctk/ctk.h"
#include "renderer/core.h"
#include "renderer/vulkan.h"
#include "renderer/platform.h"

s32 main() {
    Core *core = create_core();
    Platform *platform = create_platform(core->mem.perma);
    Vulkan *vulkan = create_vulkan(core);
    ctk_visualize_stack(core->mem.perma, "perma");
    ctk_visualize_stack(core->mem.temp, "temp");

    // Main Loop
    while (platform->window->open) {
        process_events(platform->window);
        if (key_down(platform, KEY_ESCAPE))
            break;
    }

    return 0;
}
