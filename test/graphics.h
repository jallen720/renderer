#pragma once

#include "renderer/vulkan.h"
#include "ctk/memory.h"
#include "ctk/containers.h"

using namespace ctk;

struct ShaderGroup {
    Shader *vert;
    Shader *frag;
};

struct Frame {
    VkSemaphore img_aquired;
    VkSemaphore render_finished;
    VkFence in_flight;
};

struct Graphics {
    struct {
        Allocator *module;
        Allocator *temp;
    } mem;

    VkCommandBuffer temp_cmd_buf;

    struct {
        Buffer *host;
        Buffer *device;
    } buffer;

    Region *staging_region;

    struct {
        VkSampler test;
    } sampler;

    VkDescriptorPool descriptor_pool;

    struct {
        VkDescriptorSetLayout color;
        VkDescriptorSetLayout sampler;
    } descriptor_set_layout;

    struct {
        Array<VkDescriptorSet> *color;
        VkDescriptorSet sampler;
    } descriptor_set;

    struct {
        ShaderGroup color;
        ShaderGroup sampler;
    } shader;

    RenderPass *main_render_pass;

    struct {
        Pipeline *color;
        Pipeline *sampler;
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

static void create_samplers(Graphics *gfx, Vulkan *vk) {
    gfx->sampler.test = create_sampler(vk->device, {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 16,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE,
    });
}

static void create_descriptor_sets(Graphics *gfx, Vulkan *vk) {
    // Pool
    gfx->descriptor_pool = create_descriptor_pool(vk, {
        .descriptor_count = {
            .uniform_buffer = 4,
            // .uniform_buffer_dynamic = 4,
            .combined_image_sampler = 4,
            // .input_attachment = 4,
        },
        .max_descriptor_sets = 64,
    });

    // Color
    {
        DescriptorInfo descriptor_infos[] = {
            { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT }
        };

        gfx->descriptor_set_layout.color = create_descriptor_set_layout(vk, descriptor_infos,
                                                                        CTK_ARRAY_SIZE(descriptor_infos));

        gfx->descriptor_set.color = create_array<VkDescriptorSet>(gfx->mem.module, vk->swapchain.image_count);
        allocate_descriptor_sets(vk, gfx->descriptor_pool, gfx->descriptor_set_layout.color,
                                 vk->swapchain.image_count, gfx->descriptor_set.color->data);
    }

    // Sampler
    {
        DescriptorInfo descriptor_infos[] = {
            { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
        };

        gfx->descriptor_set_layout.sampler = create_descriptor_set_layout(vk, descriptor_infos,
                                                                          CTK_ARRAY_SIZE(descriptor_infos));

        allocate_descriptor_sets(vk, gfx->descriptor_pool, gfx->descriptor_set_layout.sampler,
                                 1, &gfx->descriptor_set.sampler);
    }
}

static void create_shaders(Graphics *gfx, Vulkan *vk) {
    gfx->shader = {
        .color = {
            .vert = create_shader(vk, "data/shaders/color.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            .frag = create_shader(vk, "data/shaders/color.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .sampler = {
            .vert = create_shader(vk, "data/shaders/sampler.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            .frag = create_shader(vk, "data/shaders/sampler.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
        },
    };
}

static u32 push_attachment(RenderPassInfo *info, AttachmentInfo attachment_info) {
    if (info->attachment.descriptions->count == info->attachment.descriptions->size)
        CTK_FATAL("cannot push any more attachments to RenderPassInfo");

    u32 attachment_index = info->attachment.descriptions->count;

    push(info->attachment.descriptions, attachment_info.description);
    push(info->attachment.clear_values, attachment_info.clear_value);

    return attachment_index;
}

static void create_render_passes(Graphics *gfx, Vulkan *vk) {
    {
        push_frame(gfx->mem.temp);

        RenderPassInfo info = {
            .attachment = {
                .descriptions = create_array<VkAttachmentDescription>(gfx->mem.temp, 1),
                .clear_values = create_array<VkClearValue>(gfx->mem.temp, 1),
            },
            .subpass = {
                .infos = create_array<SubpassInfo>(gfx->mem.temp, 1),
                .dependencies = create_array<VkSubpassDependency>(gfx->mem.temp, 1),
            },
        };

        // Swapchain Image Attachment
        u32 swapchain_attachment_index = push_attachment(&info, {
            .description = {
                .flags = 0,
                .format = vk->swapchain.image_format,
                .samples = VK_SAMPLE_COUNT_1_BIT,

                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            .clear_value = { 1, 0, 0, 1 },
        });

        // Subpasses
        SubpassInfo *subpass_info = push(info.subpass.infos);
        subpass_info->color_attachment_refs = create_array<VkAttachmentReference>(gfx->mem.temp, 1);
        push(subpass_info->color_attachment_refs, {
            .attachment = swapchain_attachment_index,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });

        gfx->main_render_pass = create_render_pass(vk, &info);

        pop_frame(gfx->mem.temp);
    }
}

static void create_pipelines(Graphics *gfx, Vulkan *vk) {
    VkVertexInputBindingDescription default_vertex_binding = {
        .binding = 0,
        .stride = 20,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkExtent2D surface_extent = get_surface_extent(vk);

    VkViewport default_viewport = {
        .x = 0,
        .y = 0,
        .width = (f32)surface_extent.width,
        .height = (f32)surface_extent.height,
        .minDepth = 1,
        .maxDepth = 0
    };

    VkRect2D default_scissor = {
        .offset = { 0, 0 },
        .extent = surface_extent
    };

    // Color
    {
        push_frame(gfx->mem.temp);

        PipelineInfo info = DEFAULT_PIPELINE_INFO;
        info.descriptor_set_layouts = create_array<VkDescriptorSetLayout>(gfx->mem.temp, 1);
        info.vertex_bindings = create_array<VkVertexInputBindingDescription>(gfx->mem.temp, 1);
        info.vertex_attributes = create_array<VkVertexInputAttributeDescription>(gfx->mem.temp, 2);
        info.viewports = create_array<VkViewport>(gfx->mem.temp, 1);
        info.scissors = create_array<VkRect2D>(gfx->mem.temp, 1);

        push(&info.shaders, gfx->shader.color.vert);
        push(&info.shaders, gfx->shader.color.frag);
        push(&info.color_blend_attachments, DEFAULT_COLOR_BLEND_ATTACHMENT);

        push(info.descriptor_set_layouts, gfx->descriptor_set_layout.color);
        push(info.vertex_bindings, default_vertex_binding);
        push(info.vertex_attributes, {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        });
        push(info.viewports, default_viewport);
        push(info.scissors, default_scissor);

        gfx->pipeline.color = create_pipeline(vk, gfx->main_render_pass, 0, &info);

        pop_frame(gfx->mem.temp);
    }

    // Sampler
    {
        push_frame(gfx->mem.temp);

        PipelineInfo info = DEFAULT_PIPELINE_INFO;
        info.descriptor_set_layouts = create_array<VkDescriptorSetLayout>(gfx->mem.temp, 1);
        info.vertex_bindings = create_array<VkVertexInputBindingDescription>(gfx->mem.temp, 1);
        info.vertex_attributes = create_array<VkVertexInputAttributeDescription>(gfx->mem.temp, 2);
        info.viewports = create_array<VkViewport>(gfx->mem.temp, 1);
        info.scissors = create_array<VkRect2D>(gfx->mem.temp, 1);

        push(&info.shaders, gfx->shader.sampler.vert);
        push(&info.shaders, gfx->shader.sampler.frag);
        push(&info.color_blend_attachments, DEFAULT_COLOR_BLEND_ATTACHMENT);

        push(info.descriptor_set_layouts, gfx->descriptor_set_layout.sampler);
        push(info.vertex_bindings, default_vertex_binding);
        push(info.vertex_attributes, {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        });
        push(info.vertex_attributes, {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 12,
        });
        push(info.viewports, default_viewport);
        push(info.scissors, default_scissor);

        gfx->pipeline.sampler = create_pipeline(vk, gfx->main_render_pass, 0, &info);

        pop_frame(gfx->mem.temp);
    }
}

static void init_swap_state(Graphics *gfx, Vulkan *vk) {
    {
        gfx->swap_state.framebufs = create_array<VkFramebuffer>(gfx->mem.module, vk->swapchain.image_count);

        // Create framebuffer for each swapchain image.
        for (u32 i = 0; i < vk->swapchain.image_views.count; ++i) {
            push_frame(gfx->mem.temp);

            FramebufferInfo info = {
                .attachments = create_array<VkImageView>(gfx->mem.temp, 1),
                .extent = get_surface_extent(vk),
                .layers = 1,
            };

            push(info.attachments, vk->swapchain.image_views[i]);
            push(gfx->swap_state.framebufs, create_framebuffer(vk->device, gfx->main_render_pass->handle, &info));

            pop_frame(gfx->mem.temp);
        }
    }

    gfx->swap_state.render_cmd_bufs = alloc_cmd_bufs(vk, VK_COMMAND_BUFFER_LEVEL_PRIMARY, vk->swapchain.image_count);
}

static void init_sync(Graphics *gfx, Vulkan *vk, u32 frame_count) {
    gfx->sync.curr_frame_idx = U32_MAX;
    gfx->sync.frames = create_array<Frame>(gfx->mem.module, frame_count);

    for (u32 i = 0; i < frame_count; ++i) {
        push(gfx->sync.frames, {
            .img_aquired = create_semaphore(vk),
            .render_finished = create_semaphore(vk),
            .in_flight = create_fence(vk),
        });
    }
}

static Graphics *create_graphics(Allocator *module_mem, Vulkan *vk) {
    Graphics *gfx = allocate<Graphics>(module_mem, 1);
    gfx->mem.module = module_mem;
    gfx->mem.temp = create_stack_allocator(module_mem, megabyte(1));

    alloc_cmd_bufs(vk, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &gfx->temp_cmd_buf);
    create_buffers(gfx, vk);
    gfx->staging_region = allocate_region(vk, gfx->buffer.host, megabyte(64), 16);
    create_samplers(gfx, vk);
    create_descriptor_sets(gfx, vk);
    create_shaders(gfx, vk);
    create_render_passes(gfx, vk);
    create_pipelines(gfx, vk);

    init_swap_state(gfx, vk);
    init_sync(gfx, vk, 1);

    return gfx;
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

static void present(Graphics *gfx, Vulkan *vk) {
    // Rendering
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = NULL;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &gfx->sync.frame->img_aquired;
        submit_info.pWaitDstStageMask = &wait_stage;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &gfx->swap_state.render_cmd_bufs->data[gfx->sync.swap_img_idx];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &gfx->sync.frame->render_finished;

        validate_result(vkQueueSubmit(vk->queue.graphics, 1, &submit_info, gfx->sync.frame->in_flight),
                        "vkQueueSubmit failed");
    }

    // Presentation
    {
        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pNext = NULL;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &gfx->sync.frame->render_finished;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &vk->swapchain.handle;
        present_info.pImageIndices = &gfx->sync.swap_img_idx;
        present_info.pResults = NULL;

        validate_result(vkQueuePresentKHR(vk->queue.present, &present_info), "vkQueuePresentKHR failed");
    }
}
