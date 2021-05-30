#include "renderer/platform.h"
#include "renderer/vulkan.h"
#include "ctk/math.h"

using namespace ctk;

struct App {
    struct {
        Allocator *fixed;
        Allocator *temp;
        Allocator *platform;
        Allocator *vulkan;
    } mem;
};

struct ShaderGroup {
    Shader *vert;
    Shader *frag;
};

struct Frame {
    VkSemaphore img_aquired;
    VkSemaphore render_finished;
    VkFence     in_flight;
};

struct Mesh {
    Array<Vec3<f32>> *vertexes;
    Array<u32> *indexes;
    Region *vertex_region;
    Region *index_region;
};

struct Graphics {
    struct {
        Buffer *host;
        Buffer *device;
    } buffer;

    struct {
        Region *staging;
        Region *vertexes;
        Region *indexes;
    } region;

    RenderPass *main_render_pass;

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

    struct {
        Array<VkFramebuffer> *framebufs;
        Array<VkCommandBuffer> *render_cmd_bufs;
    } swap_state;

    struct {
        Array<Frame> *frames;
        Frame *frame;
        u32 swap_img_idx;
        u32 curr_frame_idx;
    } sync;

    VkCommandBuffer temp_cmd_buf;

    Array<Mesh> *meshes;
};

static void create_buffers(Graphics *gfx, Vulkan *vk) {
    {
        BufferInfo info = {};
        info.size = megabyte(256);
        info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                           // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                           // VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        info.mem_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        gfx->buffer.host = create_buffer(vk, &info);
    }
    {
        BufferInfo info = {};
        info.size = megabyte(256);
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
    gfx->region.staging  = allocate_region(vk, gfx->buffer.host, megabyte(64));
    gfx->region.vertexes = allocate_region(vk, gfx->buffer.host, megabyte(), 64);
    gfx->region.indexes  = allocate_region(vk, gfx->buffer.host, megabyte(), 64);
}

static u32 push_attachment(RenderPassInfo *info, AttachmentInfo attachment_info) {
    if (info->attachment.descriptions->count == info->attachment.descriptions->size)
        CTK_FATAL("cannot push any more attachments to RenderPassInfo");

    u32 attachment_index = info->attachment.descriptions->count;

    push(info->attachment.descriptions, attachment_info.description);
    push(info->attachment.clear_values, attachment_info.clear_value);

    return attachment_index;
}

static void create_render_passes(Graphics *gfx, Vulkan *vk, App *app) {
    {
        push_frame(app->mem.temp);

        RenderPassInfo info = {
            .attachment = {
                .descriptions = create_array<VkAttachmentDescription>(app->mem.temp, 1),
                .clear_values = create_array<VkClearValue>(app->mem.temp, 1),
            },
            .subpass = {
                .infos = create_array<SubpassInfo>(app->mem.temp, 1),
                .dependencies = create_array<VkSubpassDependency>(app->mem.temp, 1),
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
                .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            .clear_value = { 0, 0, 0, 1 },
        });

        // Subpasses
        SubpassInfo *subpass_info = push(info.subpass.infos);
        subpass_info->color_attachment_references = create_array<VkAttachmentReference>(app->mem.temp, 1);
        push(subpass_info->color_attachment_references, {
            .attachment = swapchain_attachment_index,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });

        gfx->main_render_pass = create_render_pass(vk, &info);

        pop_frame(app->mem.temp);
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

static void create_pipelines(Graphics *gfx, Vulkan *vk, App *app) {
    {
        push_frame(app->mem.temp);

        VkExtent2D surface_extent = get_surface_extent(vk);

        PipelineInfo info = DEFAULT_PIPELINE_INFO;
        push(&info.shaders, gfx->shader.triangle.vert);
        push(&info.shaders, gfx->shader.triangle.frag);
        push(&info.viewports, {
            .x        = 0,
            .y        = 0,
            .width    = (f32)surface_extent.width,
            .height   = (f32)surface_extent.height,
            .minDepth = 1,
            .maxDepth = 0,
        });
        push(&info.scissors, { .offset = { 0, 0 }, .extent = surface_extent });
        push(&info.color_blend_attachments, DEFAULT_COLOR_BLEND_ATTACHMENT);

        gfx->pipeline.direct = create_pipeline(vk, gfx->main_render_pass, 0, &info);

        pop_frame(app->mem.temp);
    }
}

static void init_swap_state(Graphics *gfx, Vulkan *vk, App *app) {
    {
        gfx->swap_state.framebufs = create_array<VkFramebuffer>(app->mem.fixed, vk->swapchain.image_count);

        // Create framebuffer for each swapchain image.
        for (u32 i = 0; i < vk->swapchain.image_views.count; ++i) {
            push_frame(app->mem.temp);

            FramebufferInfo info = {
                .attachments = create_array<VkImageView>(app->mem.temp, 1),
                .extent      = get_surface_extent(vk),
                .layers      = 1,
            };

            push(info.attachments, vk->swapchain.image_views[i]);
            push(gfx->swap_state.framebufs, create_framebuf(vk->device, gfx->main_render_pass->handle, &info));

            pop_frame(app->mem.temp);
        }
    }

    gfx->swap_state.render_cmd_bufs = alloc_cmd_bufs(vk, VK_COMMAND_BUFFER_LEVEL_PRIMARY, vk->swapchain.image_count);
}

static void init_sync(Graphics *gfx, App *app, Vulkan *vk, u32 frame_count) {
    gfx->sync.curr_frame_idx = U32_MAX;
    gfx->sync.frames = create_array<Frame>(app->mem.fixed, frame_count);

    for (u32 i = 0; i < frame_count; ++i) {
        push(gfx->sync.frames, {
            .img_aquired     = create_semaphore(vk),
            .render_finished = create_semaphore(vk),
            .in_flight       = create_fence(vk),
        });
    }
}

static Graphics *create_graphics(App *app, Vulkan *vk) {
    Graphics *gfx = allocate<Graphics>(app->mem.fixed, 1);
    create_buffers(gfx, vk);
    allocate_regions(gfx, vk);
    create_render_passes(gfx, vk, app);
    create_shaders(gfx, vk);
    create_descriptor_sets(gfx, vk);
    create_pipelines(gfx, vk, app);

    init_swap_state(gfx, vk, app);
    init_sync(gfx, app, vk, 1);
    alloc_cmd_bufs(vk, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &gfx->temp_cmd_buf);
    gfx->meshes = create_array<Mesh>(app->mem.fixed, 64);

    return gfx;
}

static void push_mesh(Graphics *gfx, Vulkan *vk, Array<Vec3<f32>> *vertexes, Array<u32> *indexes) {
    Mesh *mesh = push(gfx->meshes, {
        .vertexes      = vertexes,
        .indexes       = indexes,
        .vertex_region = allocate_region(vk, gfx->buffer.device, byte_count(vertexes), 64),
        .index_region  = allocate_region(vk, gfx->buffer.device, byte_count(indexes),  64),
    });

    // write_to_host_region(vk, mesh->vertex_region, mesh->vertexes->data, byte_count(mesh->vertexes));
    // write_to_host_region(vk, mesh->index_region,  mesh->indexes->data,  byte_count(mesh->indexes));
    begin_temp_cmd_buf(gfx->temp_cmd_buf);
        write_to_device_region(vk, mesh->vertex_region, gfx->region.staging, gfx->temp_cmd_buf,
                               mesh->vertexes->data, byte_count(mesh->vertexes));
        write_to_device_region(vk, mesh->index_region, gfx->region.staging, gfx->temp_cmd_buf,
                               mesh->indexes->data, byte_count(mesh->indexes));
    submit_temp_cmd_buf(gfx->temp_cmd_buf, vk->queue.graphics);
}

static void create_test_data(App *app, Graphics *gfx, Vulkan *vk) {
    {
        auto vertexes = create_array<Vec3<f32>>(app->mem.fixed, 64);
        push(vertexes, { -0.4f,  0.4f, 0 });
        push(vertexes, {  0,    -0.4f, 0 });
        push(vertexes, {  0.4f,  0.4f, 0 });

        auto indexes = create_array<u32>(app->mem.fixed, 3);
        push(indexes, 0u);
        push(indexes, 1u);
        push(indexes, 2u);

        push_mesh(gfx, vk, vertexes, indexes);
    }
}

static void next_frame(Graphics *gfx, Vulkan *vk) {
    // Update current frame and wait until it is no longer in-flight.
    if (++gfx->sync.curr_frame_idx >= gfx->sync.frames->size)
        gfx->sync.curr_frame_idx = 0;

    gfx->sync.frame = gfx->sync.frames->data + gfx->sync.curr_frame_idx;
    validate_result(vkWaitForFences(vk->device, 1, &gfx->sync.frame->in_flight, VK_TRUE, U64_MAX),
                    "vkWaitForFences failed");
    validate_result(vkResetFences(vk->device, 1, &gfx->sync.frame->in_flight), "vkResetFences failed");

    // Once current frame is not in-flight, it is safe to use it's img_aquired semaphore and aquire next swap image.
    gfx->sync.swap_img_idx = next_swap_img_idx(vk, gfx->sync.frame->img_aquired, VK_NULL_HANDLE);
}

static void render(Graphics *gfx, Vulkan *vk) {
    VkCommandBuffer cmd_buf = gfx->swap_state.render_cmd_bufs->data[gfx->sync.swap_img_idx];
    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.flags = 0;
    cmd_buf_begin_info.pInheritanceInfo = NULL;
    validate_result(vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info), "failed to begin recording command buffer");

    VkRenderPassBeginInfo rp_begin_info = {};
    rp_begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin_info.renderPass      = gfx->main_render_pass->handle;
    rp_begin_info.framebuffer     = gfx->swap_state.framebufs->data[gfx->sync.swap_img_idx];
    rp_begin_info.clearValueCount = gfx->main_render_pass->attachment_clear_values->count;
    rp_begin_info.pClearValues    = gfx->main_render_pass->attachment_clear_values->data;
    rp_begin_info.renderArea = {
        .offset = { 0, 0 },
        .extent = vk->swapchain.extent,
    };
    vkCmdBeginRenderPass(cmd_buf, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.direct->handle);
    Mesh *mesh = gfx->meshes->data + 0;
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &mesh->vertex_region->buffer->handle, &mesh->vertex_region->offset);
    vkCmdBindIndexBuffer(cmd_buf, mesh->index_region->buffer->handle, mesh->index_region->offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd_buf, mesh->indexes->count, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmd_buf);

    vkEndCommandBuffer(cmd_buf);
}

static void present(Graphics *gfx, Vulkan *vk) {
    // Rendering
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = &gfx->sync.frame->img_aquired;
        submit_info.pWaitDstStageMask    = &wait_stage;
        submit_info.commandBufferCount   = 1;
        submit_info.pCommandBuffers      = &gfx->swap_state.render_cmd_bufs->data[gfx->sync.swap_img_idx];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = &gfx->sync.frame->render_finished;

        validate_result(vkQueueSubmit(vk->queue.graphics, 1, &submit_info, gfx->sync.frame->in_flight),
                        "vkQueueSubmit failed");
    }

    // Presentation
    {
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &gfx->sync.frame->render_finished;
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &vk->swapchain.handle;
        present_info.pImageIndices      = &gfx->sync.swap_img_idx;
        present_info.pResults           = NULL;

        validate_result(vkQueuePresentKHR(vk->queue.present, &present_info), "vkQueuePresentKHR failed");
    }
}

s32 main() {
    // Create App.
    Allocator *fixed_mem = create_stack_allocator(gigabyte());
    auto app = allocate<App>(fixed_mem, 1);
    app->mem.fixed    = fixed_mem;
    app->mem.temp     = create_stack_allocator(app->mem.fixed, megabyte());
    app->mem.platform = create_stack_allocator(app->mem.fixed, kilobyte(2));
    app->mem.vulkan   = create_stack_allocator(app->mem.fixed, megabyte(4));

    // Create Modules
    Platform *platform = create_platform(app->mem.platform, {
        .surface = {
            .x      = 600,
            .y      = 100,
            .width  = 1280,
            .height = 720,
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
        // Input
        process_events(platform->window);
        if (key_down(platform, InputKey::ESCAPE))
            break;

        // Rendering
        next_frame(gfx, vk);
        render(gfx, vk);
        present(gfx, vk);
    }

    return 0;
}
