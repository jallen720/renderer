#include "renderer/platform.h"
#include "renderer/vulkan.h"
#include "ctk/math.h"

struct App {
    struct {
        CTK_Stack *fixed;
        CTK_Stack *temp;
    } mem;

    struct {
        CTK_Allocator *fixed;
        CTK_Allocator *temp;
    } alloc;

    struct {
        Buffer *host;
        Buffer *device;
    } buffer;

    struct {
        Region *staging;
        Region *mesh;
    } region;

    CTK_Array<CTK_Vec3<f32>> *vertexes;
};

static void create_buffers(App *app, Vulkan *vk) {
    {
        BufferInfo info = {};
        info.size = 256 * CTK_MEGABYTE;
        info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        app->buffer.host = create_buffer(vk, &info);
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
        app->buffer.device = create_buffer(vk, &info);
    }
}

static void allocate_regions(App *app, Vulkan *vk) {
    app->region.staging = allocate_region(vk, app->buffer.host, 64 * CTK_MEGABYTE);
    app->region.mesh = allocate_region(vk, app->buffer.host, 64, 64);
}

static App *create_app(CTK_Allocator *fixed_alloc, Vulkan *vk) {
    auto app = ctk_alloc<App>(fixed_alloc, 1);
    app->alloc.fixed = fixed_alloc;
    app->mem.temp = ctk_create_stack(app->alloc.fixed, CTK_MEGABYTE);
    app->alloc.temp = ctk_create_allocator(app->mem.temp);

    create_buffers(app, vk);
    allocate_regions(app, vk);

    return app;
}

static void test_data(App *app, Vulkan *vk) {
    app->vertexes = ctk_create_array<CTK_Vec3<f32>>(app->alloc.fixed, 64);
    ctk_push(app->vertexes, { -0.4f, -0.4f, 0 });
    ctk_push(app->vertexes, {  0,     0.4f, 0 });
    ctk_push(app->vertexes, {  0.4f, -0.4f, 0 });

    write_to_host_region(vk, app->region.mesh, app->vertexes->data, app->vertexes->count);
}

s32 main() {
    CTK_Stack *base_mem = ctk_create_stack(CTK_GIGABYTE);
    CTK_Allocator *base_alloc = ctk_create_allocator(base_mem);

    // Platform
    CTK_Stack *platform_stack = ctk_create_stack(base_alloc, 2 * CTK_KILOBYTE);
    CTK_Allocator *platform_alloc = ctk_create_allocator(platform_stack);
    Platform *platform = create_platform(platform_alloc);

    // Vulkan
    CTK_Stack *vulkan_stack = ctk_create_stack(base_alloc, 4 * CTK_MEGABYTE);
    CTK_Allocator *vulkan_alloc = ctk_create_allocator(vulkan_stack);
    VulkanInfo vulkan_info = {};
    vulkan_info.max_buffers = 2;
    vulkan_info.max_regions = 32;
    Vulkan *vk = create_vulkan(vulkan_alloc, &vulkan_info, platform);

    // App
    App *app = create_app(base_alloc, vk);

    // Setup Test Data
    test_data(app, vk);

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
