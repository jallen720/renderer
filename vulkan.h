#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "vtk/vtk.h"
#include "renderer/core.h"
#include "renderer/platform.h"

struct Instance {
    VkInstance handle;
    VkDebugUtilsMessengerEXT debug_messenger;
};

struct QueueFamilyIndexes {
    u32 graphics;
    u32 present;
};

struct Device {
    VkDevice handle;
    QueueFamilyIndexes queue_fam_idxs;
    struct {
        VkPhysicalDevice handle;
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceMemoryProperties mem_props;
        VkFormat depth_img_fmt;
    } physical;
    struct {
        VkQueue graphics;
        VkQueue present;
    } queues;
};

struct Vulkan {
    Instance instance;
    VkSurfaceKHR surface;
    Device device;
};

static void init_instance(Instance *instance, CTK_Stack *stack) {
    u32 stack_region = ctk_begin_region(stack);
    auto extensions = ctk_create_array<cstr>(stack, 16);
    auto layers = ctk_create_array<cstr>(stack, 16);
    ctk_concat(&extensions, PLATFORM_VULKAN_EXTENSIONS, PLATFORM_VULKAN_EXTENSION_COUNT);
    ctk_push(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);

    // For validation layers.
    ctk_push(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    ctk_push(&layers, "VK_LAYER_KHRONOS_validation");

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = {};
    debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_info.pNext = NULL;
    debug_messenger_info.flags = 0;
    debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                           // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_info.pfnUserCallback = vtk_debug_callback;
    debug_messenger_info.pUserData = NULL;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "renderer";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext = &debug_messenger_info;
    info.flags = 0;
    info.pApplicationInfo = &app_info;
    info.enabledLayerCount = layers.count;
    info.ppEnabledLayerNames = layers.data;
    info.enabledExtensionCount = extensions.count;
    info.ppEnabledExtensionNames = extensions.data;
    vtk_validate_result(vkCreateInstance(&info, NULL, &instance->handle), "failed to create Vulkan instance");

    VTK_LOAD_INSTANCE_EXTENSION_FUNCTION(instance->handle, vkCreateDebugUtilsMessengerEXT)
    vtk_validate_result(
        vkCreateDebugUtilsMessengerEXT(instance->handle, &debug_messenger_info, NULL, &instance->debug_messenger),
        "failed to create debug messenger");

    ctk_end_region(stack, stack_region);
}

static VkSurfaceKHR get_surface(Instance *instance, Platform *platform) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = platform->window->handle;
    info.hinstance = platform->instance;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    vtk_validate_result(vkCreateWin32SurfaceKHR(instance->handle, &info, nullptr, &surface),
                        "failed to get win32 surface");
#else
#endif

    return surface;
}

static QueueFamilyIndexes default_queue_fam_idxs() {
    return {
        CTK_U32_MAX,
        CTK_U32_MAX,
    };
}

static VkDeviceQueueCreateInfo default_queue_info(u32 queue_fam_idx) {
    static f32 const QUEUE_PRIORITIES[] = { 1.0f };

    VkDeviceQueueCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.flags = 0;
    info.queueFamilyIndex = queue_fam_idx;
    info.queueCount = CTK_ARRAY_COUNT(QUEUE_PRIORITIES);
    info.pQueuePriorities = QUEUE_PRIORITIES;

    return info;
}

static void load_device(Vulkan *vk) {
    ////////////////////////////////////////////////////////////
    /// Physical
    ////////////////////////////////////////////////////////////
    CTK_StaticArray<VkPhysicalDevice, 8> phys_devices = {};
    vtk_load_vk_objects(&phys_devices, vkEnumeratePhysicalDevices, vk->instance.handle);

    // Find suitable device for graphics/present.
    bool phys_device_found = false;
    for (u32 i = 0; !phys_device_found && i < phys_devices.count; ++i) {
        VkPhysicalDevice phys_device = phys_devices[i];

        // Check for queue families that support graphics and present.
        QueueFamilyIndexes queue_fam_idxs = default_queue_fam_idxs();

        CTK_StaticArray<VkQueueFamilyProperties, 8> queue_fam_props_arr = {};
        vtk_load_vk_objects(&queue_fam_props_arr, vkGetPhysicalDeviceQueueFamilyProperties, phys_device);

        for (u32 queue_fam_idx = 0; queue_fam_idx < queue_fam_props_arr.count; ++queue_fam_idx) {
            VkQueueFamilyProperties *queue_fam_props = queue_fam_props_arr + queue_fam_idx;
            if (queue_fam_props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
                queue_fam_idxs.graphics = queue_fam_idx;

            VkBool32 present_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phys_device, queue_fam_idx, vk->surface, &present_supported);
            if (present_supported == VK_TRUE)
                queue_fam_idxs.present = queue_fam_idx;
        }

        if (queue_fam_idxs.graphics == CTK_U32_MAX || queue_fam_idxs.present == CTK_U32_MAX)
            continue;

        // Physical device found; initialize.
        phys_device_found = true;
        vk->device.physical.handle = phys_device;
        vk->device.queue_fam_idxs = queue_fam_idxs;
        vkGetPhysicalDeviceProperties(vk->device.physical.handle, &vk->device.physical.props);
        vkGetPhysicalDeviceMemoryProperties(vk->device.physical.handle, &vk->device.physical.mem_props);
        vk->device.physical.depth_img_fmt = vtk_find_depth_image_format(vk->device.physical.handle);
    }

    if (!phys_device_found)
        CTK_FATAL("failed to find suitable physical device");

    ////////////////////////////////////////////////////////////
    /// Logical
    ////////////////////////////////////////////////////////////
    CTK_StaticArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    ctk_push(&queue_infos, default_queue_info(vk->device.queue_fam_idxs.graphics));

    // Don't create separate queues if present and graphics belong to same queue family.
    if (vk->device.queue_fam_idxs.present != vk->device.queue_fam_idxs.graphics)
        ctk_push(&queue_infos, default_queue_info(vk->device.queue_fam_idxs.present));

    VkPhysicalDeviceFeatures features = {};
    cstr extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo logical_device_info = {};
    logical_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_info.flags = 0;
    logical_device_info.queueCreateInfoCount = queue_infos.count;
    logical_device_info.pQueueCreateInfos = queue_infos.data;
    logical_device_info.enabledLayerCount = 0;
    logical_device_info.ppEnabledLayerNames = NULL;
    logical_device_info.enabledExtensionCount = CTK_ARRAY_COUNT(extensions);
    logical_device_info.ppEnabledExtensionNames = extensions;
    logical_device_info.pEnabledFeatures = &features;
    vtk_validate_result(vkCreateDevice(vk->device.physical.handle, &logical_device_info, NULL, &vk->device.handle),
                        "failed to create logical device");

    // Get logical vk->device queues.
    vkGetDeviceQueue(vk->device.handle, vk->device.queue_fam_idxs.graphics, 0, &vk->device.queues.graphics);
    vkGetDeviceQueue(vk->device.handle, vk->device.queue_fam_idxs.present, 0, &vk->device.queues.present);

}

static Vulkan *create_vulkan(Core *core, Platform *platform) {
    auto vk = ctk_alloc<Vulkan>(&core->mem.perma, 1);
    init_instance(&vk->instance, &core->mem.temp);
    vk->surface = get_surface(&vk->instance, platform);
    load_device(vk);
    return vk;
}
