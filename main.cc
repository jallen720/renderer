#include "renderer/platform.h"
#include "renderer/graphics.h"

// struct Mesh {
//     CTK_Array<CTK_Vector3<f32>> vertexes;
//     CTK_Array<u32> indexes;
//     VTK_Region vertex_region;
//     VTK_Region index_region;
// };

// static void load_mesh(Mesh *mesh) {
//     u32 verts_byte_size = ctk_byte_size(&mesh->vertexes);
//     u32 idxs_byte_size = ctk_byte_size(&mesh->indexes);
//     mesh->vertex_region = vtk_allocate_region(&vk->buffers.device, verts_byte_size, 16);
//     mesh->index_region = vtk_allocate_region(&vk->buffers.device, idxs_byte_size, 4);
//     vtk_begin_one_time_command_buffer(app->cmd_bufs.one_time);
//         vtk_write_to_device_region(&vk->device, app->cmd_bufs.one_time, mesh->vertexes.data, verts_byte_size,
//                                    &vk->staging_region, 0, &mesh->vertex_region, 0);
//         vtk_write_to_device_region(&vk->device, app->cmd_bufs.one_time, mesh->indexes.data, idxs_byte_size,
//                                    &vk->staging_region, verts_byte_size, &mesh->index_region, 0);
//     vtk_submit_one_time_command_buffer(app->cmd_bufs.one_time, vk->device.queues.graphics);
// }

s32 main() {
    Platform *platform = create_platform();
    Graphics *graphics = create_graphics(platform);

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
