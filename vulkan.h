#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "ctk/file.h"
#include "vtk/vtk.h"
#include "vtk/device_features.h"
#include "renderer/platform.h"

#define COLOR_COMPONENT_RGBA \
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT

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

struct AttachmentInfo {
    VkAttachmentDescription description;
    VkClearValue clear_value;
};

struct RenderPassInfo {
    struct {
        CTK_Array<VkAttachmentDescription> *descriptions;
        CTK_Array<VkClearValue> *clear_values;
    } attachment;

    struct {
        CTK_Array<SubpassInfo> *infos;
        CTK_Array<VkSubpassDependency> *dependencies;
    } subpass;
};

struct RenderPass {
    VkRenderPass handle;
    CTK_Array<VkClearValue> *attachment_clear_values;
};

struct FramebufferInfo {
    CTK_Array<VkImageView> *attachments;
    VkExtent2D extent;
    u32 layers;
};

struct Shader {
    VkShaderModule handle;
    VkShaderStageFlagBits stage;
};

struct DescriptorPoolInfo {
    struct {
        u32 uniform_buffer;
        u32 uniform_buffer_dynamic;
        u32 combined_image_sampler;
        u32 input_attachment;
    } descriptor_count;

    u32 max_descriptor_sets;
};

struct PipelineInfo {
    CTK_FixedArray<Shader *,   8> shaders;
    CTK_FixedArray<VkViewport, 4> viewports;
    CTK_FixedArray<VkRect2D,   4> scissors;
    CTK_FixedArray<VkPipelineColorBlendAttachmentState, 4> color_blend_attachments;

    CTK_Array<VkDescriptorSetLayout> *descriptor_set_layouts;
    CTK_Array<VkPushConstantRange>   *push_constant_ranges;

    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineDepthStencilStateCreateInfo  depth_stencil;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo   multisample;
    VkPipelineColorBlendStateCreateInfo    color_blend;
};

struct Pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
};

struct VulkanInfo {
    u32 max_buffers;
    u32 max_regions;
    u32 max_render_passes;
    u32 max_shaders;
    u32 max_pipelines;
};

struct Vulkan {
    // Memory
    struct {
        CTK_Allocator *module;
        CTK_Allocator *temp;
    } mem;

    struct {
        CTK_Pool<Buffer>     *buffer;
        CTK_Pool<Region>     *region;
        CTK_Pool<RenderPass> *render_pass;
        CTK_Pool<Shader>     *shader;
        CTK_Pool<Pipeline>   *pipeline;
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

    cstr extensions[] = {
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // Validation
    };

    cstr layers[] = {
        "VK_LAYER_KHRONOS_validation", // Validation
    };

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

    QueueFamilyIndexes queue_family_indexes = { .graphics = CTK_U32_MAX, .present = CTK_U32_MAX };
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
                                                     CTK_Array<s32> *requested_features)
{
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

static VkSurfaceCapabilitiesKHR get_surface_capabilities(Vulkan *vk) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    vtk_validate_result(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->device.physical.handle, vk->surface, &capabilities),
        "failed to get physical device surface capabilities");

    return capabilities;
}

static VkExtent2D get_surface_extent(Vulkan *vk) {
    return get_surface_capabilities(vk).currentExtent;
}

static void init_swapchain(Vulkan *vk) {
    ctk_push_frame(vk->mem.temp);
    VkSurfaceCapabilitiesKHR surface_capabilities = get_surface_capabilities(vk);

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
    vk->swapchain.image_count = swapchain_images->count;

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
    vk->pool.buffer      = ctk_create_pool<Buffer>      (vk->mem.module, info.max_buffers);
    vk->pool.region      = ctk_create_pool<Region>      (vk->mem.module, info.max_regions);
    vk->pool.render_pass = ctk_create_pool<RenderPass>  (vk->mem.module, info.max_render_passes);
    vk->pool.shader      = ctk_create_pool<Shader>      (vk->mem.module, info.max_shaders);
    vk->pool.pipeline    = ctk_create_pool<Pipeline>    (vk->mem.module, info.max_pipelines);

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

    ctk_pop_frame(vk->mem.temp);
    return vk;
}

////////////////////////////////////////////////////////////
/// Memory
////////////////////////////////////////////////////////////
static VkDeviceMemory
allocate_device_memory(Vulkan *vk, VkMemoryRequirements mem_reqs, VkMemoryPropertyFlags mem_property_flags) {
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

////////////////////////////////////////////////////////////
/// Resource Creation
////////////////////////////////////////////////////////////
static RenderPass *create_render_pass(Vulkan *vk, RenderPassInfo *info) {
    ctk_push_frame(vk->mem.temp);

    auto render_pass = ctk_alloc(vk->pool.render_pass);
    render_pass->attachment_clear_values =
        ctk_create_array<VkClearValue>(vk->mem.module, info->attachment.clear_values->count);

    // Clear Values
    ctk_concat(render_pass->attachment_clear_values, info->attachment.clear_values);

    // Subpass Descriptions
    auto subpass_descriptions = ctk_create_array<VkSubpassDescription>(vk->mem.temp, info->subpass.infos->count);
    for (u32 i = 0; i < info->subpass.infos->count; ++i) {
        SubpassInfo *subpass_info = info->subpass.infos->data + i;
        VkSubpassDescription *description = ctk_push(subpass_descriptions);
        description->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (subpass_info->input_attachment_references) {
            description->inputAttachmentCount = subpass_info->input_attachment_references->count;
            description->pInputAttachments = subpass_info->input_attachment_references->data;
        }

        if (subpass_info->color_attachment_references) {
            description->colorAttachmentCount = subpass_info->color_attachment_references->count;
            description->pColorAttachments = subpass_info->color_attachment_references->data;
        }

        if (subpass_info->preserve_attachment_indexes) {
            description->preserveAttachmentCount = subpass_info->preserve_attachment_indexes->count;
            description->pPreserveAttachments = subpass_info->preserve_attachment_indexes->data;
        }

        description->pResolveAttachments = NULL;
        description->pDepthStencilAttachment = subpass_info->depth_attachment_reference;
    }

    // Render Pass
    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount =   info->attachment.descriptions->count;
    create_info.pAttachments =      info->attachment.descriptions->data;
    create_info.subpassCount =      subpass_descriptions->count;
    create_info.pSubpasses =        subpass_descriptions->data;
    create_info.dependencyCount =   info->subpass.dependencies->count;
    create_info.pDependencies =     info->subpass.dependencies->data;
    vtk_validate_result(vkCreateRenderPass(vk->device.logical.handle, &create_info, NULL, &render_pass->handle),
                        "failed to create render pass");

    ctk_pop_frame(vk->mem.temp);
    return render_pass;
}

static VkFramebuffer create_framebuffer(VkDevice logical_device, VkRenderPass rp, FramebufferInfo *info) {
    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = rp;
    create_info.attachmentCount = info->attachments->count;
    create_info.pAttachments = info->attachments->data;
    create_info.width = info->extent.width;
    create_info.height = info->extent.height;
    create_info.layers = info->layers;
    VkFramebuffer fb = VK_NULL_HANDLE;
    vtk_validate_result(vkCreateFramebuffer(logical_device, &create_info, NULL, &fb), "failed to create framebuffer");
    return fb;
}

static Shader *create_shader(Vulkan *vk, cstr spirv_path, VkShaderStageFlagBits stage) {
    ctk_push_frame(vk->mem.temp);

    auto shader = ctk_alloc(vk->pool.shader);
    shader->stage = stage;

    CTK_Array<u8> *byte_code = ctk_read_file<u8>(vk->mem.temp, spirv_path);
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.flags = 0;
    info.codeSize = ctk_byte_size(byte_code);
    info.pCode = (u32 const *)byte_code->data;
    vtk_validate_result(vkCreateShaderModule(vk->device.logical.handle, &info, NULL, &shader->handle),
                        "failed to create shader from SPIR-V bytecode in \"%p\"", spirv_path);

    ctk_pop_frame(vk->mem.temp);
    return shader;
}

static VkDescriptorPool create_descriptor_pool(Vulkan *vk, DescriptorPoolInfo info) {
    CTK_FixedArray<VkDescriptorPoolSize, 4> pool_sizes = {};

    if (info.descriptor_count.uniform_buffer) {
        ctk_push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = info.descriptor_count.uniform_buffer,
        });
    }

    if (info.descriptor_count.uniform_buffer_dynamic) {
        ctk_push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = info.descriptor_count.uniform_buffer_dynamic,
        });
    }

    if (info.descriptor_count.combined_image_sampler) {
        ctk_push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = info.descriptor_count.combined_image_sampler,
        });
    }

    if (info.descriptor_count.input_attachment) {
        ctk_push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = info.descriptor_count.input_attachment,
        });
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = info.max_descriptor_sets;
    pool_info.poolSizeCount = pool_sizes.count;
    pool_info.pPoolSizes = pool_sizes.data;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    vtk_validate_result(vkCreateDescriptorPool(vk->device.logical.handle, &pool_info, NULL, &pool),
                        "failed to create descriptor pool");

    return pool;
}

static VkDescriptorSetLayout
create_descriptor_set_layout(VkDevice logical_device, VkDescriptorSetLayoutBinding *bindings, u32 binding_count) {
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = binding_count;
    info.pBindings = bindings;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    vtk_validate_result(vkCreateDescriptorSetLayout(logical_device, &info, NULL, &layout),
                        "error creating descriptor set layout");

    return layout;
}

static VkDescriptorSetLayout
create_descriptor_set_layout(VkDevice logical_device, CTK_Array<VkDescriptorSetLayoutBinding> *bindings) {
    return create_descriptor_set_layout(logical_device, bindings->data, bindings->count);
}

static PipelineInfo const DEFAULT_PIPELINE_INFO = {
    .input_assembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    },
    .depth_stencil = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front = {
            .failOp      = VK_STENCIL_OP_KEEP,
            .passOp      = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp   = VK_COMPARE_OP_NEVER,
            .compareMask = 0xFF,
            .writeMask   = 0xFF,
            .reference   = 1,
        },
        .back = {
            .failOp      = VK_STENCIL_OP_KEEP,
            .passOp      = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp   = VK_COMPARE_OP_NEVER,
            .compareMask = 0xFF,
            .writeMask   = 0xFF,
            .reference   = 1,
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    },
    .rasterization = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable        = VK_FALSE, // Don't clamp fragments within depth range.
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL, // Only available mode on AMD gpus?
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    },
    .multisample = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    },
    .color_blend = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 0,
        .pAttachments    = NULL,
        .blendConstants  = { 1.0f, 1.0f, 1.0f, 1.0f },
    },
};

static VkPipelineColorBlendAttachmentState const DEFAULT_COLOR_BLEND_ATTACHMENT = {
    .blendEnable         = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp        = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp        = VK_BLEND_OP_ADD,
    .colorWriteMask      = COLOR_COMPONENT_RGBA,
};

static Pipeline *create_pipeline(Vulkan *vk, RenderPass *render_pass, u32 subpass, PipelineInfo *info) {
    ctk_push_frame(vk->mem.temp);
    Pipeline *pipeline = ctk_alloc(vk->pool.pipeline);

    // Shader Stages
    auto shader_stages = ctk_create_array<VkPipelineShaderStageCreateInfo>(vk->mem.temp, info->shaders.count);
    for (u32 i = 0; i < info->shaders.count; ++i) {
        Shader *shader = info->shaders.data[i];
        VkPipelineShaderStageCreateInfo *shader_stage_info = ctk_push(shader_stages);
        shader_stage_info->sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info->flags  = 0;
        shader_stage_info->stage  = shader->stage;
        shader_stage_info->module = shader->handle;
        shader_stage_info->pName  = "main";
        shader_stage_info->pSpecializationInfo = NULL;
    }

    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (info->descriptor_set_layouts) {
        layout_ci.setLayoutCount = info->descriptor_set_layouts->count;
        layout_ci.pSetLayouts    = info->descriptor_set_layouts->data;
    }
    if (info->push_constant_ranges) {
        layout_ci.pushConstantRangeCount = info->push_constant_ranges->count;
        layout_ci.pPushConstantRanges    = info->push_constant_ranges->data;
    }
    vtk_validate_result(vkCreatePipelineLayout(vk->device.logical.handle, &layout_ci, NULL, &pipeline->layout),
                        "failed to create graphics pipeline layout");

    // Vertex Attribute Descriptions
    // auto vert_attrib_descs =
    //     ctk_create_array<VkVertexInputAttributeDescription>(vk->mem.temp, info->vertex_inputs->count);

    // for (u32 i = 0; i < info->vertex_inputs->count; ++i) {
    //     VertexInput *vert_input = info->vertex_inputs->data + i;
    //     ctk_push(vert_attrib_descs, {
    //         .location = vert_input->location,
    //         .binding  = vert_input->binding,
    //         .format   = vert_input->attribute->format,
    //         .offset   = vert_input->attribute->offset,
    //     });
    // }

    VkVertexInputBindingDescription vert_input_binding_descs[] = {
        {
            .binding   = 0,
            .stride    = 12,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    VkVertexInputAttributeDescription vert_attrib_descs[] = {
        {
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0,
        },
    };

    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount   = CTK_ARRAY_SIZE(vert_input_binding_descs);
    vertex_input.pVertexBindingDescriptions      = vert_input_binding_descs;
    vertex_input.vertexAttributeDescriptionCount = CTK_ARRAY_SIZE(vert_attrib_descs);
    vertex_input.pVertexAttributeDescriptions    = vert_attrib_descs;

    // // Viewport State
    // VkPipelineViewportStateCreateInfo viewport_state = {};
    // viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // bool dynamic_viewport = false;
    // bool dynamic_scissor = false;
    // for (u32 i = 0; i < info->dynamic_states.count; ++i) {
    //     if (info->dynamic_states[i] == VK_DYNAMIC_STATE_VIEWPORT)
    //         dynamic_viewport = true;

    //     if (info->dynamic_states[i] == VK_DYNAMIC_STATE_SCISSOR)
    //         dynamic_scissor = true;
    // }

    // if (dynamic_viewport) {
    //     viewport_state.viewportCount = 1;
    //     viewport_state.pViewports = NULL;
    // }
    // else {
    //     viewport_state.viewportCount = info->viewports.count;
    //     viewport_state.pViewports = info->viewports.data;
    // }

    // if (dynamic_scissor) {
    //     viewport_state.scissorCount = 1;
    //     viewport_state.pScissors = NULL;
    // }
    // else {
    //     viewport_state.scissorCount = info->scissors.count;
    //     viewport_state.pScissors = info->scissors.data;
    // }

    VkPipelineViewportStateCreateInfo viewport = {};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = info->viewports.count;
    viewport.pViewports    = info->viewports.data;
    viewport.scissorCount  = info->scissors.count;
    viewport.pScissors     = info->scissors.data;

    // VkPipelineDynamicStateCreateInfo dynamic_state = {};
    // dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamic_state.dynamicStateCount = info->dynamic_states.count;
    // dynamic_state.pDynamicStates = info->dynamic_states.data;

    // Reference attachment array in color_blend struct.
    info->color_blend.attachmentCount = info->color_blend_attachments.count;
    info->color_blend.pAttachments    = info->color_blend_attachments.data;

    VkGraphicsPipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount          = shader_stages->count;
    create_info.pStages             = shader_stages->data;
    create_info.pVertexInputState   = &vertex_input;
    create_info.pInputAssemblyState = &info->input_assembly;
    create_info.pTessellationState  = NULL;
    create_info.pViewportState      = &viewport;
    create_info.pRasterizationState = &info->rasterization;
    create_info.pMultisampleState   = &info->multisample;
    create_info.pDepthStencilState  = &info->depth_stencil;
    create_info.pColorBlendState    = &info->color_blend;
    create_info.pDynamicState       = NULL;//&dynamic;
    create_info.layout              = pipeline->layout;
    create_info.renderPass          = render_pass->handle;
    create_info.subpass             = subpass;
    create_info.basePipelineHandle  = VK_NULL_HANDLE;
    create_info.basePipelineIndex   = -1;
    vtk_validate_result(
        vkCreateGraphicsPipelines(vk->device.logical.handle, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline->handle),
        "failed to create graphics pipeline");

    // Cleanup
    ctk_pop_frame(vk->mem.temp);

    return pipeline;
}
