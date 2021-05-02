#include "renderer/platform.h"
#include "renderer/vulkan.h"
#include "ctk/math.h"

struct App {
    CTK_Stack *fixed_stack;
    CTK_Allocator *fixed_alloc;
    CTK_Stack *temp_stack;
    CTK_Allocator *temp_alloc;

    struct {
        CTK_Stack *platform;
        CTK_Stack *vulkan;
    } module_stack;

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
    CTK_Stack *fixed_stack = ctk_create_stack(CTK_GIGABYTE);

    auto app = ctk_alloc<App>(fixed_stack, 1);
    app->fixed_stack = fixed_stack;
    app->fixed_alloc = ctk_create_allocator(app->fixed_stack);
    app->temp_stack = ctk_create_stack(app->fixed_alloc, CTK_MEGABYTE);
    app->temp_alloc = ctk_create_allocator(app->temp_stack);
    app->module_stack.platform = ctk_create_stack(app->fixed_alloc, 2 * CTK_KILOBYTE);
    app->module_stack.vulkan = ctk_create_stack(app->fixed_alloc, 4 * CTK_MEGABYTE);

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
    app->vertexes = ctk_create_array<CTK_Vec3<f32>>(app->fixed_alloc, 64);
    ctk_push(app->vertexes, { -0.4f, -0.4f, 0 });
    ctk_push(app->vertexes, {  0,     0.4f, 0 });
    ctk_push(app->vertexes, {  0.4f, -0.4f, 0 });

    write_to_host_region(vk, app->region.mesh, app->vertexes->data, app->vertexes->count);
}

static void create_render_passes(App *app, Vulkan *vk) {
    {
        ctk_push_frame(app->temp_stack);

        RenderPassInfo info = {};
        info.attachment_descriptions = ctk_create_array<VkAttachmentDescription>(vk->temp_alloc, 1);
        info.subpass_infos = ctk_create_array<SubpassInfo>(vk->temp_alloc, 1);
        info.subpass_dependencies = ctk_create_array<VkSubpassDependency>(vk->temp_alloc, 1);
        info.framebuffer_infos = ctk_create_array<FramebufferInfo>(vk->temp_alloc, 1);
        info.clear_values = ctk_create_array<VkClearValue>(vk->temp_alloc, 1);

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

        ctk_pop_frame(app->temp_stack);
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
    Platform *platform = create_platform(app->module_stack.platform);
    Vulkan *vk = create_vulkan(app->module_stack.vulkan, platform, { .max_buffers = 2, .max_regions = 32 });

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
