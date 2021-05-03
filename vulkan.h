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
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkFormat depth_image_format;
};

struct LogicalDevice {
    VkDevice handle;

    struct {
        VkQueue graphics;
        VkQueue present;
    } queue;
};

struct Swapchain {
    VkSwapchainKHR handle;
    CTK_FixedArray<VkImageView, 4> image_views;
    u32 image_count;
    VkFormat image_format;
    VkExtent2D extent;
};

struct BufferInfo {
    VkDeviceSize size;
    VkSharingMode sharing_mode;
    VkBufferUsageFlags usage_flags;
    VkMemoryPropertyFlags mem_property_flags;
};

struct Buffer {
    VkBuffer handle;
    VkDeviceMemory mem;
    VkDeviceSize size;
    VkDeviceSize end;
};

struct Region {
    Buffer *buffer;
    VkDeviceSize size;
    VkDeviceSize offset;
};

struct SubpassInfo {
    CTK_Array<u32> *preserve_attachment_indexes;
    CTK_Array<VkAttachmentReference> *input_attachment_references;
    CTK_Array<VkAttachmentReference> *color_attachment_references;
    VkAttachmentReference *depth_attachment_reference;
};

struct FramebufferInfo {
    CTK_Array<VkImageView> *attachments;
    VkExtent2D extent;
    u32 layers;
};

struct RenderPassInfo {
    CTK_Array<VkAttachmentDescription> *attachment_descriptions;
    CTK_Array<SubpassInfo> *subpass_infos;
    CTK_Array<VkSubpassDependency> *subpass_dependencies;
    CTK_Array<FramebufferInfo> *framebuffer_infos;
    CTK_Array<VkClearValue> *clear_values;
};

struct RenderPass {
    VkRenderPass handle;
    CTK_Array<VkClearValue> *clear_values;
    CTK_Array<VkFramebuffer> *framebuffers;
};

struct VulkanInfo {
    u32 max_buffers;
    u32 max_regions;
};

struct Vulkan {
    struct {
        CTK_Allocator *module;
        CTK_Allocator *temp;
    } mem;

    struct {
        CTK_Pool<Region> *region;
        CTK_Pool<Buffer> *buffer;
    } pool;

    // State
    Instance instance;
    VkSurfaceKHR surface;

    struct {
        PhysicalDevice physical;
        LogicalDevice logical;
    } device;

    Swapchain swapchain;
    VkCommandPool command_pool;
};

static void init_instance(Vulkan *vk) {
    Instance *instance = &vk->instance;

    cstr extensions[] = {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // Validation
    };

    cstr layers[] = {
        "VK_LAYER_KHRONOS_validation", // Validation
    };

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
    info.enabledLayerCount = CTK_ARRAY_SIZE(layers);
    info.ppEnabledLayerNames = layers;
    info.enabledExtensionCount = CTK_ARRAY_SIZE(extensions);
    info.ppEnabledExtensionNames = extensions;
    vtk_validate_result(vkCreateInstance(&info, NULL, &instance->handle), "failed to create Vulkan instance");

    VTK_LOAD_INSTANCE_EXTENSION_FUNCTION(instance->handle, vkCreateDebugUtilsMessengerEXT);
    vtk_validate_result(
        vkCreateDebugUtilsMessengerEXT(instance->handle, &debug_messenger_info, NULL, &instance->debug_messenger),
        "failed to create debug messenger");
}

static void init_surface(Vulkan *vk, Platform *platform) {
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = platform->window->handle;
    info.hinstance = platform->instance;
    vtk_validate_result(vkCreateWin32SurfaceKHR(vk->instance.handle, &info, nullptr, &vk->surface),
                        "failed to get win32 surface");
}

static QueueFamilyIndexes find_queue_family_indexes(Vulkan *vk, VkPhysicalDevice physical_device) {
    ctk_push_frame(vk->mem.temp);

    QueueFamilyIndexes queue_family_indexes = { CTK_U32_MAX, CTK_U32_MAX };
    auto queue_family_props_array =
        vtk_load_vk_objects<VkQueueFamilyProperties>(
            vk->mem.temp,
            vkGetPhysicalDeviceQueueFamilyProperties,
            physical_device);

    for (u32 queue_family_index = 0; queue_family_index < queue_family_props_array->count; ++queue_family_index) {
        VkQueueFamilyProperties *queue_family_props = queue_family_props_array->data + queue_family_index;
        if (queue_family_props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queue_family_indexes.graphics = queue_family_index;

        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_index, vk->surface, &present_supported);
        if (present_supported == VK_TRUE)
            queue_family_indexes.present = queue_family_index;
    }

    ctk_pop_frame(vk->mem.temp);
    return queue_family_indexes;
}

static PhysicalDevice *find_suitable_physical_device(Vulkan *vk, CTK_Array<PhysicalDevice *> *physical_devices,
                                                     CTK_Array<s32> *requested_features) {
    ctk_push_frame(vk->mem.temp);

    CTK_Array<s32> *unsupported_features =
        requested_features
        ? ctk_create_array<s32>(vk->mem.temp, requested_features->size)
        : NULL;

    PhysicalDevice *suitable_device = NULL;
    for (u32 i = 0; suitable_device == NULL && i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data[i];

        // Check for queue families that support vk and present.
        bool has_required_queue_families = physical_device->queue_family_indexes.graphics != CTK_U32_MAX &&
                                           physical_device->queue_family_indexes.present != CTK_U32_MAX;

        bool requested_features_supported = true;
        if (requested_features) {
            ctk_clear(unsupported_features);

            // Check that all requested features are supported.
            for (u32 feat_index = 0; feat_index < requested_features->count; ++feat_index) {
                s32 requested_feature = requested_features->data[feat_index];

                if (!vtk_physical_device_feature_supported(requested_feature, &physical_device->features))
                    ctk_push(unsupported_features, requested_feature);
            }

            if (unsupported_features->count > 0) {
                requested_features_supported = false;
                CTK_TODO("report unsupported features");
            }
        }

        // Check if device passes all tests and load more physical_device about device if so.
        if (has_required_queue_families && requested_features_supported)
            suitable_device = physical_device;
    }

    ctk_pop_frame(vk->mem.temp);
    return suitable_device;
}

static void load_physical_device(Vulkan *vk, CTK_Array<s32> *requested_features) {
    ctk_push_frame(vk->mem.temp);

    // Load info about all physical devices.
    auto vk_physical_devices =
        vtk_load_vk_objects<VkPhysicalDevice>(
            vk->mem.temp,
            vkEnumeratePhysicalDevices,
            vk->instance.handle);

    auto physical_devices = ctk_create_array<PhysicalDevice>(vk->mem.temp, vk_physical_devices->count);

    for (u32 i = 0; i < vk_physical_devices->count; ++i) {
        VkPhysicalDevice vk_physical_device = vk_physical_devices->data[i];
        PhysicalDevice *physical_device = ctk_push(physical_devices);
        physical_device->handle = vk_physical_device;
        physical_device->queue_family_indexes = find_queue_family_indexes(vk, vk_physical_device);

        vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device->features);
        vkGetPhysicalDeviceProperties(vk_physical_device, &physical_device->properties);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device->mem_properties);
        physical_device->depth_image_format = vtk_find_depth_image_format(physical_device->handle);
    }

    // Sort out discrete and integrated gpus.
    auto discrete_devices = ctk_create_array<PhysicalDevice *>(vk->mem.temp, physical_devices->count);
    auto integrated_devices = ctk_create_array<PhysicalDevice *>(vk->mem.temp, physical_devices->count);

    for (u32 i = 0; i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data + i;

        if (physical_device->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            ctk_push(discrete_devices, physical_device);
        else if (physical_device->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            ctk_push(integrated_devices, physical_device);
    }

    // Find suitable discrete device, or fallback to an integrated device.
    PhysicalDevice *suitable_device = find_suitable_physical_device(vk, discrete_devices, requested_features);

    if (suitable_device == NULL) {
        suitable_device = find_suitable_physical_device(vk, integrated_devices, requested_features);

        if (suitable_device == NULL)
            CTK_FATAL("failed to find any suitable device");
    }

    vk->device.physical = *suitable_device;
    ctk_pop_frame(vk->mem.temp);
}

static void init_logical_device(Vulkan *vk, CTK_Array<s32> *requested_features) {
    CTK_FixedArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    ctk_push(&queue_infos, vtk_default_queue_info(vk->device.physical.queue_family_indexes.graphics));

    // Don't create separate queues if present and vk belong to same queue family.
    if (vk->device.physical.queue_family_indexes.present != vk->device.physical.queue_family_indexes.graphics)
        ctk_push(&queue_infos, vtk_default_queue_info(vk->device.physical.queue_family_indexes.present));

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
    logical_device_info.enabledExtensionCount = CTK_ARRAY_SIZE(extensions);
    logical_device_info.ppEnabledExtensionNames = extensions;
    logical_device_info.pEnabledFeatures = (VkPhysicalDeviceFeatures *)enabled_features;
    vtk_validate_result(
        vkCreateDevice(vk->device.physical.handle, &logical_device_info, NULL, &vk->device.logical.handle),
        "failed to create logical device");

    // Get logical vk->device.logical queues.
    vkGetDeviceQueue(vk->device.logical.handle, vk->device.physical.queue_family_indexes.graphics, 0,
                     &vk->device.logical.queue.graphics);
    vkGetDeviceQueue(vk->device.logical.handle, vk->device.physical.queue_family_indexes.present, 0,
                     &vk->device.logical.queue.present);
}

static void init_swapchain(Vulkan *vk) {
    ctk_push_frame(vk->mem.temp);

    ////////////////////////////////////////////////////////////
    /// Configuration
    ////////////////////////////////////////////////////////////

    // Configure swapchain based on surface properties.
    auto surface_formats =
        vtk_load_vk_objects<VkSurfaceFormatKHR>(
            vk->mem.temp,
            vkGetPhysicalDeviceSurfaceFormatsKHR,
            vk->device.physical.handle,
            vk->surface);

    auto surface_present_modes =
        vtk_load_vk_objects<VkPresentModeKHR>(
            vk->mem.temp,
            vkGetPhysicalDeviceSurfacePresentModesKHR,
            vk->device.physical.handle,
            vk->surface);

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    vtk_validate_result(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->device.physical.handle, vk->surface, &surface_capabilities),
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
    u32 graphics_queue_family_index = vk->device.physical.queue_family_indexes.graphics;
    u32 present_queue_family_index = vk->device.physical.queue_family_indexes.present;
    u32 queue_family_indexes[] = {
        graphics_queue_family_index,
        present_queue_family_index,
    };

    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = vk->surface;
    info.flags = 0;
    info.minImageCount = selected_image_count;
    info.imageFormat = selected_format.format;
    info.imageColorSpace = selected_format.colorSpace;
    info.imageExtent = surface_capabilities.currentExtent;
    info.imageArrayLayers = 1; // Always 1 for standard images.
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.preTransform = surface_capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = selected_present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;
    if (graphics_queue_family_index != present_queue_family_index) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = CTK_ARRAY_SIZE(queue_family_indexes);
        info.pQueueFamilyIndices = queue_family_indexes;
    }
    else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = NULL;
    }
    vtk_validate_result(vkCreateSwapchainKHR(vk->device.logical.handle, &info, NULL, &vk->swapchain.handle),
                        "failed to create swapchain");

    // Store surface state used to create swapchain for future reference.
    vk->swapchain.image_format = selected_format.format;
    vk->swapchain.extent = surface_capabilities.currentExtent;

    ////////////////////////////////////////////////////////////
    /// Image View Creation
    ////////////////////////////////////////////////////////////
    auto swapchain_images =
        vtk_load_vk_objects<VkImage>(
            vk->mem.temp,
            vkGetSwapchainImagesKHR,
            vk->device.logical.handle,
            vk->swapchain.handle);

    CTK_ASSERT(swapchain_images->count <= ctk_size(&vk->swapchain.image_views));
    vk->swapchain.image_views.count = swapchain_images->count;

    for (u32 i = 0; i < swapchain_images->count; ++i) {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = swapchain_images->data[i];
        view_info.flags = 0;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = vk->swapchain.image_format;
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
            vkCreateImageView(vk->device.logical.handle, &view_info, NULL, vk->swapchain.image_views.data + i),
            "failed to create image view");
    }

    ctk_pop_frame(vk->mem.temp);
}

static void init_command_pool(Vulkan *vk) {
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = vk->device.physical.queue_family_indexes.graphics;
    vtk_validate_result(
        vkCreateCommandPool(vk->device.logical.handle, &command_pool_info, NULL, &vk->command_pool),
        "failed to create command pool");
}

static Vulkan *create_vulkan(CTK_Allocator *module_mem, Platform *platform, VulkanInfo info) {
    // Allocate memory for vk module.
    auto vk = ctk_alloc<Vulkan>(module_mem, 1);
    vk->mem.module = module_mem;
    vk->mem.temp = ctk_create_stack_allocator(module_mem, CTK_MEGABYTE);
    vk->pool.buffer = ctk_create_pool<Buffer>(vk->mem.module, info.max_buffers);
    vk->pool.region = ctk_create_pool<Region>(vk->mem.module, info.max_regions);

    ctk_push_frame(vk->mem.temp);

    // Initialization
    init_instance(vk);
    init_surface(vk, platform);

    // Physical/Logical Devices
    auto requested_features = ctk_create_array<s32>(vk->mem.temp, 2);
    ctk_push(requested_features, (s32)VTK_PHYSICAL_DEVICE_FEATURE_geometryShader);
    load_physical_device(vk, requested_features);
    init_logical_device(vk, requested_features);

    init_swapchain(vk);
    init_command_pool(vk);

    // Cleanup
    ctk_pop_frame(vk->mem.temp);

    return vk;
}

static VkDeviceMemory allocate_device_memory(Vulkan *vk, VkMemoryRequirements mem_reqs,
                                             VkMemoryPropertyFlags mem_property_flags) {
    // Allocate memory
    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = mem_reqs.size;
    info.memoryTypeIndex = vtk_find_memory_type_index(vk->device.physical.mem_properties, mem_reqs,
                                                      mem_property_flags);
    VkDeviceMemory mem = VK_NULL_HANDLE;
    vtk_validate_result(vkAllocateMemory(vk->device.logical.handle, &info, NULL, &mem), "failed to allocate memory");
    return mem;
}

static Buffer *create_buffer(Vulkan *vk, BufferInfo *buffer_info) {
    auto buffer = ctk_alloc(vk->pool.buffer);
    buffer->size = buffer_info->size;

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = buffer_info->size;
    info.usage = buffer_info->usage_flags;
    info.sharingMode = buffer_info->sharing_mode;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = NULL; // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
    vtk_validate_result(vkCreateBuffer(vk->device.logical.handle, &info, NULL, &buffer->handle),
                        "failed to create buffer");

    // Allocate / Bind Memory
    VkMemoryRequirements mem_reqs = {};
    vkGetBufferMemoryRequirements(vk->device.logical.handle, buffer->handle, &mem_reqs);
    buffer->mem = allocate_device_memory(vk, mem_reqs, buffer_info->mem_property_flags);
    vtk_validate_result(
        vkBindBufferMemory(vk->device.logical.handle, buffer->handle, buffer->mem, 0),
        "failed to bind buffer memory");

    return buffer;
}

static Region *allocate_region(Vulkan *vk, Buffer *buffer, u32 size, VkDeviceSize align = 1) {
    VkDeviceSize align_offset = buffer->end % align;

    auto region = ctk_alloc(vk->pool.region);
    region->buffer = buffer;
    region->offset = align_offset ? buffer->end - align_offset + align : buffer->end;

    if (region->offset + size > buffer->size) {
        CTK_FATAL("buffer (size=%u end=%u) cannot allocate region of size %u and alignment %u (only %u bytes left)",
                  buffer->size, buffer->end, size, align, buffer->size - buffer->end);
    }

    region->size = size;
    buffer->end = region->offset + region->size;

    return region;
}

static void write_to_host_region(Vulkan *vk, Region *region, void *data, u32 size) {
    CTK_ASSERT(size < region->size);
    void *mapped_mem = NULL;
    vkMapMemory(vk->device.logical.handle, region->buffer->mem, region->offset, size, 0, &mapped_mem);
    memcpy(mapped_mem, data, size);
    vkUnmapMemory(vk->device.logical.handle, region->buffer->mem);
}

static void write_to_device_region(Vulkan *vk, Region *region, Region *staging_region, void *data, u32 size) {
    write_to_host_region(vk, staging_region, data, size);


}

static RenderPass *create_render_pass(Vulkan *vk, RenderPassInfo *info) {
    ctk_push_frame(vk->mem.temp);

    auto render_pass = ctk_alloc<RenderPass>(vk->mem.module, 1);
    render_pass->clear_values = ctk_create_array<VkClearValue>(vk->mem.module, info->clear_values->count);
    render_pass->framebuffers = ctk_create_array<VkFramebuffer>(vk->mem.module, info->framebuffer_infos->count);

    // Clear Values
    ctk_concat(render_pass->clear_values, info->clear_values->data, info->clear_values->count);

    // Subpass Descriptions
    auto subpass_descriptions = ctk_create_array<VkSubpassDescription>(vk->mem.temp, info->subpass_infos->count);
    for (u32 i = 0; i < info->subpass_infos->count; ++i) {
        SubpassInfo *subpass_info = info->subpass_infos->data + i;
        VkSubpassDescription *description = ctk_push(subpass_descriptions);
        description->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        description->inputAttachmentCount =     subpass_info->input_attachment_references->count;
        description->pInputAttachments =        subpass_info->input_attachment_references->data;
        description->colorAttachmentCount =     subpass_info->color_attachment_references->count;
        description->pColorAttachments =        subpass_info->color_attachment_references->data;
        description->preserveAttachmentCount =  subpass_info->preserve_attachment_indexes->count;
        description->pPreserveAttachments =     subpass_info->preserve_attachment_indexes->data;
        description->pResolveAttachments =      NULL;
        description->pDepthStencilAttachment =  subpass_info->depth_attachment_reference;
    }

    // Render Pass
    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount =   info->attachment_descriptions->count;
    create_info.pAttachments =      info->attachment_descriptions->data;
    create_info.subpassCount =      subpass_descriptions->count;
    create_info.pSubpasses =        subpass_descriptions->data;
    create_info.dependencyCount =   info->subpass_dependencies->count;
    create_info.pDependencies =     info->subpass_dependencies->data;
    vtk_validate_result(vkCreateRenderPass(vk->device.logical.handle, &create_info, NULL, &render_pass->handle),
                        "failed to create render pass");

    // // Framebuffers
    // u32 framebuffer_count = info->framebuffer_infos->count;
    // for (u32 i = 0; i < framebuffer_count; ++i) {
    //     ctk_push(
    //         render_pass->framebuffers,
    //         vtk_create_framebuffer(vk->device.logical.handle, render_pass->handle, info->framebuffer_infos->data + i));
    // }

    // Cleanup
    ctk_pop_frame(vk->mem.temp);

    return render_pass;
}

////////////////////////////////////////////////////////////
/// Pipeline
////////////////////////////////////////////////////////////
// static PipelineInfo vtk_default_pipeline_info() {
//     PipelineInfo info = {};
//     info.input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     info.input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     info.input_assembly_state.primitiveRestartEnable = VK_FALSE;
//
//     info.depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//     // Depth
//     info.depth_stencil_state.depthTestEnable = VK_FALSE;
//     info.depth_stencil_state.depthWriteEnable = VK_FALSE;
//     info.depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;
//     info.depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
//     info.depth_stencil_state.minDepthBounds = 0.0f;
//     info.depth_stencil_state.maxDepthBounds = 1.0f;
//     // Stencil
//     info.depth_stencil_state.stencilTestEnable = VK_FALSE;
//     info.depth_stencil_state.front.compareOp = VK_COMPARE_OP_NEVER;
//     info.depth_stencil_state.front.passOp = VK_STENCIL_OP_KEEP;
//     info.depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
//     info.depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
//     info.depth_stencil_state.front.compareMask = 0xFF;
//     info.depth_stencil_state.front.writeMask = 0xFF;
//     info.depth_stencil_state.front.reference = 1;
//     info.depth_stencil_state.back = info.depth_stencil_state.front;
//
//     info.rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     info.rasterization_state.depthClampEnable = VK_FALSE; // Don't clamp fragments within depth range.
//     info.rasterization_state.rasterizerDiscardEnable = VK_FALSE;
//     info.rasterization_state.polygonMode = VK_POLYGON_MODE_FILL; // Only available mode on AMD gpus?
//     info.rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
//     info.rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//     info.rasterization_state.depthBiasEnable = VK_FALSE;
//     info.rasterization_state.depthBiasConstantFactor = 0.0f;
//     info.rasterization_state.depthBiasClamp = 0.0f;
//     info.rasterization_state.depthBiasSlopeFactor = 0.0f;
//     info.rasterization_state.lineWidth = 1.0f;
//
//     info.multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     info.multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//     info.multisample_state.sampleShadingEnable = VK_FALSE;
//     info.multisample_state.minSampleShading = 1.0f;
//     info.multisample_state.pSampleMask = NULL;
//     info.multisample_state.alphaToCoverageEnable = VK_FALSE;
//     info.multisample_state.alphaToOneEnable = VK_FALSE;
//
//     info.color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     info.color_blend_state.logicOpEnable = VK_FALSE;
//     info.color_blend_state.logicOp = VK_LOGIC_OP_COPY;
//     info.color_blend_state.attachmentCount = 0;
//     info.color_blend_state.pAttachments = NULL;
//     info.color_blend_state.blendConstants[0] = 1.0f;
//     info.color_blend_state.blendConstants[1] = 1.0f;
//     info.color_blend_state.blendConstants[2] = 1.0f;
//     info.color_blend_state.blendConstants[3] = 1.0f;
//     return info;
// }

// static VkPipelineColorBlendAttachmentState vtk_default_color_blend_attachment_state() {
//     VkPipelineColorBlendAttachmentState state = {};
//     state.blendEnable = VK_FALSE;
//     state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
//     state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
//     state.colorBlendOp = VK_BLEND_OP_ADD;
//     state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//     state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//     state.alphaBlendOp = VK_BLEND_OP_ADD;
//     state.colorWriteMask = COLOR_COMPONENT_RGBA;
//     return state;
// }

// static Pipeline vtk_create_graphics_pipeline(Vulkan *vk, VkDevice device, RenderPass *render_pass, u32 subpass_index,
//                                              PipelineInfo *info) {
//     ctk_push_frame(vk->mem.temp);
//     Pipeline pipeline = {};
//
//     // Shader Stages
//     auto shader_stages = ctk_create_array<VkPipelineShaderStageCreateInfo>(vk->mem.temp,
//                                                                            info->shaders.count);
//     for (u32 i = 0; i < info->shaders.count; ++i) {
//         Shader *shader = info->shaders[i];
//         VkPipelineShaderStageCreateInfo *shader_stage_ci = ctk_push(&shader_stages);
//         shader_stage_ci->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//         shader_stage_ci->flags = 0;
//         shader_stage_ci->stage = shader->stage;
//         shader_stage_ci->module = shader->handle;
//         shader_stage_ci->pName = "main";
//         shader_stage_ci->pSpecializationInfo = NULL;
//     }
//
//     VkPipelineLayoutCreateInfo layout_ci = {};
//     layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//     layout_ci.setLayoutCount = info->descriptor_set_layouts.count;
//     layout_ci.pSetLayouts = info->descriptor_set_layouts.data;
//     layout_ci.pushConstantRangeCount = info->push_constant_ranges.count;
//     layout_ci.pPushConstantRanges = info->push_constant_ranges.data;
//     vtk_validate_result(vkCreatePipelineLayout(device, &layout_ci, NULL, &pipeline.layout),
//                         "failed to create graphics pipeline layout");
//
//     // Vertex Attribute Descriptions
//     auto vertex_attribute_descriptions =
//         ctk_create_array<VkVertexInputAttributeDescription>(vk->mem.temp, info->vert_inputs->count);
//
//     for (u32 i = 0; i < info->vertex_inputs->count; ++i) {
//         VertexInput *vert_input = info->vertex_inputs->data + i;
//         VkVertexInputAttributeDescription *attrib_desc = ctk_push(&vertex_attribute_descriptions);
//         attrib_desc->location = vert_input->location;
//         attrib_desc->binding = vert_input->binding;
//         attrib_desc->format = vert_input->attribute->format;
//         attrib_desc->offset = vert_input->attribute->offset;
//     }
//
//     VkPipelineVertexInputStateCreateInfo vert_input_state = {};
//     vert_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//     vert_input_state.vertexBindingDescriptionCount = info->vertex_input_binding_descriptions.count;
//     vert_input_state.pVertexBindingDescriptions = info->vertex_input_binding_descriptions.data;
//     vert_input_state.vertexAttributeDescriptionCount = vertex_attribute_descriptions->count;
//     vert_input_state.pVertexAttributeDescriptions = vertex_attribute_descriptions->data;
//
//     // Viewport State
//     VkPipelineViewportStateCreateInfo viewport_state = {};
//     viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     bool dynamic_viewport = false;
//     bool dynamic_scissor = false;
//     for (u32 i = 0; i < info->dynamic_states.count; ++i) {
//         if (info->dynamic_states[i] == VK_DYNAMIC_STATE_VIEWPORT)
//             dynamic_viewport = true;
//
//         if (info->dynamic_states[i] == VK_DYNAMIC_STATE_SCISSOR)
//             dynamic_scissor = true;
//     }
//
//     if (dynamic_viewport) {
//         viewport_state.viewportCount = 1;
//         viewport_state.pViewports = NULL;
//     }
//     else {
//         viewport_state.viewportCount = info->viewports.count;
//         viewport_state.pViewports = info->viewports.data;
//     }
//
//     if (dynamic_scissor) {
//         viewport_state.scissorCount = 1;
//         viewport_state.pScissors = NULL;
//     }
//     else {
//         viewport_state.scissorCount = info->scissors.count;
//         viewport_state.pScissors = info->scissors.data;
//     }
//
//     info->color_blend_state.attachmentCount = info->color_blend_attachment_states.count;
//     info->color_blend_state.pAttachments = info->color_blend_attachment_states.data;
//
//     VkPipelineDynamicStateCreateInfo dynamic_state = {};
//     dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//     dynamic_state.dynamicStateCount = info->dynamic_states.count;
//     dynamic_state.pDynamicStates = info->dynamic_states.data;
//
//     VkPipelineCreateInfo create_info = {};
//     create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//     create_info.stageCount = shader_stages->count;
//     create_info.pStages = shader_stages->data;
//     create_info.pVertexInputState = &vert_input_state;
//     create_info.pInputAssemblyState = &info->input_assembly_state;
//     create_info.pTessellationState = NULL;
//     create_info.pViewportState = &viewport_state;
//     create_info.pRasterizationState = &info->rasterization_state;
//     create_info.pMultisampleState = &info->multisample_state;
//     create_info.pDepthStencilState = &info->depth_stencil_state;
//     create_info.pColorBlendState = &info->color_blend_state;
//     create_info.pDynamicState = &dynamic_state;
//     create_info.layout = pipeline.layout;
//     create_info.renderPass = render_pass->handle;
//     create_info.subpass = subpass_index;
//     create_info.basePipelineHandle = VK_NULL_HANDLE;
//     create_info.basePipelineIndex = -1;
//     vtk_validate_result(vkCreatePipelines(device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline.handle),
//                         "failed to create graphics pipeline");
//
//     // Cleanup
//     ctk_pop_frame(vk->mem.temp);
//
//     return pipeline;
// }
