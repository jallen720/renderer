#include "renderer/core.h"
#include "renderer/vulkan.h"
#include "renderer/platform.h"

s32 main() {
    Core *core = create_core();
    Platform *platform = create_platform(&core->mem.perma);
    Vulkan* vulkan = create_vulkan(core, platform);

    // Main Loop
    while (platform->window->open) {
        // Handle Input
        process_events(platform->window);
        if (key_down(platform, INPUT_KEY_ESCAPE))
            break;

        // ...

    }

    return 0;
}
