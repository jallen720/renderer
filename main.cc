#include "renderer/platform.h"
#include "renderer/vulkan.h"
#include "ctk/math.h"

struct App {
    struct {
        CTK_Allocator *fixed;
        CTK_Allocator *temp;
        CTK_Allocator *platform;
        CTK_Allocator *vulkan;
    } mem;

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

static App *create_app() {
    CTK_Allocator *fixed_mem = ctk_create_stack_allocator(CTK_GIGABYTE);

    auto app = ctk_alloc<App>(fixed_mem, 1);
    app->mem.fixed      = fixed_mem;
    app->mem.temp       = ctk_create_stack_allocator(app->mem.fixed, CTK_MEGABYTE);
    app->mem.platform   = ctk_create_stack_allocator(app->mem.fixed, 2 * CTK_KILOBYTE);
    app->mem.vulkan     = ctk_create_stack_allocator(app->mem.fixed, 4 * CTK_MEGABYTE);

    return app;
}

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

static void create_test_data(App *app, Vulkan *vk) {
    app->vertexes = ctk_create_array<CTK_Vec3<f32>>(app->mem.fixed, 64);
    ctk_push(app->vertexes, { -0.4f, -0.4f, 0 });
    ctk_push(app->vertexes, {  0,     0.4f, 0 });
    ctk_push(app->vertexes, {  0.4f, -0.4f, 0 });

    write_to_host_region(vk, app->region.mesh, app->vertexes->data, app->vertexes->count);
}

static void create_render_passes(App *app, Vulkan *vk) {
    {
        ctk_push_frame(app->mem.temp);

        RenderPassInfo info = {};
        info.attachment_descriptions = ctk_create_array<VkAttachmentDescription>(vk->mem.temp, 1);
        info.subpass_infos = ctk_create_array<SubpassInfo>(vk->mem.temp, 1);
        info.subpass_dependencies = ctk_create_array<VkSubpassDependency>(vk->mem.temp, 1);
        info.framebuffer_infos = ctk_create_array<FramebufferInfo>(vk->mem.temp, 1);
        info.clear_values = ctk_create_array<VkClearValue>(vk->mem.temp, 1);

        // Swapchain Image Attachment
        ctk_push(info.attachment_descriptions, {
            .flags = 0,
            .format = vk->swapchain.image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
        ctk_push(info.clear_values, { 0, 0, 0, 1 });

        ctk_pop_frame(app->mem.temp);
    }
}

static void init_app(App *app, Vulkan *vk) {
    create_buffers(app, vk);
    allocate_regions(app, vk);
    create_render_passes(app, vk);

    create_test_data(app, vk);
}

s32 main() {
    App *app = create_app();
    Platform *platform = create_platform(app->mem.platform);
    Vulkan *vk = create_vulkan(app->mem.vulkan, platform, { .max_buffers = 2, .max_regions = 32 });

    init_app(app, vk);

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
