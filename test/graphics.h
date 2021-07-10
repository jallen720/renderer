#pragma once

#include "renderer/vulkan.h"
#include "ctk/memory.h"
#include "ctk/containers.h"

using namespace ctk;

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
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

    VkCommandPool main_cmd_pool;
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
        // VkDescriptorSetLayout mvp_matrix;
        VkDescriptorSetLayout image_sampler;
    } descriptor_set_layout;

    struct {
        // Array<VkDescriptorSet> *mvp_matrix;
        Array<VkDescriptorSet> *image_sampler;
    } descriptor_set;

    struct {
        ShaderGroup test;
    } shader;

    RenderPass *main_render_pass;

    struct {
        Image *depth;
    } framebuffer_image;

    struct {
        Pipeline *test;
    } pipeline;

    Array<VkFramebuffer> *framebuffers;

    Array<VkCommandBuffer> *render_pass_cmd_bufs;
    Array<VkCommandPool> *render_cmd_pools;
    Array<Array<VkCommandBuffer> *> *render_cmd_bufs;

    struct {
        Array<Frame> *frames;
        Frame *frame;
        u32 swap_img_idx;
        u32 curr_frame_idx;
    } sync;
};

////////////////////////////////////////////////////////////
/// Internal
////////////////////////////////////////////////////////////
static void create_cmd_state(Graphics *gfx, Vulkan *vk) {
    gfx->main_cmd_pool = create_cmd_pool(vk);

    allocate_cmd_bufs(vk, &gfx->temp_cmd_buf, {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = gfx->main_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    });
}

static void create_buffers(Graphics *gfx, Vulkan *vk) {
    {
        BufferInfo info = {};
        info.size = megabyte(512);
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
        info.size = megabyte(512);
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
            .uniform_buffer = 8,
            .uniform_buffer_dynamic = 4,
            .combined_image_sampler = 8,
            // .input_attachment = 4,
        },
        .max_descriptor_sets = 64,
    });

    // // MVP Matrix
    // {
    //     DescriptorInfo descriptor_info = {
    //         .count = 1,
    //         .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
    //         .stage = VK_SHADER_STAGE_VERTEX_BIT,
    //     };

    //     gfx->descriptor_set_layout.mvp_matrix = create_descriptor_set_layout(vk, &descriptor_info, 1);
    //     gfx->descriptor_set.mvp_matrix = create_array<VkDescriptorSet>(gfx->mem.module, vk->swapchain.image_count);
    //     allocate_descriptor_sets(vk, gfx->descriptor_pool, gfx->descriptor_set_layout.mvp_matrix,
    //                              vk->swapchain.image_count, gfx->descriptor_set.mvp_matrix->data);
    // }

    // Image Sampler
    {
        DescriptorInfo descriptor_info = {
            .count = 1,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        gfx->descriptor_set_layout.image_sampler = create_descriptor_set_layout(vk, &descriptor_info, 1);
        gfx->descriptor_set.image_sampler = create_array<VkDescriptorSet>(gfx->mem.module, vk->swapchain.image_count);
        allocate_descriptor_sets(vk, gfx->descriptor_pool, gfx->descriptor_set_layout.image_sampler,
                                 vk->swapchain.image_count, gfx->descriptor_set.image_sampler->data);
    }
}

static void create_shaders(Graphics *gfx, Vulkan *vk) {
    gfx->shader = {
        .test = {
            .vert = create_shader(vk, "data/shaders/test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            .frag = create_shader(vk, "data/shaders/test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
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
                .descriptions = create_array<VkAttachmentDescription>(gfx->mem.temp, 2),
                .clear_values = create_array<VkClearValue>(gfx->mem.temp, 2),
            },
            .subpass = {
                .infos = create_array<SubpassInfo>(gfx->mem.temp, 1),
                .dependencies = create_array<VkSubpassDependency>(gfx->mem.temp, 1),
            },
        };

        // Swapchain Image Attachment
        u32 depth_attachment_index = push_attachment(&info, {
            .description = {
                .flags = 0,
                .format = vk->physical_device.depth_image_format,
                .samples = VK_SAMPLE_COUNT_1_BIT,

                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
            .clear_value = { 1.0f, 0 },
        });
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
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
            .clear_value = { 0, 0, 0, 1 },
        });

        // Subpasses
        SubpassInfo *subpass_info = push(info.subpass.infos);
        subpass_info->color_attachment_refs = create_array<VkAttachmentReference>(gfx->mem.temp, 1);
        subpass_info->depth_attachment_ref = {
            .attachment = depth_attachment_index,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        push(subpass_info->color_attachment_refs, {
            .attachment = swapchain_attachment_index,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });

        // push(info.subpass.dependencies, {
        //     .srcSubpass = 0,
        //     .dstSubpass = VK_SUBPASS_EXTERNAL,
        //     .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        //     .dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        //     .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        //                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        //     .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        //     .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        // });

        gfx->main_render_pass = create_render_pass(vk, &info);

        pop_frame(gfx->mem.temp);
    }
}

static void create_framebuffer_images(Graphics *gfx, Vulkan *vk) {
    VkFormat depth_fmt = vk->physical_device.depth_image_format;

    gfx->framebuffer_image.depth = create_image(vk, {
        .image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = depth_fmt,
            .extent = {
                .width = vk->swapchain.extent.width,
                .height = vk->swapchain.extent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
            .pQueueFamilyIndices = NULL, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
        .view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .flags = 0,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depth_fmt,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        },
        .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    });
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
        .minDepth = 0,
        .maxDepth = 1
    };

    VkRect2D default_scissor = {
        .offset = { 0, 0 },
        .extent = surface_extent
    };

    // Test
    {
        push_frame(gfx->mem.temp);

        PipelineInfo info = DEFAULT_PIPELINE_INFO;
        info.descriptor_set_layouts = create_array<VkDescriptorSetLayout>(gfx->mem.temp, 2);
        info.push_constant_ranges = create_array<VkPushConstantRange>(gfx->mem.temp, 1);
        info.vertex_bindings = create_array<VkVertexInputBindingDescription>(gfx->mem.temp, 1);
        info.vertex_attributes = create_array<VkVertexInputAttributeDescription>(gfx->mem.temp, 2);
        info.viewports = create_array<VkViewport>(gfx->mem.temp, 1);
        info.scissors = create_array<VkRect2D>(gfx->mem.temp, 1);

        push(&info.shaders, gfx->shader.test.vert);
        push(&info.shaders, gfx->shader.test.frag);
        push(&info.color_blend_attachments, DEFAULT_COLOR_BLEND_ATTACHMENT);

        push(info.descriptor_set_layouts, gfx->descriptor_set_layout.image_sampler);
        // push(info.descriptor_set_layouts, gfx->descriptor_set_layout.mvp_matrix);
        push(info.push_constant_ranges, {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = 64
        });
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

        // Enable depth testing.
        info.depth_stencil.depthTestEnable = VK_TRUE;
        info.depth_stencil.depthWriteEnable = VK_TRUE;
        info.depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        gfx->pipeline.test = create_pipeline(vk, gfx->main_render_pass, 0, &info);

        pop_frame(gfx->mem.temp);
    }
}

static void create_framebuffers(Graphics *gfx, Vulkan *vk) {
    gfx->framebuffers = create_array<VkFramebuffer>(gfx->mem.module, vk->swapchain.image_count);

    // Create framebuffer for each swapchain image.
    for (u32 i = 0; i < vk->swapchain.image_views.count; ++i) {
        push_frame(gfx->mem.temp);

        FramebufferInfo info = {};

        info.attachments = create_array<VkImageView>(gfx->mem.temp, 2),
        push(info.attachments, gfx->framebuffer_image.depth->view);
        push(info.attachments, vk->swapchain.image_views[i]);

        info.extent = get_surface_extent(vk);
        info.layers = 1;

        push(gfx->framebuffers, create_framebuffer(vk->device, gfx->main_render_pass->handle, &info));

        pop_frame(gfx->mem.temp);
    }
}

static void create_render_cmd_state(Graphics *gfx, Vulkan *vk, u32 render_thread_count) {
    gfx->render_pass_cmd_bufs = create_cmd_buf_array(vk, gfx->mem.module, {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = gfx->main_cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = vk->swapchain.image_count,
    });

    gfx->render_cmd_pools = create_array_full<VkCommandPool>(gfx->mem.module, render_thread_count);
    gfx->render_cmd_bufs = create_array_full<Array<VkCommandBuffer> *>(gfx->mem.module, vk->swapchain.image_count);

    // Create command buffer arrays for rendering to each swapchain image.
    for (u32 swap_img_idx = 0; swap_img_idx < vk->swapchain.image_count; ++swap_img_idx) {
        gfx->render_cmd_bufs->data[swap_img_idx] =
            create_array_full<VkCommandBuffer>(gfx->mem.module, render_thread_count);
    }

    // Allocate command pools for each thread and command buffers for each thread and swapchain image.
    for (u32 thread_idx = 0; thread_idx < render_thread_count; ++thread_idx) {
        push_frame(gfx->mem.temp);

        gfx->render_cmd_pools->data[thread_idx] = create_cmd_pool(vk);

        // Allocate command buffers for this thread for each swapchain image.
        Array<VkCommandBuffer> *thread_cmd_bufs = create_cmd_buf_array(vk, gfx->mem.temp, {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = gfx->render_cmd_pools->data[thread_idx],
            .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            .commandBufferCount = vk->swapchain.image_count,
        });

        for (u32 swap_img_idx = 0; swap_img_idx < vk->swapchain.image_count; ++swap_img_idx)
            gfx->render_cmd_bufs->data[swap_img_idx]->data[thread_idx] = thread_cmd_bufs->data[swap_img_idx];

        pop_frame(gfx->mem.temp);
    }
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

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static Graphics *create_graphics(Allocator *module_mem, Vulkan *vk, u32 render_thread_count) {
    Graphics *gfx = allocate<Graphics>(module_mem, 1);
    gfx->mem.module = module_mem;
    gfx->mem.temp = create_stack_allocator(module_mem, megabyte(1));

    create_cmd_state(gfx, vk);
    create_buffers(gfx, vk);
    CTK_TODO("what should alignment be?")
    gfx->staging_region = allocate_region(vk, gfx->buffer.host, megabyte(256), 16);
    create_samplers(gfx, vk);
    create_descriptor_sets(gfx, vk);
    create_shaders(gfx, vk);
    create_render_passes(gfx, vk);
    create_framebuffer_images(gfx, vk);
    create_pipelines(gfx, vk);
    create_framebuffers(gfx, vk);
    create_render_cmd_state(gfx, vk, render_thread_count);
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

static void submit_render_cmds(Graphics *gfx, Vulkan *vk) {
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
        submit_info.pCommandBuffers = &gfx->render_pass_cmd_bufs->data[gfx->sync.swap_img_idx];
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
