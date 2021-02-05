#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "vtk/vtk.h"
#include "vtk/device_features.h"
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

struct PhysicalDevice {
    VkPhysicalDevice handle;
    QueueFamilyIndexes queue_fam_idxs;
    VkPhysicalDeviceFeatures feats;
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkFormat depth_img_fmt;
};

struct Device {
    VkDevice handle;
    struct {
        VkQueue graphics;
        VkQueue present;
    } queues;
};

struct Swapchain {
    VkSwapchainKHR handle;
    CTK_StaticArray<VkImageView, 4> image_views;
    u32 image_count;
    VkFormat image_format;
    VkExtent2D extent;
};

struct Vulkan {
    Instance instance;
    VkSurfaceKHR surface;
    PhysicalDevice physical_device;
    Device device;
};

static void create_instance(Instance *instance, CTK_Stack *stack) {
    u32 fn_region = ctk_begin_region(stack);
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
    debug_messenger_info.messageSeverity = // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
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

    ctk_end_region(stack, fn_region);
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

static QueueFamilyIndexes find_queue_family_idxs(VkPhysicalDevice physical_device, Vulkan *vk, CTK_Stack *stack) {
    u32 fn_region = ctk_begin_region(stack);

    QueueFamilyIndexes queue_fam_idxs = { CTK_U32_MAX, CTK_U32_MAX };
    auto queue_fam_props_array =
        vtk_load_vk_objects<VkQueueFamilyProperties>(stack, vkGetPhysicalDeviceQueueFamilyProperties, physical_device);

    for (u32 queue_fam_idx = 0; queue_fam_idx < queue_fam_props_array.count; ++queue_fam_idx) {
        VkQueueFamilyProperties *queue_fam_props = queue_fam_props_array + queue_fam_idx;
        if (queue_fam_props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queue_fam_idxs.graphics = queue_fam_idx;

        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_fam_idx, vk->surface, &present_supported);
        if (present_supported == VK_TRUE)
            queue_fam_idxs.present = queue_fam_idx;
    }

    ctk_end_region(stack, fn_region);
    return queue_fam_idxs;
}

static PhysicalDevice *find_suitable_physical_device(Vulkan *vk, CTK_Array<PhysicalDevice *> *physical_devices,
                                                     VTK_PhysicalDeviceFeatures *requested_features) {
    PhysicalDevice *suitable_device = NULL;

    for (u32 i = 0; suitable_device == NULL && i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data[i];

        // Check for queue families that support graphics and present.
        bool has_required_queue_families = physical_device->queue_fam_idxs.graphics != CTK_U32_MAX &&
                                           physical_device->queue_fam_idxs.present  != CTK_U32_MAX;

        // Check that all requested features are supported.
        VTK_PhysicalDeviceFeatures unsupported_features = {};

        for (u32 feat_idx = 0; feat_idx < requested_features->count; ++feat_idx) {
            s32 requested_feature = requested_features->data[feat_idx];
            if (!vtk_physical_device_feature_supported(requested_feature, &physical_device->feats))
                ctk_push(&unsupported_features, requested_feature);
        }

        bool requested_features_supported = unsupported_features.count == 0;

        // Check if device passes all tests and load more physical_device about device if so.
        if (has_required_queue_families && requested_features_supported)
            suitable_device = physical_device;
    }

    return suitable_device;
}

static void load_physical_device(Vulkan *vk, CTK_Stack *stack, VTK_PhysicalDeviceFeatures *requested_features) {
    u32 fn_region = ctk_begin_region(stack);

    // Load info about all physical devices.
    auto vk_physical_devices = vtk_load_vk_objects<VkPhysicalDevice>(stack, vkEnumeratePhysicalDevices,
                                                                     vk->instance.handle);
    auto physical_devices = ctk_create_array<PhysicalDevice>(stack, vk_physical_devices.count);

    for (u32 i = 0; i < vk_physical_devices.count; ++i) {
        VkPhysicalDevice vk_physical_device = vk_physical_devices[i];
        PhysicalDevice *physical_device = ctk_push(&physical_devices);
        physical_device->handle = vk_physical_device;
        physical_device->queue_fam_idxs = find_queue_family_idxs(vk_physical_device, vk, stack);
        vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device->feats);
        vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device->props);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device->mem_props);
        physical_device->depth_img_fmt = vtk_find_depth_image_format(physical_device->handle);
    }

    // Sort out discrete and integrated gpus.
    auto discrete_devices = ctk_create_array<PhysicalDevice *>(stack, physical_devices.count);
    auto integrated_devices = ctk_create_array<PhysicalDevice *>(stack, physical_devices.count);

    for (u32 i = 0; i < physical_devices.count; ++i) {
        PhysicalDevice *physical_device = physical_devices + i;
        if (physical_device->props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            ctk_push(&discrete_devices, physical_device);
        else if (physical_device->props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            ctk_push(&integrated_devices, physical_device);
    }

    // Find suitable discrete device, or fallback to an integrated device.
    PhysicalDevice *suitable_device = find_suitable_physical_device(vk, &discrete_devices, requested_features);
    if (suitable_device == NULL) {
        suitable_device = find_suitable_physical_device(vk, &integrated_devices, requested_features);
        if (suitable_device == NULL)
            CTK_FATAL("failed to find any suitable device");
    }

    vk->physical_device = *suitable_device;
    ctk_end_region(stack, fn_region);
}

static void create_device(Vulkan *vk, VTK_PhysicalDeviceFeatures *requested_features) {
    CTK_StaticArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    ctk_push(&queue_infos, vtk_default_queue_info(vk->physical_device.queue_fam_idxs.graphics));

    // Don't create separate queues if present and graphics belong to same queue family.
    if (vk->physical_device.queue_fam_idxs.present != vk->physical_device.queue_fam_idxs.graphics)
        ctk_push(&queue_infos, vtk_default_queue_info(vk->physical_device.queue_fam_idxs.present));

    cstr extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkBool32 enabled_features[VTK_PHYSICAL_DEVICE_FEATURE_COUNT] = {};
    for (u32 i = 0; i < requested_features->count; ++i)
        enabled_features[requested_features->data[i]] = VK_TRUE;

    VkDeviceCreateInfo logical_device_info = {};
    logical_device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logical_device_info.flags = 0;
    logical_device_info.queueCreateInfoCount = queue_infos.count;
    logical_device_info.pQueueCreateInfos = queue_infos.data;
    logical_device_info.enabledLayerCount = 0;
    logical_device_info.ppEnabledLayerNames = NULL;
    logical_device_info.enabledExtensionCount = CTK_ARRAY_COUNT(extensions);
    logical_device_info.ppEnabledExtensionNames = extensions;
    logical_device_info.pEnabledFeatures = (VkPhysicalDeviceFeatures *)enabled_features;
    vtk_validate_result(vkCreateDevice(vk->physical_device.handle, &logical_device_info, NULL, &vk->device.handle),
                        "failed to create logical device");

    // Get logical vk->device queues.
    vkGetDeviceQueue(vk->device.handle, vk->physical_device.queue_fam_idxs.graphics, 0, &vk->device.queues.graphics);
    vkGetDeviceQueue(vk->device.handle, vk->physical_device.queue_fam_idxs.present, 0, &vk->device.queues.present);
}

static void create_swapchain(Vulkan *vk, Memory *mem) {
    // u32 fn_region = ctk_begin_region(&mem->temp);

    // ////////////////////////////////////////////////////////////
    // /// Configuration
    // ////////////////////////////////////////////////////////////

    // // Configure swapchain based on surface properties.
    // auto surface_fmts =
    //     vtk_load_vk_objects<VkSurfaceFormatKHR>(&mem->temp, vkGetPhysicalDeviceSurfaceFormatsKHR,
    //                                             device->physical, surface);
    // auto surface_present_modes =
    //     vtk_load_vk_objects<VkPresentModeKHR>(&mem->temp, vkGetPhysicalDeviceSurfacePresentModesKHR,
    //                                           device->physical, surface);
    // VkSurfaceCapabilitiesKHR surface_capabilities = {};
    // vtk_validate_result(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical, surface, &surface_capabilities),
    //                     "failed to get physical device surface capabilities");

    // // Default to first surface format.
    // VkSurfaceFormatKHR selected_fmt = surface_fmts[0];
    // for (u32 i = 0; i < surface_fmts.count; ++i) {
    //     VkSurfaceFormatKHR surface_fmt = surface_fmts[i];

    //     // Prefer 4-component 8-bit BGRA unnormalized format and sRGB color space.
    //     if (surface_fmt.format == VK_FORMAT_B8G8R8A8_UNORM &&
    //         surface_fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
    //         selected_fmt = surface_fmt;
    //         break;
    //     }
    // }

    // // Default to FIFO (only present mode with guarenteed availability).
    // VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    // for (u32 i = 0; i < surface_present_modes.count; ++i) {
    //     VkPresentModeKHR surface_present_mode = surface_present_modes[i];

    //     // Mailbox is the preferred present mode if available.
    //     if (surface_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //         selected_present_mode = surface_present_mode;
    //         break;
    //     }
    // }

    // // Set image count to min image count + 1 or max image count (whichever is smaller).
    // u32 selected_image_count = surface_capabilities.minImageCount + 1;
    // if (surface_capabilities.maxImageCount > 0 && selected_image_count > surface_capabilities.maxImageCount)
    //     selected_image_count = surface_capabilities.maxImageCount;

    // // Verify current extent has been set for surface.
    // if (surface_capabilities.currentExtent.width == UINT32_MAX)
    //     CTK_FATAL("current extent not set for surface")

    // ////////////////////////////////////////////////////////////
    // /// Creation
    // ////////////////////////////////////////////////////////////
    // u32 graphics_queue_fam_idx = vk->physical_device.queue_fam_idxs.graphics;
    // u32 present_queue_fam_idx = vk->physical_device.queue_fam_idxs.present;
    // u32 queue_fam_idxs[] = { graphics_queue_fam_idx, present_queue_fam_idx };

    // VkSwapchainCreateInfoKHR info = {};
    // info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // info.surface = surface;
    // info.flags = 0;
    // info.minImageCount = selected_image_count;
    // info.imageFormat = selected_fmt.format;
    // info.imageColorSpace = selected_fmt.colorSpace;
    // info.imageExtent = surface_capabilities.currentExtent;
    // info.imageArrayLayers = 1; // Always 1 for standard images.
    // info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // info.preTransform = surface_capabilities.currentTransform;
    // info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    // info.presentMode = selected_present_mode;
    // info.clipped = VK_TRUE;
    // info.oldSwapchain = VK_NULL_HANDLE;
    // if (graphics_queue_fam_idx != present_queue_fam_idx) {
    //     info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    //     info.queueFamilyIndexCount = CTK_ARRAY_COUNT(queue_fam_idxs);
    //     info.pQueueFamilyIndices = queue_fam_idxs;
    // }
    // else {
    //     info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //     info.queueFamilyIndexCount = 0;
    //     info.pQueueFamilyIndices = NULL;
    // }
    // vtk_validate_result(vkCreateSwapchainKHR(vk->device.handle, &info, NULL, &swapchain.handle),
    //                     "failed to create swapchain");

    // // Store surface state used to create swapchain for future reference.
    // swapchain.image_format = selected_fmt.format;
    // swapchain.extent = surface_capabilities.currentExtent;

    // ////////////////////////////////////////////////////////////
    // /// Image View Creation
    // ////////////////////////////////////////////////////////////
    // auto swapchain_images = vtk_load_vk_objects<VkImage>(&mem->temp, vkGetSwapchainImagesKHR, vk->device.handle,
    //                                                      swapchain.handle);
    // swapchain.image_views = ctk_create_array_full<VkImageView>(&mem->free_list, swapchain_images.count);
    // for (u32 i = 0; i < swapchain_images.count; ++i) {
    //     VkImageViewCreateInfo view_info = {};
    //     view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    //     view_info.image = swapchain_images[i];
    //     view_info.flags = 0;
    //     view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //     view_info.format = swapchain.image_format;
    //     view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    //     view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    //     view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    //     view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    //     view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //     view_info.subresourceRange.baseMipLevel = 0;
    //     view_info.subresourceRange.levelCount = 1;
    //     view_info.subresourceRange.baseArrayLayer = 0;
    //     view_info.subresourceRange.layerCount = 1;
    //     vtk_validate_result(vkCreateImageView(vk->device.handle, &view_info, NULL, swapchain.image_views + i),
    //                         "failed to create image view");
    // }

    // ctk_end_region(&mem->temp, fn_region);
}

static Vulkan *create_vulkan(Core *core, Platform *platform) {
    auto vk = ctk_alloc<Vulkan>(&core->mem.perma, 1);
    create_instance(&vk->instance, &core->mem.temp);
    vk->surface = get_surface(&vk->instance, platform);

    // Physical/Logical Devices
    VTK_PhysicalDeviceFeatures requested_features = {};
    ctk_push(&requested_features, (s32)VTK_PHYSICAL_DEVICE_FEATURE_geometryShader);
    load_physical_device(vk, &core->mem.temp, &requested_features);
    create_device(vk, &requested_features);

    create_swapchain(vk, &core->mem);
    return vk;
}
