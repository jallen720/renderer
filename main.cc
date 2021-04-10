#include "renderer/platform.h"
#include "renderer/vulkan.h"

struct App {
    struct {
        CTK_Stack *base;
        CTK_Stack *temp;
    } mem;

    struct {
        Buffer *host;
        Buffer *device;
    } buffers;

    struct {
        Region *staging;
        Region *mesh;
    } regions;
};

static void create_buffers(App *app, Vulkan *vulkan) {
    {
        BufferInfo info = {};
        info.size = 256 * CTK_MEGABYTE;
        info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        app->buffers.host = create_buffer(vulkan, &info);
    }

    {
        BufferInfo info = {};
        info.size = 256 * CTK_MEGABYTE;
        info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        app->buffers.device = create_buffer(vulkan, &info);
    }
}

static void allocate_regions(App *app, Vulkan *vulkan) {
    app->regions.staging = allocate_region(vulkan, app->buffers.host, 64 * CTK_MEGABYTE);
    app->regions.mesh = allocate_region(vulkan, app->buffers.device, 64, 64);
}

static App *create_app(Vulkan *vulkan) {
    CTK_Stack *base = ctk_create_stack(CTK_GIGABYTE);
    auto app = ctk_alloc<App>(base, 1);
    app->mem.base = base;
    app->mem.temp = ctk_create_stack(CTK_MEGABYTE, &base->allocator);

    create_buffers(app, vulkan);
    allocate_regions(app, vulkan);

    return app;
}

s32 main() {
    // Init
    Platform *platform = create_platform();
    Vulkan *vulkan = create_vulkan(platform);
    App *app = create_app(vulkan);

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
