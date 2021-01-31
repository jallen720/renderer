#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "vtk/vtk.h"
#include "renderer/core.h"
#include "renderer/platform.h"

struct Vulkan {
    VTK_Instance instance;
};

static Vulkan *create_vulkan(Core *core) {
    u32 temp_region = ctk_begin_region(&core->mem.temp);
    auto vulkan = ctk_alloc<Vulkan>(&core->mem.perma, 1);
    auto extensions = ctk_create_array<cstr>(&core->mem.temp, 16);
    auto layers = ctk_create_array<cstr>(&core->mem.temp, 16);
    ctk_concat(&extensions, PLATFORM_VULKAN_EXTENSIONS, PLATFORM_VULKAN_EXTENSION_COUNT);
    ctk_push(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);

    // For validation layers.
    ctk_push(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    ctk_push(&layers, "VK_LAYER_KHRONOS_validation");

    vulkan->instance = vtk_create_instance(&extensions, &layers);
    ctk_end_region(&core->mem.temp, temp_region);
    return vulkan;
}
