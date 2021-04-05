#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "vtk/vtk.h"
#include "vtk/device_features.h"
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
    QueueFamilyIndexes queue_family_indexes;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkFormat depth_image_format;
};

struct LogicalDevice {
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
    LogicalDevice logical_device;
    Swapchain swapchain;
    VkCommandPool graphics_command_pool;
    struct {

    } command_buffers;
    struct {
        VTK_Buffer host;
        VTK_Buffer device;
    } buffers;
    VTK_Region staging_region;
};

struct Graphics {
    struct {
        CTK_Stack *base;
        CTK_Stack *temp;
    } mem;
    Vulkan *vulkan;
};

static void init_instance(Instance *instance, CTK_Stack *temp_mem) {
    u32 fn_region = ctk_begin_region(temp_mem);

    auto extensions = ctk_create_array<cstr>(16, 0, &temp_mem->allocator);
    auto layers = ctk_create_array<cstr>(16, 0, &temp_mem->allocator);
    ctk_push(extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    ctk_push(extensions, VK_KHR_SURFACE_EXTENSION_NAME);
    ctk_push(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Validation
    ctk_push(layers, "VK_LAYER_KHRONOS_validation"); // Validation

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
    info.enabledLayerCount = layers->count;
    info.ppEnabledLayerNames = layers->data;
    info.enabledExtensionCount = extensions->count;
    info.ppEnabledExtensionNames = extensions->data;
    vtk_validate_result(vkCreateInstance(&info, NULL, &instance->handle), "failed to create Vulkan instance");

    VTK_LOAD_INSTANCE_EXTENSION_FUNCTION(instance->handle, vkCreateDebugUtilsMessengerEXT);
    vtk_validate_result(
        vkCreateDebugUtilsMessengerEXT(instance->handle, &debug_messenger_info, NULL, &instance->debug_messenger),
        "failed to create debug messenger");

    ctk_end_region(temp_mem, fn_region);
}

static void init_surface(Vulkan *vulkan, Platform *platform) {
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = platform->window->handle;
    info.hinstance = platform->instance;
    vtk_validate_result(vkCreateWin32SurfaceKHR(vulkan->instance.handle, &info, nullptr, &vulkan->surface),
                        "failed to get win32 surface");
}

static QueueFamilyIndexes find_queue_family_indexes(VkPhysicalDevice physical_device, Vulkan *vulkan,
                                                    CTK_Stack *temp_mem) {
    u32 fn_region = ctk_begin_region(temp_mem);

    QueueFamilyIndexes queue_family_indexes = { CTK_U32_MAX, CTK_U32_MAX };
    auto queue_family_props_array =
        vtk_load_vk_objects<VkQueueFamilyProperties>(
            &temp_mem->allocator,
            vkGetPhysicalDeviceQueueFamilyProperties,
            physical_device);

    for (u32 queue_family_index = 0; queue_family_index < queue_family_props_array->count; ++queue_family_index) {
        VkQueueFamilyProperties *queue_family_props = queue_family_props_array->data + queue_family_index;
        if (queue_family_props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queue_family_indexes.graphics = queue_family_index;

        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, vulkan->surface, &present_supported);
        if (present_supported == VK_TRUE)
            queue_family_indexes.present = queue_family_index;
    }

    ctk_end_region(temp_mem, fn_region);
    return queue_family_indexes;
}

static PhysicalDevice *find_suitable_physical_device(Vulkan *vulkan, CTK_Array<PhysicalDevice *> *physical_devices,
                                                     VTK_PhysicalDeviceFeatureArray *requested_features) {
    PhysicalDevice *suitable_device = NULL;
    for (u32 i = 0; suitable_device == NULL && i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data[i];

        // Check for queue families that support graphics and present.
        bool has_required_queue_families = physical_device->queue_family_indexes.graphics != CTK_U32_MAX &&
                                           physical_device->queue_family_indexes.present  != CTK_U32_MAX;

        // Check that all requested features are supported.
        VTK_PhysicalDeviceFeatureArray unsupported_features = {};
        for (u32 feat_index = 0; feat_index < requested_features->count; ++feat_index) {
            s32 requested_feature = requested_features->data[feat_index];
            if (!vtk_physical_device_feature_supported(requested_feature, &physical_device->features))
                ctk_push(&unsupported_features, requested_feature);
        }
        bool requested_features_supported = unsupported_features.count == 0;

        // Check if device passes all tests and load more physical_device about device if so.
        if (has_required_queue_families && requested_features_supported)
            suitable_device = physical_device;
    }
    return suitable_device;
}

static void load_physical_device(Vulkan *vulkan, CTK_Stack *temp_mem, VTK_PhysicalDeviceFeatureArray *requested_features) {
    u32 fn_region = ctk_begin_region(temp_mem);

    // Load info about all physical devices.
    auto vk_physical_devices =
        vtk_load_vk_objects<VkPhysicalDevice>(&temp_mem->allocator, vkEnumeratePhysicalDevices, vulkan->instance.handle);

    auto physical_devices = ctk_create_array<PhysicalDevice>(vk_physical_devices->count, 0, &temp_mem->allocator);

    for (u32 i = 0; i < vk_physical_devices->count; ++i) {
        VkPhysicalDevice vk_physical_device = vk_physical_devices->data[i];
        PhysicalDevice *physical_device = ctk_push(physical_devices);
        physical_device->handle = vk_physical_device;
        physical_device->queue_family_indexes = find_queue_family_indexes(vk_physical_device, vulkan, temp_mem);
        vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device->features);
        vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device->properties);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device->memory_properties);
        physical_device->depth_image_format = vtk_find_depth_image_format(physical_device->handle);
    }

    // Sort out discrete and integrated gpus.
    auto discrete_devices = ctk_create_array<PhysicalDevice *>(physical_devices->count, 0, &temp_mem->allocator);
    auto integrated_devices = ctk_create_array<PhysicalDevice *>(physical_devices->count, 0, &temp_mem->allocator);

    for (u32 i = 0; i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data + i;
        if (physical_device->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            ctk_push(discrete_devices, physical_device);
        else if (physical_device->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            ctk_push(integrated_devices, physical_device);
    }

    // Find suitable discrete device, or fallback to an integrated device.
    PhysicalDevice *suitable_device = find_suitable_physical_device(vulkan, discrete_devices, requested_features);
    if (suitable_device == NULL) {
        suitable_device = find_suitable_physical_device(vulkan, integrated_devices, requested_features);
        if (suitable_device == NULL)
            CTK_FATAL("failed to find any suitable device");
    }

    vulkan->physical_device = *suitable_device;
    ctk_end_region(temp_mem, fn_region);
}

static void init_logical_device(Vulkan *vulkan, VTK_PhysicalDeviceFeatureArray *requested_features) {
    CTK_StaticArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    ctk_push(&queue_infos, vtk_default_queue_info(vulkan->physical_device.queue_family_indexes.graphics));

    // Don't create separate queues if present and graphics belong to same queue family.
    if (vulkan->physical_device.queue_family_indexes.present != vulkan->physical_device.queue_family_indexes.graphics)
        ctk_push(&queue_infos, vtk_default_queue_info(vulkan->physical_device.queue_family_indexes.present));

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
    vtk_validate_result(
        vkCreateDevice(vulkan->physical_device.handle, &logical_device_info, NULL, &vulkan->logical_device.handle),
        "failed to create logical device");

    // Get logical vulkan->logical_device queues.
    vkGetDeviceQueue(vulkan->logical_device.handle, vulkan->physical_device.queue_family_indexes.graphics, 0,
                     &vulkan->logical_device.queues.graphics);
    vkGetDeviceQueue(vulkan->logical_device.handle, vulkan->physical_device.queue_family_indexes.present, 0,
                     &vulkan->logical_device.queues.present);
}

static void init_swapchain(Vulkan *vulkan, CTK_Stack *temp_mem) {
    u32 fn_region = ctk_begin_region(temp_mem);

    ////////////////////////////////////////////////////////////
    /// Configuration
    ////////////////////////////////////////////////////////////

    // Configure swapchain based on surface properties.
    auto surface_formats = vtk_load_vk_objects<VkSurfaceFormatKHR>(&temp_mem->allocator,
                                                                   vkGetPhysicalDeviceSurfaceFormatsKHR,
                                                                   vulkan->physical_device.handle, vulkan->surface);
    auto surface_present_modes = vtk_load_vk_objects<VkPresentModeKHR>(&temp_mem->allocator,
                                                                       vkGetPhysicalDeviceSurfacePresentModesKHR,
                                                                       vulkan->physical_device.handle, vulkan->surface);
    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vtk_validate_result(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan->physical_device.handle, vulkan->surface, &surface_capabilities),
        "failed to get physical device surface capabilities");

    // Default to first surface format.
    VkSurfaceFormatKHR selected_format = surface_formats->data[0];
    for (u32 i = 0; i < surface_formats->count; ++i) {
        VkSurfaceFormatKHR surface_format = surface_formats->data[i];

        // Prefer 4-component 8-bit BGRA unnormalized format and sRGB color space.
        if (surface_format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selected_format = surface_format;
            break;
        }
    }

    // Default to FIFO (only present mode with guarenteed availability).
    VkPresentModeKHR selected_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < surface_present_modes->count; ++i) {
        VkPresentModeKHR surface_present_mode = surface_present_modes->data[i];

        // Mailbox is the preferred present mode if available.
        if (surface_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            selected_present_mode = surface_present_mode;
            break;
        }
    }

    // Set image count to min image count + 1 or max image count (whichever is smaller).
    u32 selected_image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && selected_image_count > surface_capabilities.maxImageCount)
        selected_image_count = surface_capabilities.maxImageCount;

    // Verify current extent has been set for surface.
    if (surface_capabilities.currentExtent.width == UINT32_MAX)
        CTK_FATAL("current extent not set for surface")

    ////////////////////////////////////////////////////////////
    /// Creation
    ////////////////////////////////////////////////////////////
    u32 graphics_queue_family_index = vulkan->physical_device.queue_family_indexes.graphics;
    u32 present_queue_family_index = vulkan->physical_device.queue_family_indexes.present;
    u32 queue_family_indexes[] = { graphics_queue_family_index, present_queue_family_index };

    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = vulkan->surface;
    info.flags = 0;
    info.minImageCount = selected_image_count;
    info.imageFormat = selected_format.format;
    info.imageColorSpace = selected_format.colorSpace;
    info.imageExtent = surface_capabilities.currentExtent;
    info.imageArrayLayers = 1; // Always 1 for standard images.
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.preTransform = surface_capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = selected_present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;
    if (graphics_queue_family_index != present_queue_family_index) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = CTK_ARRAY_COUNT(queue_family_indexes);
        info.pQueueFamilyIndices = queue_family_indexes;
    }
    else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = NULL;
    }
    vtk_validate_result(vkCreateSwapchainKHR(vulkan->logical_device.handle, &info, NULL, &vulkan->swapchain.handle),
                        "failed to create swapchain");

    // Store surface state used to create swapchain for future reference.
    vulkan->swapchain.image_format = selected_format.format;
    vulkan->swapchain.extent = surface_capabilities.currentExtent;

    ////////////////////////////////////////////////////////////
    /// Image View Creation
    ////////////////////////////////////////// //////////////////
    auto swapchain_images = vtk_load_vk_objects<VkImage>(&temp_mem->allocator, vkGetSwapchainImagesKHR,
                                                         vulkan->logical_device.handle, vulkan->swapchain.handle);
    CTK_ASSERT(swapchain_images->count <= ctk_size(&vulkan->swapchain.image_views));
    vulkan->swapchain.image_views.count = swapchain_images->count;
    for (u32 i = 0; i < swapchain_images->count; ++i) {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = swapchain_images->data[i];
        view_info.flags = 0;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = vulkan->swapchain.image_format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        vtk_validate_result(
            vkCreateImageView(vulkan->logical_device.handle, &view_info, NULL, vulkan->swapchain.image_views.data + i),
            "failed to create image view");
    }

    ctk_end_region(temp_mem, fn_region);
}

static void init_graphics_command_pool(Vulkan *vulkan) {
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = vulkan->physical_device.queue_family_indexes.graphics;
    vtk_validate_result(
        vkCreateCommandPool(vulkan->logical_device.handle, &cmd_pool_info, NULL, &vulkan->graphics_command_pool),
        "failed to create command pool");
}

static void init_buffers(Vulkan *vulkan) {
    VTK_BufferInfo host_buf_info = {};
    host_buf_info.size = 256 * CTK_MEGABYTE;
    host_buf_info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    host_buf_info.memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    host_buf_info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    vulkan->buffers.host = vtk_create_buffer(vulkan->logical_device.handle, vulkan->physical_device.memory_properties,
                                             &host_buf_info);

    VTK_BufferInfo device_buf_info = {};
    device_buf_info.size = 256 * CTK_MEGABYTE;
    device_buf_info.usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    device_buf_info.memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    device_buf_info.sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    vulkan->buffers.device = vtk_create_buffer(vulkan->logical_device.handle, vulkan->physical_device.memory_properties,
                                               &device_buf_info);
}

static void init_vulkan(Graphics *graphics, Platform *platform) {
    Vulkan *vulkan = ctk_alloc<Vulkan>(graphics->mem.base, 1);
    init_instance(&vulkan->instance, graphics->mem.temp);
    init_surface(vulkan, platform);

    // Physical/Logical Devices
    VTK_PhysicalDeviceFeatureArray requested_features = {};
    ctk_push(&requested_features, (s32)VTK_PHYSICAL_DEVICE_FEATURE_geometryShader);
    load_physical_device(vulkan, graphics->mem.temp, &requested_features);
    init_logical_device(vulkan, &requested_features);

    init_swapchain(vulkan, graphics->mem.temp);
    init_graphics_command_pool(vulkan);
    init_buffers(vulkan);
    vulkan->staging_region = vtk_allocate_region(&vulkan->buffers.host, 64 * CTK_MEGABYTE);

    graphics->vulkan = vulkan;
}

static Graphics *create_graphics(Platform *platform) {
    // Allocate memory for graphics module.
    CTK_Stack *base = ctk_create_stack(CTK_GIGABYTE);
    auto graphics = ctk_alloc<Graphics>(base, 1);
    graphics->mem.base = base;
    graphics->mem.temp = ctk_create_stack(CTK_MEGABYTE, &base->allocator);

    // Initialization
    init_vulkan(graphics, platform);

    return graphics;
}
