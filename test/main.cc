#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb/stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "renderer/platform.h"
#include "renderer/vulkan.h"
#include "renderer/test/graphics.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "ctk/math.h"

using namespace ctk;

#define USE_GLM

namespace test {
#ifdef USE_GLM
    using Matrix = glm::mat4;

    template<typename Type>
    using Vec3 = glm::vec3;

    Matrix perspective(f32 fov, f32 aspect_ratio, f32 z_near, f32 z_far) {
        return glm::perspective(fov, aspect_ratio, z_near, z_far);
    }

    f32 radians(f32 degrees) {
        return glm::radians(degrees);
    }

    Matrix rotate(Matrix matrix, f32 degrees, Axis axis) {
        if (axis == Axis::X) return glm::rotate(matrix, radians(degrees), { 1, 0, 0 });
        if (axis == Axis::Y) return glm::rotate(matrix, radians(degrees), { 0, 1, 0 });
        if (axis == Axis::Z) return glm::rotate(matrix, radians(degrees), { 0, 0, 1 });
    }

    Matrix translate(Matrix matrix, f32 x, f32 y, f32 z) {
        return glm::translate(matrix, { x, y, z });
    }
#else
    using Matrix = ctk::Matrix;

    template<typename Type>
    using Vec3 = ctk::Vec3<Type>;

    Matrix perspective(f32 fov, f32 aspect_ratio, f32 z_near, f32 z_far) {
        return ctk::perspective(fov, aspect_ratio, z_near, z_far);
    }

    Matrix translate(Matrix matrix, f32 x, f32 y, f32 z) {
        return ctk::translate(Matrix, x, y, z);
    }
#endif
}

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
    f32 fov;
    f32 aspect;
    f32 z_near;
    f32 z_far;
    test::Vec3<f32> position;
    test::Vec3<f32> rotation;
};

struct Test {
    struct {
        Mesh quad;
    } mesh;

    struct {
        Image *test;
    } image;

    struct {
        Array<Region *> *color;
        Array<Region *> *matrix;
    } uniform_buffer;

    struct {
        ImageSampler test;
    } image_sampler;

    enum struct Pipeline {
        SAMPLER,
        COLOR,
    } pipeline;

    Vec3<f32> color;
    View view;

    struct {
        Vec2<s32> last_mouse_position;
        Vec2<s32> mouse_delta;
    } input;
};

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

static void create_meshes(Test *test, Memory *mem, Graphics *gfx, Vulkan *vk) {
    {
        auto vertexes = create_array<Vertex>(mem->fixed, 8);
        push(vertexes, { { -1,-1,-1 }, { 0, 0 } });
        push(vertexes, { { -1, 1,-1 }, { 0, 1 } });
        push(vertexes, { {  1, 1,-1 }, { 1, 1 } });
        push(vertexes, { {  1,-1,-1 }, { 1, 0 } });

        push(vertexes, { {  1,-1, 1 }, { 0, 0 } });
        push(vertexes, { {  1, 1, 1 }, { 0, 1 } });
        push(vertexes, { { -1, 1, 1 }, { 1, 1 } });
        push(vertexes, { { -1,-1, 1 }, { 1, 0 } });

        auto indexes = create_array<u32>(mem->fixed, 36);
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

static void create_uniform_buffers(Test *test, Memory *mem, Graphics *gfx, Vulkan *vk) {
    test->uniform_buffer.color = create_array<Region *>(mem->fixed, vk->swapchain.image_count);
    for (u32 i = 0; i < vk->swapchain.image_count; ++i)
        test->uniform_buffer.color->data[i] = allocate_uniform_buffer_region(vk, gfx->buffer.device, 16);

    test->uniform_buffer.matrix = create_array<Region *>(mem->fixed, vk->swapchain.image_count);
    for (u32 i = 0; i < vk->swapchain.image_count; ++i)
        test->uniform_buffer.matrix->data[i] = allocate_uniform_buffer_region(vk, gfx->buffer.device, 64);
}

static void create_image_samplers(Test *test, Graphics *gfx) {
    test->image_sampler.test = { test->image.test, gfx->sampler.test };
}

static void bind_descriptor_data(Test *test, Graphics *gfx, Vulkan *vk) {
    {
        for (u32 i = 0; i < vk->swapchain.image_count; ++i) {
            DescriptorBinding bindings[] = {
                {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .uniform_buffer = test->uniform_buffer.color->data[i]
                },
            };

            update_descriptor_set(vk, gfx->descriptor_set.color->data[i], CTK_ARRAY_SIZE(bindings), bindings);
        }
    }

    {
        for (u32 i = 0; i < vk->swapchain.image_count; ++i) {
            DescriptorBinding bindings[] = {
                {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .uniform_buffer = test->uniform_buffer.matrix->data[i]
                },
                {
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .image_sampler = &test->image_sampler.test
                },
            };

            update_descriptor_set(vk, gfx->descriptor_set.sampler->data[i], CTK_ARRAY_SIZE(bindings), bindings);
        }
    }
}

static Test *create_test(Memory *mem, Graphics *gfx, Vulkan *vk, Platform *platform) {
    auto test = allocate<Test>(mem->fixed, 1);
    create_meshes(test, mem, gfx, vk);
    create_images(test, gfx, vk);
    create_uniform_buffers(test, mem, gfx, vk);
    create_image_samplers(test, gfx);
    bind_descriptor_data(test, gfx, vk);

    test->view = {
        .fov = 90,
        .aspect = (f32)vk->swapchain.extent.width / vk->swapchain.extent.height,
        .z_near = 0.1f,
        .z_far = 100,
        .position = { 0, 0, -2 },
        .rotation = { 0, 0, 0 },
    };

    test->input.last_mouse_position = get_mouse_position(platform);
    return test;
}

static void camera_controls(Test *test, Platform *platform) {
    // Translation
    static constexpr f32 TRANSLATION_SPEED = 0.05f;
    f32 mod = key_down(platform, InputKey::SHIFT) ? 2 : 1;
    test::Vec3<f32> move_vec(0);

    if (key_down(platform, InputKey::D)) move_vec.x += TRANSLATION_SPEED * mod;
    if (key_down(platform, InputKey::A)) move_vec.x -= TRANSLATION_SPEED * mod;
    if (key_down(platform, InputKey::E)) move_vec.y -= TRANSLATION_SPEED * mod;
    if (key_down(platform, InputKey::Q)) move_vec.y += TRANSLATION_SPEED * mod;
    if (key_down(platform, InputKey::W)) move_vec.z += TRANSLATION_SPEED * mod;
    if (key_down(platform, InputKey::S)) move_vec.z -= TRANSLATION_SPEED * mod;

    test::Matrix model_matrix(1.0f);
    model_matrix = test::rotate(model_matrix, test->view.rotation.x, Axis::X);
    model_matrix = test::rotate(model_matrix, test->view.rotation.y, Axis::Y);
    model_matrix = test::rotate(model_matrix, test->view.rotation.z, Axis::Z);
    test::Vec3<f32> forward = { model_matrix[0][2], model_matrix[1][2], model_matrix[2][2] };
    test::Vec3<f32> right = { model_matrix[0][0], model_matrix[1][0], model_matrix[2][0] };
    test->view.position += move_vec.z * forward;
    test->view.position += move_vec.x * right;
    test->view.position.y += move_vec.y;

    // Rotation
    // if (key_down(platform, InputKey::MOUSE_0)) print_line("0");
    // if (key_down(platform, InputKey::MOUSE_1)) print_line("1");
    // if (key_down(platform, InputKey::MOUSE_2)) print_line("2");
    // if (key_down(platform, InputKey::MOUSE_3)) print_line("3");
    // if (key_down(platform, InputKey::MOUSE_4)) print_line("4");
    // if (key_down(platform, InputKey::MOUSE_2)) {
        static constexpr f32 ROTATION_SPEED = 0.2f;
        test->view.rotation.x += test->input.mouse_delta.y * ROTATION_SPEED;
        test->view.rotation.y -= test->input.mouse_delta.x * ROTATION_SPEED;
    // }
}

static void update_mouse_delta(Test *test, Platform *platform, Vulkan *vk) {
    Vec2<s32> mouse_position = get_mouse_position(platform);
    test->input.mouse_delta = mouse_position - test->input.last_mouse_position;

    // Wrap mouse to window.
    bool wrapped = false;

    if (mouse_position.x < 0) {
        mouse_position.x += vk->swapchain.extent.width;
        wrapped = true;
    }

    if (mouse_position.y < 0) {
        mouse_position.y += vk->swapchain.extent.height;
        wrapped = true;
    }

    if (mouse_position.x >= vk->swapchain.extent.width) {
        mouse_position.x -= vk->swapchain.extent.width;
        wrapped = true;
    }

    if (mouse_position.y >= vk->swapchain.extent.height) {
        mouse_position.y -= vk->swapchain.extent.height;
        wrapped = true;
    }

    if (wrapped) {
        set_mouse_position(platform, mouse_position);
    }

    test->input.last_mouse_position = mouse_position;
}

static void handle_input(Test *test, Platform *platform, Vulkan *vk) {
    if (key_down(platform, InputKey::ESCAPE)) {
        platform->window->open = false;
        return;
    }

    update_mouse_delta(test, platform, vk);
    camera_controls(test, platform);

         if (key_down(platform, InputKey::Z)) test->color = { 1, 0.5f, 0.5f };
    else if (key_down(platform, InputKey::X)) test->color = { 0.5f, 1, 0.5f };
    else if (key_down(platform, InputKey::C)) test->color = { 0.5f, 0.5f, 1 };

         if (key_down(platform, InputKey::NUM_1)) test->pipeline = Test::Pipeline::COLOR;
    else if (key_down(platform, InputKey::NUM_2)) test->pipeline = Test::Pipeline::SAMPLER;
}

static test::Matrix calculate_view_space_matrix(View *view) {
    // View Matrix
    test::Matrix model_matrix(1.0f);
    model_matrix = test::rotate(model_matrix, view->rotation.x, Axis::X);
    model_matrix = test::rotate(model_matrix, view->rotation.y, Axis::Y);
    model_matrix = test::rotate(model_matrix, view->rotation.z, Axis::Z);
    test::Vec3<f32> forward = { model_matrix[0][2], model_matrix[1][2], model_matrix[2][2] };
    test::Matrix view_matrix = glm::lookAt(view->position, view->position + forward, { 0.0f, -1.0f, 0.0f });

    // Projection Matrix
    test::Matrix projection_matrix = test::perspective(test::radians(view->fov), view->aspect, view->z_near, view->z_far);
    projection_matrix[1][1] *= -1; // Flip y value for scale (glm is designed for OpenGL).

    return projection_matrix * view_matrix;
}

static void update_render_state(Graphics *gfx, Vulkan *vk, Test *test) {
    begin_temp_cmd_buf(gfx->temp_cmd_buf);
        write_to_device_region(vk, gfx->temp_cmd_buf,
                               gfx->staging_region, 0,
                               test->uniform_buffer.color->data[gfx->sync.swap_img_idx], 0,
                               &test->color, sizeof(test->color));

        test::Matrix view_space_matrix = calculate_view_space_matrix(&test->view);

        write_to_device_region(vk, gfx->temp_cmd_buf,
                               gfx->staging_region, sizeof(test->color),
                               test->uniform_buffer.matrix->data[gfx->sync.swap_img_idx], 0,
                               &view_space_matrix, sizeof(view_space_matrix));
    submit_temp_cmd_buf(gfx->temp_cmd_buf, vk->queue.graphics);
}

static void render(Graphics *gfx, Vulkan *vk, Test *test) {
    VkCommandBuffer cmd_buf = gfx->swap_state.render_cmd_bufs->data[gfx->sync.swap_img_idx];
    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.flags = 0;
    cmd_buf_begin_info.pInheritanceInfo = NULL;
    validate_result(vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin_info), "failed to begin recording command buffer");

    VkRenderPassBeginInfo rp_begin_info = {};
    rp_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin_info.renderPass = gfx->main_render_pass->handle;
    rp_begin_info.framebuffer = gfx->swap_state.framebufs->data[gfx->sync.swap_img_idx];
    rp_begin_info.clearValueCount = gfx->main_render_pass->attachment_clear_values->count;
    rp_begin_info.pClearValues = gfx->main_render_pass->attachment_clear_values->data;
    rp_begin_info.renderArea = {
        .offset = { 0, 0 },
        .extent = vk->swapchain.extent,
    };
    vkCmdBeginRenderPass(cmd_buf, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        if (test->pipeline == Test::Pipeline::COLOR) {
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.color->handle);

            // Bind descriptor sets.
            VkDescriptorSet descriptor_sets[] = {
                gfx->descriptor_set.color->data[gfx->sync.swap_img_idx],
            };

            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.color->layout,
                                    0, CTK_ARRAY_SIZE(descriptor_sets), descriptor_sets,
                                    0, NULL);
        }
        else {
            vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.sampler->handle);

            // Bind descriptor sets.
            VkDescriptorSet descriptor_sets[] = {
                gfx->descriptor_set.sampler->data[gfx->sync.swap_img_idx],
            };

            vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx->pipeline.sampler->layout,
                                    0, CTK_ARRAY_SIZE(descriptor_sets), descriptor_sets,
                                    0, NULL);
        }

        // Bind mesh data.
        Mesh *mesh = &test->mesh.quad;
        vkCmdBindVertexBuffers(cmd_buf, 0, 1, &mesh->vertex_region->buffer->handle, &mesh->vertex_region->offset);
        vkCmdBindIndexBuffer(cmd_buf, mesh->index_region->buffer->handle, mesh->index_region->offset,
                             VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd_buf, mesh->indexes->count, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd_buf);

    vkEndCommandBuffer(cmd_buf);
}

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
    Platform *platform = create_platform(mem->platform, {
        .surface = {
            .x = 600,
            .y = 100,
            .width = 1000,
            .height = 600,
        },
        .title = L"Renderer",
    });
    set_mouse_visible(false);

    Vulkan *vk = create_vulkan(mem->vulkan, platform, {
        .max_buffers = 2,
        .max_regions = 32,
        .max_images = 16,
        .max_render_passes = 2,
        .max_shaders = 16,
        .max_pipelines = 8,
    });

    Graphics *gfx = create_graphics(mem->graphics, vk);
    Test *test = create_test(mem, gfx, vk, platform);

    // Main Loop
    while (1) {
        process_events(platform->window);

        if (!window_is_active(platform->window))
            continue;

        // Quit event closed the window.
        if (!platform->window->open)
            break;

        handle_input(test, platform, vk);

        // Input closed the window.
        if (!platform->window->open)
            break;

        // Rendering
        next_frame(gfx, vk);
        update_render_state(gfx, vk, test);
        render(gfx, vk, test);
        present(gfx, vk);
    }

    return 0;
}
