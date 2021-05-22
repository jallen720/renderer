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

    CTK_Array<CTK_Vec3<f32>> *vertexes;
};

struct ShaderGroup {
    Shader *vert;
    Shader *frag;
};

struct Graphics {
    struct {
        Buffer *host;
        Buffer *device;
    } buffer;

    struct {
        Region *staging;
        Region *mesh;
    } region;

    RenderPass *main_render_pass;
    CTK_Array<VkFramebuffer> *swapchain_framebufs;
    CTK_Array<VkCommandBuffer> *render_cmd_bufs;

    struct {
        ShaderGroup triangle;
    } shader;

    struct {
        VkDescriptorPool pool;

        struct {
            VkDescriptorSetLayout triangle;
        } set_layout;
    } descriptor;

    struct {
        Pipeline *direct;
    } pipeline;

};

static void create_buffers(Graphics *gfx, Vulkan *vk) {
    {
        BufferInfo info = {};
        info.size = 256 * CTK_MEGABYTE;
        info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        gfx->buffer.host = create_buffer(vk, &info);
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
        gfx->buffer.device = create_buffer(vk, &info);
    }
}

static void allocate_regions(Graphics *gfx, Vulkan *vk) {
    gfx->region.staging = allocate_region(vk, gfx->buffer.host, 64 * CTK_MEGABYTE);
    gfx->region.mesh = allocate_region(vk, gfx->buffer.host, 64, 64);
}

static u32 push_attachment(RenderPassInfo *info, AttachmentInfo attachment_info) {
    if (info->attachment.descriptions->count == info->attachment.descriptions->size)
        CTK_FATAL("cannot push any more attachments to RenderPassInfo");

    u32 attachment_index = info->attachment.descriptions->count;

    ctk_push(info->attachment.descriptions, attachment_info.description);
    ctk_push(info->attachment.clear_values, attachment_info.clear_value);

    return attachment_index;
}

static void create_render_passes(Graphics *gfx, App *app, Vulkan *vk) {
    {
        ctk_push_frame(app->mem.temp);

        RenderPassInfo info = {
            .attachment = {
                .descriptions = ctk_create_array<VkAttachmentDescription>(app->mem.temp, 1),
                .clear_values = ctk_create_array<VkClearValue>(app->mem.temp, 1),
            },
            .subpass = {
                .infos = ctk_create_array<SubpassInfo>(app->mem.temp, 1),
                .dependencies = ctk_create_array<VkSubpassDependency>(app->mem.temp, 1),
            },
        };

        // Swapchain Image Attachment
        u32 swapchain_attachment_index = push_attachment(&info, {
            .description = {
                .flags          = 0,
                .format         = vk->swapchain.image_format,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            .clear_value = { 0, 0, 0, 1 },
        });

        // Subpasses
        SubpassInfo *subpass_info = ctk_push(info.subpass.infos);
        subpass_info->color_attachment_references = ctk_create_array<VkAttachmentReference>(app->mem.temp, 1);
        ctk_push(subpass_info->color_attachment_references, {
            .attachment = swapchain_attachment_index,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });

        gfx->main_render_pass = create_render_pass(vk, &info);

        ctk_pop_frame(app->mem.temp);
    }
}

static void create_framebuffers(Graphics *gfx, App *app, Vulkan *vk) {
    {
        gfx->swapchain_framebufs = ctk_create_array<VkFramebuffer>(app->mem.fixed, vk->swapchain.image_count);

        // Create framebuffer for each swapchain image.
        for (u32 i = 0; i < vk->swapchain.image_views.count; ++i) {
            ctk_push_frame(app->mem.temp);

            FramebufferInfo info = {
                .attachments = ctk_create_array<VkImageView>(app->mem.temp, 1),
                .extent      = get_surface_extent(vk),
                .layers      = 1,
            };

            ctk_push(info.attachments, vk->swapchain.image_views[i]);

            ctk_push(gfx->swapchain_framebufs,
                create_framebuffer(vk->device.logical.handle, gfx->main_render_pass->handle, &info));

            ctk_pop_frame(app->mem.temp);
        }
    }
}

static void create_shaders(Graphics *gfx, Vulkan *vk) {
    gfx->shader = {
        .triangle = {
            .vert = create_shader(vk, "data/shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            .frag = create_shader(vk, "data/shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
        },
    };
}

static void create_descriptor_sets(Graphics *gfx, Vulkan *vk) {
    gfx->descriptor.pool = create_descriptor_pool(vk, {
        .descriptor_count = {
            .uniform_buffer = 4,
            // .uniform_buffer_dynamic = 0,
            // .combined_image_sampler = 0,
            // .input_attachment = 0,
        },
        .max_descriptor_sets = 64,
    });


}

static void create_pipelines(Graphics *gfx, App *app, Vulkan *vk) {
    {
        ctk_push_frame(app->mem.temp);

        VkExtent2D surface_extent = get_surface_extent(vk);

        PipelineInfo info = DEFAULT_PIPELINE_INFO;
        ctk_push(&info.shaders, gfx->shader.triangle.vert);
        ctk_push(&info.viewports, {
            .x        = 0,
            .y        = 0,
            .width    = (f32)surface_extent.width,
            .height   = (f32)surface_extent.height,
            .minDepth = 1,
            .maxDepth = 0,
        });
        ctk_push(&info.scissors, { .offset = { 0, 0 }, .extent = surface_extent });
        ctk_push(&info.color_blend_attachments, DEFAULT_COLOR_BLEND_ATTACHMENT);

        gfx->pipeline.direct = create_pipeline(vk, gfx->main_render_pass, 0, &info);

        ctk_pop_frame(app->mem.temp);
    }
}

static Graphics *create_graphics(App *app, Vulkan *vk) {
    Graphics *gfx = ctk_alloc<Graphics>(app->mem.fixed, 1);
    create_buffers(gfx, vk);
    allocate_regions(gfx, vk);
    create_render_passes(gfx, app, vk);
    create_framebuffers(gfx, app, vk);
    create_shaders(gfx, vk);
    create_descriptor_sets(gfx, vk);
    create_pipelines(gfx, app, vk);
    return gfx;
}

static void create_test_data(App *app, Graphics *gfx, Vulkan *vk) {
    app->vertexes = ctk_create_array<CTK_Vec3<f32>>(app->mem.fixed, 64);
    ctk_push(app->vertexes, { -0.4f, -0.4f, 0 });
    ctk_push(app->vertexes, {  0,     0.4f, 0 });
    ctk_push(app->vertexes, {  0.4f, -0.4f, 0 });

    write_to_host_region(vk, gfx->region.mesh, app->vertexes->data, app->vertexes->count);
}

s32 main() {
    // Create App.
    CTK_Allocator *fixed_mem = ctk_create_stack_allocator(1 * CTK_GIGABYTE);
    auto app = ctk_alloc<App>(fixed_mem, 1);
    app->mem.fixed    = fixed_mem;
    app->mem.temp     = ctk_create_stack_allocator(app->mem.fixed, CTK_MEGABYTE);
    app->mem.platform = ctk_create_stack_allocator(app->mem.fixed, 2 * CTK_KILOBYTE);
    app->mem.vulkan   = ctk_create_stack_allocator(app->mem.fixed, 4 * CTK_MEGABYTE);

    // Create Modules
    Platform *platform = create_platform(app->mem.platform, {
        .surface = {
            .x      = 2560 - 1920,   // Right Align
            .y      = 60,            // Align to Top Taskbar
            .width  = 1920,
            .height = 1080,
        },
        .title = L"Renderer",
    });

    Vulkan *vk = create_vulkan(app->mem.vulkan, platform, {
        .max_buffers       = 2,
        .max_regions       = 32,
        .max_render_passes = 2,
        .max_shaders       = 16,
        .max_pipelines     = 8,
    });

    Graphics *gfx = create_graphics(app, vk);

    // Test
    create_test_data(app, gfx, vk);

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
