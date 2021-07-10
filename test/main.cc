#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb/stb_image.h>

#include <thread>
#include <vector>
#include "renderer/platform.h"
#include "renderer/vulkan.h"
#include "renderer/test/graphics.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "ctk/math.h"
#include "ctk/benchmark.h"
#include "ctk/task.h"

using namespace ctk;

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
struct Memory {
    Allocator *fixed;
    Allocator *temp;
    Allocator *platform;
    Allocator *vulkan;
    Allocator *graphics;
};

struct Vertex {
    Vec3<f32> position;
    Vec2<f32> uv;
};

struct Mesh {
    Array<Vertex> *vertexes;
    Array<u32> *indexes;
    Region *vertex_region;
    Region *index_region;
};

struct View {
    PerspectiveInfo perspective_info;
    Vec3<f32> position;
    Vec3<f32> rotation;
    f32 max_x_angle;
};

struct Entity {
    Vec3<f32> position;
    Vec3<f32> rotation;
};

struct Test {
    static constexpr s32 CUBE_MATRIX_SIZE = 64;
    static constexpr f32 CUBE_MATRIX_SPREAD = 2.5f;
    static constexpr u32 MAX_ENTITIES = CUBE_MATRIX_SIZE * CUBE_MATRIX_SIZE * CUBE_MATRIX_SIZE;

    Memory *mem;

    struct {
        Mesh quad;
    } mesh;

    struct {
        Image *test;
    } image;

    struct {
        Array<Region *> *mvp_matrixes;
    } uniform_buffer;

    struct {
        ImageSampler test;
    } image_sampler;

    View view;

    struct {
        Vec2<s32> last_mouse_position;
        Vec2<s32> mouse_delta;
    } input;

    FixedArray<Entity, MAX_ENTITIES> entities;
    FixedArray<Matrix, MAX_ENTITIES> mvp_matrixes;

    FrameBenchmark *frame_benchmark;
};

static bool use_threads;

////////////////////////////////////////////////////////////
/// Utils
////////////////////////////////////////////////////////////
static void init_mesh(Mesh *mesh, Graphics *gfx, Vulkan *vk, Array<Vertex> *vertexes, Array<u32> *indexes) {
    mesh->vertexes = vertexes,
    mesh->indexes = indexes,
    mesh->vertex_region = allocate_region(vk, gfx->buffer.device, byte_count(vertexes), 16),
    mesh->index_region = allocate_region(vk, gfx->buffer.device, byte_count(indexes), 16),

    begin_temp_cmd_buf(gfx->temp_cmd_buf);
        u32 vertex_data_size = byte_count(mesh->vertexes);
        write_to_device_region(vk, gfx->temp_cmd_buf,
                               gfx->staging_region, 0,
                               mesh->vertex_region, 0,
                               mesh->vertexes->data, vertex_data_size);
        write_to_device_region(vk, gfx->temp_cmd_buf,
                               gfx->staging_region, vertex_data_size,
                               mesh->index_region,  0,
                               mesh->indexes->data, byte_count(mesh->indexes));
    submit_temp_cmd_buf(gfx->temp_cmd_buf, vk->queue.graphics);
}

static void create_meshes(Test *test, Graphics *gfx, Vulkan *vk) {
    {
        auto vertexes = create_array<Vertex>(test->mem->fixed, 8);
        push(vertexes, { { -1,-1,-1 }, { 0, 0 } });
        push(vertexes, { { -1, 1,-1 }, { 0, 1 } });
        push(vertexes, { {  1, 1,-1 }, { 1, 1 } });
        push(vertexes, { {  1,-1,-1 }, { 1, 0 } });

        push(vertexes, { {  1,-1, 1 }, { 0, 0 } });
        push(vertexes, { {  1, 1, 1 }, { 0, 1 } });
        push(vertexes, { { -1, 1, 1 }, { 1, 1 } });
        push(vertexes, { { -1,-1, 1 }, { 1, 0 } });

        auto indexes = create_array<u32>(test->mem->fixed, 36);
        push(indexes, 0u);
        push(indexes, 2u);
        push(indexes, 1u);
        push(indexes, 0u);
        push(indexes, 3u);
        push(indexes, 2u);

        push(indexes, 3u);
        push(indexes, 5u);
        push(indexes, 2u);
        push(indexes, 3u);
        push(indexes, 4u);
        push(indexes, 5u);

        push(indexes, 4u);
        push(indexes, 6u);
        push(indexes, 5u);
        push(indexes, 4u);
        push(indexes, 7u);
        push(indexes, 6u);

        push(indexes, 7u);
        push(indexes, 1u);
        push(indexes, 6u);
        push(indexes, 7u);
        push(indexes, 0u);
        push(indexes, 1u);

        push(indexes, 7u);
        push(indexes, 3u);
        push(indexes, 0u);
        push(indexes, 7u);
        push(indexes, 4u);
        push(indexes, 3u);

        push(indexes, 1u);
        push(indexes, 5u);
        push(indexes, 6u);
        push(indexes, 1u);
        push(indexes, 2u);
        push(indexes, 5u);

        init_mesh(&test->mesh.quad, gfx, vk, vertexes, indexes);
    }
}

static Image *load_image(Graphics *gfx, Vulkan *vk, cstr path, ImageInfo info) {
    // Load Image Data to Staging Region
    s32 width = 0;
    s32 height = 0;
    s32 channel_count = 0;
    stbi_uc *data = stbi_load(path, &width, &height, &channel_count, STBI_rgb_alpha);
    if (data == NULL)
        CTK_FATAL("failed to load image from \"%s\"", path)

    write_to_host_region(vk->device, gfx->staging_region, 0, data, width * height * STBI_rgb_alpha);
    stbi_image_free(data);

    info.image.extent.width = (u32)width;
    info.image.extent.height = (u32)height;

    // Create Vulkan Image
    Image *image = create_image(vk, info);

    // Write Image Data to Vulkan Image
    begin_temp_cmd_buf(gfx->temp_cmd_buf);
        write_to_image(vk, gfx->temp_cmd_buf, gfx->staging_region, 0, image);
    submit_temp_cmd_buf(gfx->temp_cmd_buf, vk->queue.graphics);

    return image;
}

static void create_images(Test *test, Graphics *gfx, Vulkan *vk) {
    VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

    {
        test->image.test = load_image(gfx, vk, "data/test.png", {
            .image = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = color_format,
                .extent = { .depth = 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
                .pQueueFamilyIndices = NULL, // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            },
            .view = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .flags = 0,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = color_format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            },
            .mem_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        });
    }
}

static void create_uniform_buffers(Test *test, Graphics *gfx, Vulkan *vk) {
    test->uniform_buffer.mvp_matrixes = create_array<Region *>(test->mem->fixed, vk->swapchain.image_count);
    for (u32 i = 0; i < vk->swapchain.image_count; ++i) {
        u32 element_size = multiple_of(sizeof(Matrix), vk->physical_device.min_uniform_buffer_offset_alignment);
        test->uniform_buffer.mvp_matrixes->data[i] =
            allocate_uniform_buffer_region(vk, gfx->buffer.device, element_size * Test::MAX_ENTITIES);
    }
}

static void create_image_samplers(Test *test, Graphics *gfx) {
    test->image_sampler.test = { test->image.test, gfx->sampler.test };
}

static void bind_descriptor_data(Test *test, Graphics *gfx, Vulkan *vk) {
    for (u32 i = 0; i < vk->swapchain.image_count; ++i) {
        DescriptorBinding binding = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .image_sampler = &test->image_sampler.test,
        };

        update_descriptor_set(vk, gfx->descriptor_set.image_sampler->data[i], 1, &binding);
    }

    for (u32 i = 0; i < vk->swapchain.image_count; ++i) {
        DescriptorBinding binding = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .uniform_buffer = test->uniform_buffer.mvp_matrixes->data[i],
        };

        update_descriptor_set(vk, gfx->descriptor_set.mvp_matrix->data[i], 1, &binding);
    }
}

static void create_entities(Test *test) {
    for (s32 z = 0; z < Test::CUBE_MATRIX_SIZE; ++z)
    for (s32 y = 0; y < Test::CUBE_MATRIX_SIZE; ++y)
    for (s32 x = 0; x < Test::CUBE_MATRIX_SIZE; ++x) {
        push(&test->entities, {
            .position = { x * Test::CUBE_MATRIX_SPREAD, -y * Test::CUBE_MATRIX_SPREAD, z * Test::CUBE_MATRIX_SPREAD },
            .rotation = { 0, 0, 0 },
        });
    }
}

static Test *create_test(Memory *mem, Graphics *gfx, Vulkan *vk, Platform *platform) {
    auto test = allocate<Test>(mem->fixed, 1);
    test->mem = mem;
    create_meshes(test, gfx, vk);
    create_images(test, gfx, vk);
    create_uniform_buffers(test, gfx, vk);
    create_image_samplers(test, gfx);
    bind_descriptor_data(test, gfx, vk);

    test->view = {
        .perspective_info = {
            .vertical_fov = 90,
            .aspect = (f32)vk->swapchain.extent.width / vk->swapchain.extent.height,
            .z_near = 0.1f,
            .z_far = 1000,
        },
        .position = {
            .x = -Test::CUBE_MATRIX_SIZE * Test::CUBE_MATRIX_SPREAD * 0.125f,
            .y = -Test::CUBE_MATRIX_SIZE * Test::CUBE_MATRIX_SPREAD * 1.125f,
            .z = -Test::CUBE_MATRIX_SIZE * Test::CUBE_MATRIX_SPREAD * 0.125f
        },
        .rotation = { 45, -45, 0 },
        .max_x_angle = 89,
    };

    test->input.last_mouse_position = get_mouse_position(platform);
    create_entities(test);
    test->frame_benchmark = create_frame_benchmark(test->mem->fixed, 64);

    return test;
}

static bool wrap_mouse_position(Vec2<s32> *mouse_position, u32 max_width, u32 max_height) {
    bool wrapped = false;

    if (mouse_position->x < 0) {
        mouse_position->x += max_width;
        wrapped = true;
    }
    else if (mouse_position->x >= max_width) {
        mouse_position->x -= max_width;
        wrapped = true;
    }

    if (mouse_position->y < 0) {
        mouse_position->y += max_height;
        wrapped = true;
    }
    else if (mouse_position->y >= max_height) {
        mouse_position->y -= max_height;
        wrapped = true;
    }

    return wrapped;
}

static void update_mouse_delta(Test *test, Platform *platform, Vulkan *vk) {
    Vec2<s32> mouse_position = get_mouse_position(platform);
    test->input.mouse_delta = mouse_position - test->input.last_mouse_position;

    // if (wrap_mouse_position(&mouse_position, vk->swapchain.extent.width, vk->swapchain.extent.height))
    //     set_mouse_position(platform, mouse_position);

    test->input.last_mouse_position = mouse_position;
}

static void camera_controls(Test *test, Platform *platform) {
    // Translation
    static constexpr f32 TRANSLATION_SPEED = 0.05f;
    f32 mod = key_down(platform, Key::SHIFT) ? 32 : 1;
    Vec3<f32> move_vec = {};

    if (key_down(platform, Key::D)) move_vec.x += TRANSLATION_SPEED * mod;
    if (key_down(platform, Key::A)) move_vec.x -= TRANSLATION_SPEED * mod;
    if (key_down(platform, Key::E)) move_vec.y -= TRANSLATION_SPEED * mod;
    if (key_down(platform, Key::Q)) move_vec.y += TRANSLATION_SPEED * mod;
    if (key_down(platform, Key::W)) move_vec.z += TRANSLATION_SPEED * mod;
    if (key_down(platform, Key::S)) move_vec.z -= TRANSLATION_SPEED * mod;

    Matrix model_matrix = MATRIX_ID;
    model_matrix = rotate(model_matrix, test->view.rotation.x, Axis::X);
    model_matrix = rotate(model_matrix, test->view.rotation.y, Axis::Y);
    model_matrix = rotate(model_matrix, test->view.rotation.z, Axis::Z);
    Vec3<f32> forward = { model_matrix[0][2], model_matrix[1][2], model_matrix[2][2] };
    Vec3<f32> right = { model_matrix[0][0], model_matrix[1][0], model_matrix[2][0] };
    test->view.position += move_vec.z * forward;
    test->view.position += move_vec.x * right;
    test->view.position.y += move_vec.y;

    // Rotation
    if (mouse_button_down(platform, 1)) {
        static constexpr f32 ROTATION_SPEED = 0.2f;
        test->view.rotation.x += test->input.mouse_delta.y * ROTATION_SPEED;
        test->view.rotation.y -= test->input.mouse_delta.x * ROTATION_SPEED;
        test->view.rotation.x = clamp(test->view.rotation.x, -test->view.max_x_angle, test->view.max_x_angle);
    }
}

static void handle_input(Test *test, Platform *platform, Vulkan *vk) {
    if (key_down(platform, Key::ESCAPE)) {
        platform->window->open = false;
        return;
    }

    update_mouse_delta(test, platform, vk);
    camera_controls(test, platform);

    //      if (key_down(platform, Key::F1)) { print_line("single thread"); use_threads = false; }
    // else if (key_down(platform, Key::F2)) { print_line("multi thread");  use_threads = true;  }
}

static Matrix calculate_view_space_matrix(View *view) {
    // View Matrix
    Matrix model_matrix = MATRIX_ID;
    model_matrix = rotate(model_matrix, view->rotation.x, Axis::X);
    model_matrix = rotate(model_matrix, view->rotation.y, Axis::Y);
    model_matrix = rotate(model_matrix, view->rotation.z, Axis::Z);
    Vec3<f32> forward = { model_matrix[0][2], model_matrix[1][2], model_matrix[2][2] };
    Matrix view_matrix = look_at(view->position, view->position + forward, { 0.0f, -1.0f, 0.0f });

    // Projection Matrix
    Matrix projection_matrix = perspective_matrix(view->perspective_info);
    projection_matrix[1][1] *= -1; // Flip y value for scale (glm is designed for OpenGL).

    return projection_matrix * view_matrix;
}

struct UpdateMVPMatrixesState {
    Test *test;
    Matrix view_space_matrix;
};

static void update_mvp_matrixes(UpdateMVPMatrixesState state, u32 thread_index) {
    Test *test = state.test;
    Matrix view_space_matrix = state.view_space_matrix;

    for (u32 i = 0; i < test->entities.count; ++i) {
        Entity *entity = test->entities.data + i;

        Matrix model_matrix = translate(MATRIX_ID, entity->position);
        model_matrix = rotate(model_matrix, entity->rotation.x, Axis::X);
        model_matrix = rotate(model_matrix, entity->rotation.y, Axis::Y);
        model_matrix = rotate(model_matrix, entity->rotation.z, Axis::Z);

        test->mvp_matrixes.data[i] = view_space_matrix * model_matrix;
    }
}

struct RecordRenderCmdsState {
    Test *test;
    Graphics *gfx;
    Range *thread_ranges;
};

static void record_render_cmds(RecordRenderCmdsState state, u32 thread_index) {
    Test *test = state.test;
    Graphics *gfx = state.gfx;
    Range range = state.thread_ranges[thread_index];
    VkCommandBuffer cmd_buf = gfx->render_cmd_bufs->data[gfx->sync.swap_img_idx]->data[thread_index];

    VkCommandBufferInheritanceInfo cmd_buf_inheritance_info = {};
    cmd_buf_inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    cmd_buf_inheritance_info.renderPass = gfx->main_render_pass->handle;
    cmd_buf_inheritance_info.subpass = 0;
    cmd_buf_inheritance_info.framebuffer = gfx->framebuffers->data[gfx->sync.swap_img_idx];
    cmd_buf_inheritance_info.occlusionQueryEnable = VK_FALSE;
    cmd_buf_inheritance_info.queryFlags = 0;
    cmd_buf_inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.flags = // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT |
                               VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmd_buf_begin_info.pInheritanceInfo = &cmd_buf_inheritance_info;
    validate_result(vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info),
                    "failed to begin recording command buffer");

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.test->handle);

    // Bind descriptor sets.
    VkDescriptorSet *ds = &gfx->descriptor_set.image_sampler->data[gfx->sync.swap_img_idx];
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.test->layout,
                            0, 1, &gfx->descriptor_set.image_sampler->data[gfx->sync.swap_img_idx],
                            0, NULL);

    // Bind mesh data.
    Mesh *mesh = &test->mesh.quad;
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &mesh->vertex_region->buffer->handle, &mesh->vertex_region->offset);
    vkCmdBindIndexBuffer(cmd_buf, mesh->index_region->buffer->handle, mesh->index_region->offset, VK_INDEX_TYPE_UINT32);

    for (u32 i = range.start; i < range.start + range.size; ++i) {
        vkCmdPushConstants(cmd_buf, gfx->pipeline.test->layout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, 64, &test->mvp_matrixes.data[i]);
        // u32 offset = i * 64;
        // vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.test->layout,
        //                         1, 1, &gfx->descriptor_set.mvp_matrix->data[gfx->sync.swap_img_idx],
        //                         1, &offset);
        vkCmdDrawIndexed(cmd_buf, mesh->indexes->count, 1, 0, 0, 0);
    }

    vkEndCommandBuffer(cmd_buf);
}

static void record_render_cmd_bufs(Test *test, Graphics *gfx, Vulkan *vk, u32 render_thread_count) {
    push_frame(test->mem->temp);

    auto thread_ranges = create_array<Range>(render_thread_count);
    partition_data(test->entities.count, thread_ranges->size, thread_ranges->data);

    RecordRenderCmdsState state = { test, gfx, thread_ranges->data };
    run_parallel(state, record_render_cmds, render_thread_count, test->mem->temp);

    pop_frame(test->mem->temp);
}

struct RecordRenderPassState {
    Test *test;
    Graphics *gfx;
    Vulkan *vk;
    u32 render_thread_count;
};

static void record_render_pass(RecordRenderPassState state, u32 thread_index) {
    Test *test = state.test;
    Graphics *gfx = state.gfx;
    Vulkan *vk = state.vk;
    u32 render_thread_count = state.render_thread_count;

    VkCommandBuffer cmd_buf = gfx->render_pass_cmd_bufs->data[gfx->sync.swap_img_idx];

    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.flags = 0;
    cmd_buf_begin_info.pInheritanceInfo = NULL;
    validate_result(vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info),
                    "failed to begin recording command buffer");

    VkRenderPassBeginInfo rp_begin_info = {};
    rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin_info.renderPass = gfx->main_render_pass->handle;
    rp_begin_info.framebuffer = gfx->framebuffers->data[gfx->sync.swap_img_idx];
    rp_begin_info.clearValueCount = gfx->main_render_pass->attachment_clear_values->count;
    rp_begin_info.pClearValues = gfx->main_render_pass->attachment_clear_values->data;
    rp_begin_info.renderArea = {
        .offset = { 0, 0 },
        .extent = vk->swapchain.extent,
    };
    vkCmdBeginRenderPass(cmd_buf, &rp_begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    record_render_cmd_bufs(test, gfx, vk, render_thread_count);
    Array<VkCommandBuffer> *render_cmd_bufs = gfx->render_cmd_bufs->data[gfx->sync.swap_img_idx];
    vkCmdExecuteCommands(cmd_buf, render_thread_count, render_cmd_bufs->data);

    vkCmdEndRenderPass(cmd_buf);

    vkEndCommandBuffer(cmd_buf);
}

static void update(Test *test, Graphics *gfx, Vulkan *vk, Platform *platform) {
    // Update uniform buffer data.
    Matrix view_space_matrix = calculate_view_space_matrix(&test->view);
start_benchmark(test->frame_benchmark, "update_mvp_matrixes()");
    {
        UpdateMVPMatrixesState state = { test, view_space_matrix };
        run_parallel(state, update_mvp_matrixes, 1, test->mem->temp);
    }
end_benchmark(test->frame_benchmark);

start_benchmark(test->frame_benchmark, "record_render_pass()");
    {
        RecordRenderPassState state = { test, gfx, vk, platform->thread_count - 2 };
        run_parallel(state, record_render_pass, 1, test->mem->temp); // Leave 2 threads available.
    }
end_benchmark(test->frame_benchmark);

    // Write to uniform buffers.
    begin_temp_cmd_buf(gfx->temp_cmd_buf);
        write_to_device_region(vk, gfx->temp_cmd_buf,
                               gfx->staging_region, sizeof(view_space_matrix),
                               test->uniform_buffer.mvp_matrixes->data[gfx->sync.swap_img_idx], 0,
                               test->mvp_matrixes.data, byte_size(&test->mvp_matrixes));
    submit_temp_cmd_buf(gfx->temp_cmd_buf, vk->queue.graphics);
}

////////////////////////////////////////////////////////////
/// Main
////////////////////////////////////////////////////////////
s32 main() {
    // Initialize Memory
    Allocator *fixed_mem = create_stack_allocator(gigabyte(1));
    auto mem = allocate<Memory>(fixed_mem, 1);
    mem->fixed = fixed_mem;
    mem->temp = create_stack_allocator(mem->fixed, megabyte(1));
    mem->platform = create_stack_allocator(mem->fixed, kilobyte(2));
    mem->vulkan = create_stack_allocator(mem->fixed, megabyte(4));
    mem->graphics = create_stack_allocator(mem->fixed, megabyte(4));

    // Create Modules
    static constexpr u32 WIN_WIDTH = 1600;
    Platform *platform = create_platform(mem->platform, {
        .surface = {
            .x = 0,
            .y = 100,
            .width = WIN_WIDTH,
            .height = 900,
        },
        .title = L"Renderer",
    });

    SetWindowPos(platform->window->handle, HWND_TOP,
                 GetSystemMetrics(SM_CXSCREEN) - WIN_WIDTH - 10, 100, 0, 0, SWP_NOSIZE);

    Vulkan *vk = create_vulkan(mem->vulkan, platform, {
        .max_buffers = 2,
        .max_regions = 32,
        .max_images = 16,
        .max_render_passes = 2,
        .max_shaders = 16,
        .max_pipelines = 8,
        .enable_validation = false,
    });

    Graphics *gfx = create_graphics(mem->graphics, vk, platform->thread_count - 2);
    Test *test = create_test(mem, gfx, vk, platform);

    // Main Loop
    clock_t start = clock();
    u32 frames = 0;
    while (1) {
start_benchmark(test->frame_benchmark, "frame");
        process_events(platform->window);

        // Quit event closed the window.
        if (!platform->window->open)
            break;

        // If window is open but not active (focused), skip frame processing.
        if (!window_is_active(platform->window))
            goto loop_end;

        handle_input(test, platform, vk);

        // Input closed the window.
        if (!platform->window->open)
            break;

        // Update
        next_frame(gfx, vk);
start_benchmark(test->frame_benchmark, "update()");
        update(test, gfx, vk, platform);
end_benchmark(test->frame_benchmark);
        submit_render_cmds(gfx, vk);

loop_end:
        // Update FPS display.
        clock_t end = clock();
        f64 frame_ms = clocks_to_ms(start, end);
        if (frame_ms >= 1000.0) {
            char buf[128] = {};
            sprintf(buf, "%.2f FPS", (f64)frames * (frame_ms / 1000.0));
            set_window_title(platform->window, buf);
            start = end;
            frames = 0;
        }
        else {
            ++frames;
        }
end_benchmark(test->frame_benchmark);
print_frame_benchmark(test->frame_benchmark);
reset_frame_benchmark(test->frame_benchmark);
    }

    return 0;
}
