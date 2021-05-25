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

struct Frame {
    VkSemaphore img_aquired;
    VkSemaphore render_finished;
    VkFence     in_flight;
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
        CTK_Array<VkFramebuffer> *framebufs;
        CTK_Array<VkCommandBuffer> *render_cmd_bufs;
    } swap_state;

    struct {
        u32 current_frame_idx;
        CTK_Array<Frame> *frames;
    } sync;
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

static void create_framebufs(Graphics *gfx, App *app, Vulkan *vk) {
    {
        // gfx->frame_state.framebufs = ctk_create_array<VkFramebuffer>(app->mem.fixed, vk->swapchain.image_count);

        // // Create framebuffer for each swapchain image.
        // for (u32 i = 0; i < vk->swapchain.image_views.count; ++i) {
        //     ctk_push_frame(app->mem.temp);

        //     FramebufferInfo info = {
        //         .attachments = ctk_create_array<VkImageView>(app->mem.temp, 1),
        //         .extent      = get_surface_extent(vk),
        //         .layers      = 1,
        //     };

        //     ctk_push(info.attachments, vk->swapchain.image_views[i]);
        //     ctk_push(gfx->swap_state.framebufs, create_framebuf(vk->device, gfx->main_render_pass->handle, &info));

        //     ctk_pop_frame(app->mem.temp);
        // }
    }
}

static void create_cmd_bufs(Graphics *gfx, App *app, Vulkan *vk) {
    // auto render_cmd_bufs = ctk_create_array_full<VkCommandBuffer>(app->mem.fixed, vk->swapchain.image_count);
    // alloc_cmd_bufs(vk, VK_COMMAND_BUFFER_LEVEL_PRIMARY, render_cmd_bufs->count, render_cmd_bufs->data);
    // gfx->frame_state.render_cmd_bufs = render_cmd_bufs;
}

static void create_frames(Graphics *gfx, App *app, Vulkan *vk, u32 frame_count) {
    gfx->sync.frames = ctk_create_array<Frame>(app->mem.fixed, frame_count);

    for (u32 i = 0; i < frame_count; ++i) {
        ctk_push(gfx->sync.frames, {
            .img_aquired     = create_semaphore(vk),
            .render_finished = create_semaphore(vk),
            .in_flight       = create_fence(vk),
        });
    }

    gfx->sync.current_frame_idx = CTK_U32_MAX;
}

static Graphics *create_graphics(App *app, Vulkan *vk) {
    Graphics *gfx = ctk_alloc<Graphics>(app->mem.fixed, 1);
    create_buffers(gfx, vk);
    allocate_regions(gfx, vk);
    create_render_passes(gfx, app, vk);
    create_shaders(gfx, vk);
    create_descriptor_sets(gfx, vk);
    create_pipelines(gfx, app, vk);

    create_framebufs(gfx, app, vk);
    create_cmd_bufs(gfx, app, vk);
    create_frames(gfx, app, vk, 1);

    return gfx;
}

static void create_test_data(App *app, Graphics *gfx, Vulkan *vk) {
    app->vertexes = ctk_create_array<CTK_Vec3<f32>>(app->mem.fixed, 64);
    ctk_push(app->vertexes, { -0.4f, -0.4f, 0 });
    ctk_push(app->vertexes, {  0,     0.4f, 0 });
    ctk_push(app->vertexes, {  0.4f, -0.4f, 0 });

    write_to_host_region(vk, gfx->region.mesh, app->vertexes->data, app->vertexes->count);
}

static Frame *get_next_frame(Graphics *gfx) {
    if (++gfx->sync.current_frame_idx >= gfx->sync.frames->size)
        gfx->sync.current_frame_idx = 0;

    return gfx->sync.frames->data + gfx->sync.current_frame_idx;
}

static void render(Graphics *gfx, Vulkan *vk) {
    Frame *frame = get_next_frame(gfx);
    u32 swapchain_img_idx = get_next_swapchain_img_idx(vk, frame->img_aquired, VK_NULL_HANDLE);

    // Rendering
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount   = 1;
        submit_info.pWaitSemaphores      = &frame->img_aquired;
        submit_info.pWaitDstStageMask    = &wait_stage;
        submit_info.commandBufferCount   = 0;
        submit_info.pCommandBuffers      = NULL;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = &frame->render_finished;

        validate_result(vkQueueSubmit(vk->queue.graphics, 1, &submit_info, frame->in_flight), "vkQueueSubmit failed");
        validate_result(vkWaitForFences(vk->device, 1, &frame->in_flight, VK_TRUE, CTK_U64_MAX), "vkWaitForFences failed");
        validate_result(vkResetFences(vk->device, 1, &frame->in_flight), "vkResetFences failed");
    }

    // Presentation
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = &frame->render_finished;
        present_info.swapchainCount     = 1;
        present_info.pSwapchains        = &vk->swapchain.handle;
        present_info.pImageIndices      = &swapchain_img_idx;
        present_info.pResults           = NULL;

        validate_result(vkQueuePresentKHR(vk->queue.present, &present_info), "vkQueuePresentKHR failed");
    }
}

static void hack(App *app, Vulkan *vk) {
    ctk_push_frame(app->mem.temp);

    auto swapchain_images = load_vk_objects<VkImage>(app->mem.temp, vkGetSwapchainImagesKHR, vk->device,
                                                     vk->swapchain.handle);

    // for (u32 i = 0; i < swapchain_images->count; ++i) {
    //     vtk_begin_one_time_command_buffer(app->cmd_bufs.one_time);
    //         VkImageMemoryBarrier pre_mem_barrier = {};
    //         pre_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //         pre_mem_barrier.srcAccessMask = 0;
    //         pre_mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //         pre_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //         pre_mem_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //         pre_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //         pre_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //         pre_mem_barrier.image = tex.handle;
    //         pre_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //         pre_mem_barrier.subresourceRange.baseMipLevel = 0;
    //         pre_mem_barrier.subresourceRange.levelCount = 1;
    //         pre_mem_barrier.subresourceRange.baseArrayLayer = 0;
    //         pre_mem_barrier.subresourceRange.layerCount = 1;
    //         vkCmdPipelineBarrier(app->cmd_bufs.one_time,
    //                              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //                              VK_PIPELINE_STAGE_TRANSFER_BIT,
    //                              0, // Dependency Flags
    //                              0, NULL, // Memory Barriers
    //                              0, NULL, // Buffer Memory Barriers
    //                              1, &pre_mem_barrier); // Image Memory Barriers

    //         VkBufferImageCopy copy = {};
    //         copy.bufferOffset = 0;
    //         copy.bufferRowLength = 0;
    //         copy.bufferImageHeight = 0;
    //         copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //         copy.imageSubresource.mipLevel = 0;
    //         copy.imageSubresource.baseArrayLayer = 0;
    //         copy.imageSubresource.layerCount = 1;
    //         copy.imageOffset.x = 0;
    //         copy.imageOffset.y = 0;
    //         copy.imageOffset.z = 0;
    //         copy.imageExtent.width = width;
    //         copy.imageExtent.height = height;
    //         copy.imageExtent.depth = 1;
    //         vkCmdCopyBufferToImage(app->cmd_bufs.one_time, vk->staging_region.buffer->handle, tex.handle,
    //                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    //         VkImageMemoryBarrier post_mem_barrier = {};
    //         post_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //         post_mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //         post_mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //         post_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //         post_mem_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //         post_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //         post_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //         post_mem_barrier.image = tex.handle;
    //         post_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //         post_mem_barrier.subresourceRange.baseMipLevel = 0;
    //         post_mem_barrier.subresourceRange.levelCount = 1;
    //         post_mem_barrier.subresourceRange.baseArrayLayer = 0;
    //         post_mem_barrier.subresourceRange.layerCount = 1;
    //         vkCmdPipelineBarrier(app->cmd_bufs.one_time,
    //                              VK_PIPELINE_STAGE_TRANSFER_BIT,
    //                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    //                              0, // Dependency Flags
    //                              0, NULL, // Memory Barriers
    //                              0, NULL, // Buffer Memory Barriers
    //                              1, &post_mem_barrier); // Image Memory Barriers
    //     vtk_submit_one_time_command_buffer(app->cmd_bufs.one_time, vk->device.queues.graphics);
    // }

    ctk_pop_frame(app->mem.temp);
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

    // render(gfx, vk);
    // render(gfx, vk);

    // Main Loop
    while (platform->window->open) {
        // Input
        process_events(platform->window);
        if (key_down(platform, INPUT_KEY_ESCAPE))
            break;

        // Rendering
        // render(gfx, vk);
    }

    return 0;
}
