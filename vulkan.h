#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"
#include "ctk/memory.h"
#include "ctk/containers.h"
#include "ctk/file.h"
#include "renderer/vulkan_debug.h"
#include "renderer/vulkan_device_features.h"
#include "renderer/platform.h"

using namespace ctk;

////////////////////////////////////////////////////////////
/// Macros
////////////////////////////////////////////////////////////
#define LOAD_INSTANCE_EXTENSION_FUNCTION(INSTANCE, FUNC_NAME)\
    auto FUNC_NAME = (PFN_ ## FUNC_NAME)vkGetInstanceProcAddr(INSTANCE, #FUNC_NAME);\
    if (FUNC_NAME == NULL)\
        CTK_FATAL("failed to load instance extension function \"%s\"", #FUNC_NAME)

#define COLOR_COMPONENT_RGBA \
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
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
    QueueFamilyIndexes queue_family_idxs;

    VkPhysicalDeviceType type;
    u32 min_uniform_buffer_offset_alignment;
    u32 max_push_constant_size;

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties mem_properties;
    VkFormat depth_image_format;
};

struct Swapchain {
    VkSwapchainKHR handle;
    FixedArray<VkImageView, 4> image_views;
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

struct ImageInfo {
    VkImageCreateInfo image;
    VkImageViewCreateInfo view;
    VkMemoryPropertyFlagBits mem_property_flags;
};

struct Image {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory mem;
    VkExtent3D extent;
};

struct ImageSampler {
    Image *image;
    VkSampler sampler;
};

struct SubpassInfo {
    Array<u32> *preserve_attachment_indexes;
    Array<VkAttachmentReference> *input_attachment_refs;
    Array<VkAttachmentReference> *color_attachment_refs;
    VkAttachmentReference depth_attachment_ref;
};

struct AttachmentInfo {
    VkAttachmentDescription description;
    VkClearValue clear_value;
};

struct RenderPassInfo {
    struct {
        Array<VkAttachmentDescription> *descriptions;
        Array<VkClearValue> *clear_values;
    } attachment;

    struct {
        Array<SubpassInfo> *infos;
        Array<VkSubpassDependency> *dependencies;
    } subpass;
};

struct RenderPass {
    VkRenderPass handle;
    Array<VkClearValue> *attachment_clear_values;
};

struct FramebufferInfo {
    Array<VkImageView> *attachments;
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

struct DescriptorInfo {
    u32 count;
    VkDescriptorType type;
    VkShaderStageFlags stage;
};

struct DescriptorBinding {
    VkDescriptorType type;
    union {
        Region *uniform_buffer;
        ImageSampler *image_sampler;
    };
};

struct PipelineInfo {
    FixedArray<Shader *, 8> shaders;
    FixedArray<VkPipelineColorBlendAttachmentState, 8> color_blend_attachments;

    Array<VkDescriptorSetLayout> *descriptor_set_layouts;
    Array<VkPushConstantRange> *push_constant_ranges;
    Array<VkVertexInputBindingDescription> *vertex_bindings;
    Array<VkVertexInputAttributeDescription> *vertex_attributes;
    Array<VkViewport> *viewports;
    Array<VkRect2D> *scissors;

    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineRasterizationStateCreateInfo rasterization;
    VkPipelineMultisampleStateCreateInfo multisample;
    VkPipelineColorBlendStateCreateInfo color_blend;
};

struct Pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
};

struct VulkanInfo {
    u32 max_buffers;
    u32 max_regions;
    u32 max_images;
    u32 max_samplers;
    u32 max_render_passes;
    u32 max_shaders;
    u32 max_pipelines;
    bool enable_validation;
};

struct Vulkan {
    // Memory
    struct {
        Allocator *module;
        Allocator *temp;
    } mem;

    struct {
        Pool<Buffer> *buffer;
        Pool<Region> *region;
        Pool<Image> *image;
        Pool<RenderPass> *render_pass;
        Pool<Shader> *shader;
        Pool<Pipeline> *pipeline;
    } pool;

    // State
    Instance instance;
    VkSurfaceKHR surface;

    PhysicalDevice physical_device;
    VkDevice device;

    struct {
        VkQueue graphics;
        VkQueue present;
    } queue;

    Swapchain swapchain;
};

////////////////////////////////////////////////////////////
/// Utils
////////////////////////////////////////////////////////////
template<typename Object, typename Loader, typename ...Args>
static Array<Object> *load_vk_objects(Allocator *allocator, Loader loader, Args... args) {
    u32 count = 0;
    loader(args..., &count, NULL);
    CTK_ASSERT(count > 0);
    auto vk_objects = create_array_full<Object>(allocator, count, 0);
    loader(args..., &vk_objects->count, vk_objects->data);
    return vk_objects;
}

static VkFormat find_depth_image_format(VkPhysicalDevice physical_device) {
    static constexpr VkFormat DEPTH_IMAGE_FORMATS[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    static constexpr VkFormatFeatureFlags DEPTH_IMG_FMT_FEATS = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    // Find format that supports depth-stencil attachment feature for physical device.
    for (u32 i = 0; i < CTK_ARRAY_SIZE(DEPTH_IMAGE_FORMATS); i++) {
        VkFormat depth_img_fmt = DEPTH_IMAGE_FORMATS[i];
        VkFormatProperties depth_img_fmt_props = {};
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_img_fmt, &depth_img_fmt_props);

        if ((depth_img_fmt_props.optimalTilingFeatures & DEPTH_IMG_FMT_FEATS) == DEPTH_IMG_FMT_FEATS)
            return depth_img_fmt;
    }

    CTK_FATAL("failed to find physical device depth format that supports the depth-stencil attachment feature")
}

static VkDeviceQueueCreateInfo default_queue_info(u32 queue_fam_idx) {
    static f32 const QUEUE_PRIORITIES[] = { 1.0f };

    VkDeviceQueueCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    info.flags = 0;
    info.queueFamilyIndex = queue_fam_idx;
    info.queueCount = CTK_ARRAY_SIZE(QUEUE_PRIORITIES);
    info.pQueuePriorities = QUEUE_PRIORITIES;

    return info;
}

static u32 find_memory_type_index(VkPhysicalDeviceMemoryProperties mem_props, VkMemoryRequirements mem_reqs,
                                  VkMemoryPropertyFlags mem_prop_flags)
{
    // Find memory type index from device based on memory property flags.
    for (u32 mem_type_idx = 0; mem_type_idx < mem_props.memoryTypeCount; ++mem_type_idx) {
        // Ensure index refers to memory type from memory requirements.
        if (!(mem_reqs.memoryTypeBits & (1 << mem_type_idx)))
            continue;

        // Check if memory at index has correct properties.
        if ((mem_props.memoryTypes[mem_type_idx].propertyFlags & mem_prop_flags) == mem_prop_flags)
            return mem_type_idx;
    }

    CTK_FATAL("failed to find memory type that satisfies property requirements");
}

////////////////////////////////////////////////////////////
/// Initialization
////////////////////////////////////////////////////////////
static void init_instance(Vulkan *vk, bool enable_validation) {
    Instance *instance = &vk->instance;

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = {};
    if (enable_validation) {
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
        debug_messenger_info.pfnUserCallback = debug_callback;
        debug_messenger_info.pUserData = NULL;
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "renderer";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "renderer";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    FixedArray<cstr, 16> extensions = {};
    push(&extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    push(&extensions, VK_KHR_SURFACE_EXTENSION_NAME);
    if (enable_validation)
        push(&extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Validation

    FixedArray<cstr, 16> layers = {};
    if (enable_validation)
        push(&layers, "VK_LAYER_KHRONOS_validation"); // Validation

    VkInstanceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pNext = enable_validation ? &debug_messenger_info : NULL;
    info.flags = 0;
    info.pApplicationInfo = &app_info;
    info.enabledLayerCount = layers.count;
    info.ppEnabledLayerNames = layers.data;
    info.enabledExtensionCount = extensions.count;
    info.ppEnabledExtensionNames = extensions.data;
    validate_result(vkCreateInstance(&info, NULL, &instance->handle), "failed to create Vulkan instance");

    if (enable_validation) {
        LOAD_INSTANCE_EXTENSION_FUNCTION(instance->handle, vkCreateDebugUtilsMessengerEXT);
        validate_result(
            vkCreateDebugUtilsMessengerEXT(instance->handle, &debug_messenger_info, NULL, &instance->debug_messenger),
            "failed to create debug messenger");
    }
}

static void init_surface(Vulkan *vk, Platform *platform) {
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hwnd = platform->window->handle;
    info.hinstance = platform->instance;
    validate_result(vkCreateWin32SurfaceKHR(vk->instance.handle, &info, nullptr, &vk->surface),
                    "failed to get win32 surface");
}

static QueueFamilyIndexes find_queue_family_idxs(Vulkan *vk, VkPhysicalDevice physical_device) {
    push_frame(vk->mem.temp);

    QueueFamilyIndexes queue_family_idxs = { .graphics = U32_MAX, .present = U32_MAX };
    auto queue_family_props_array =
        load_vk_objects<VkQueueFamilyProperties>(vk->mem.temp, vkGetPhysicalDeviceQueueFamilyProperties,
                                                 physical_device);

    for (u32 queue_family_idx = 0; queue_family_idx < queue_family_props_array->count; ++queue_family_idx) {
        VkQueueFamilyProperties *queue_family_props = queue_family_props_array->data + queue_family_idx;

        if (queue_family_props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            queue_family_idxs.graphics = queue_family_idx;

        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_idx, vk->surface, &present_supported);

        if (present_supported == VK_TRUE)
            queue_family_idxs.present = queue_family_idx;
    }

    pop_frame(vk->mem.temp);
    return queue_family_idxs;
}

static PhysicalDevice *find_suitable_physical_device(Vulkan *vk, Array<PhysicalDevice *> *physical_devices,
                                                     PhysicalDeviceFeature *requested_features,
                                                     u32 requested_feature_count)
{
    push_frame(vk->mem.temp);

    Array<PhysicalDeviceFeature> *unsupported_features =
        requested_features
        ? create_array<PhysicalDeviceFeature>(vk->mem.temp, requested_feature_count)
        : NULL;

    PhysicalDevice *suitable_device = NULL;
    for (u32 i = 0; suitable_device == NULL && i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data[i];

        // Check for queue families that support vk and present.
        bool has_required_queue_families = physical_device->queue_family_idxs.graphics != U32_MAX &&
                                           physical_device->queue_family_idxs.present  != U32_MAX;

        bool requested_features_supported = true;
        if (requested_features) {
            clear(unsupported_features);

            // Check that all requested features are supported.
            for (u32 feat_index = 0; feat_index < requested_feature_count; ++feat_index) {
                PhysicalDeviceFeature requested_feature = requested_features[feat_index];

                if (!physical_device_feature_supported(requested_feature, &physical_device->features))
                    push(unsupported_features, requested_feature);
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

    pop_frame(vk->mem.temp);
    return suitable_device;
}

static void load_physical_device(Vulkan *vk, PhysicalDeviceFeature *requested_features, u32 requested_feature_count) {
    push_frame(vk->mem.temp);

    // Load info about all physical devices.
    auto vk_physical_devices = load_vk_objects<VkPhysicalDevice>(vk->mem.temp, vkEnumeratePhysicalDevices,
                                                                 vk->instance.handle);

    auto physical_devices = create_array<PhysicalDevice>(vk->mem.temp, vk_physical_devices->count);

    for (u32 i = 0; i < vk_physical_devices->count; ++i) {
        VkPhysicalDevice vk_physical_device = vk_physical_devices->data[i];
        PhysicalDevice *physical_device = push(physical_devices);
        physical_device->handle = vk_physical_device;
        physical_device->queue_family_idxs = find_queue_family_idxs(vk, vk_physical_device);

        // Load properties for future reference.
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(vk_physical_device, &properties);
        physical_device->type = properties.deviceType;
        physical_device->min_uniform_buffer_offset_alignment = properties.limits.minUniformBufferOffsetAlignment;
        physical_device->max_push_constant_size = properties.limits.maxPushConstantsSize;


        vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device->features);
        vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &physical_device->mem_properties);
        physical_device->depth_image_format = find_depth_image_format(physical_device->handle);
    }

    // Sort out discrete and integrated gpus.
    auto discrete_devices = create_array<PhysicalDevice *>(vk->mem.temp, physical_devices->count);
    auto integrated_devices = create_array<PhysicalDevice *>(vk->mem.temp, physical_devices->count);

    for (u32 i = 0; i < physical_devices->count; ++i) {
        PhysicalDevice *physical_device = physical_devices->data + i;

        if (physical_device->type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            push(discrete_devices, physical_device);
        else if (physical_device->type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            push(integrated_devices, physical_device);
    }

    // Find suitable discrete device, or fallback to an integrated device.
    PhysicalDevice *suitable_device = find_suitable_physical_device(vk, discrete_devices, requested_features,
                                                                    requested_feature_count);

    if (suitable_device == NULL) {
        suitable_device = find_suitable_physical_device(vk, integrated_devices, requested_features,
                                                        requested_feature_count);

        if (suitable_device == NULL)
            CTK_FATAL("failed to find any suitable device");
    }

    vk->physical_device = *suitable_device;

    pop_frame(vk->mem.temp);
}

static void init_device(Vulkan *vk, PhysicalDeviceFeature *requested_features, u32 requested_feature_count) {
    FixedArray<VkDeviceQueueCreateInfo, 2> queue_infos = {};
    push(&queue_infos, default_queue_info(vk->physical_device.queue_family_idxs.graphics));

    // Don't create separate queues if present and vk belong to same queue family.
    if (vk->physical_device.queue_family_idxs.present != vk->physical_device.queue_family_idxs.graphics)
        push(&queue_infos, default_queue_info(vk->physical_device.queue_family_idxs.present));

    cstr extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkBool32 enabled_features[(s32)PhysicalDeviceFeature::COUNT] = {};

    for (u32 i = 0; i < requested_feature_count; ++i)
        enabled_features[(s32)requested_features[i]] = VK_TRUE;

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
    validate_result(vkCreateDevice(vk->physical_device.handle, &logical_device_info, NULL, &vk->device),
                    "failed to create logical device");
}

static void init_queues(Vulkan *vk) {
    // Get logical vk->device.logical queues.
    vkGetDeviceQueue(vk->device, vk->physical_device.queue_family_idxs.graphics, 0, &vk->queue.graphics);
    vkGetDeviceQueue(vk->device, vk->physical_device.queue_family_idxs.present,  0, &vk->queue.present);
}

static VkSurfaceCapabilitiesKHR get_surface_capabilities(Vulkan *vk) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    validate_result(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk->physical_device.handle, vk->surface, &capabilities),
                    "failed to get physical device surface capabilities");

    return capabilities;
}

static VkExtent2D get_surface_extent(Vulkan *vk) {
    return get_surface_capabilities(vk).currentExtent;
}

static void init_swapchain(Vulkan *vk) {
    push_frame(vk->mem.temp);
    VkSurfaceCapabilitiesKHR surface_capabilities = get_surface_capabilities(vk);

    ////////////////////////////////////////////////////////////
    /// Configuration
    ////////////////////////////////////////////////////////////

    // Configure swapchain based on surface properties.
    auto surface_formats =
        load_vk_objects<VkSurfaceFormatKHR>(
            vk->mem.temp,
            vkGetPhysicalDeviceSurfaceFormatsKHR,
            vk->physical_device.handle,
            vk->surface);

    auto surface_present_modes =
        load_vk_objects<VkPresentModeKHR>(
            vk->mem.temp,
            vkGetPhysicalDeviceSurfacePresentModesKHR,
            vk->physical_device.handle,
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
    u32 graphics_queue_family_idx = vk->physical_device.queue_family_idxs.graphics;
    u32 present_queue_family_idx = vk->physical_device.queue_family_idxs.present;
    u32 queue_family_idxs[] = {
        graphics_queue_family_idx,
        present_queue_family_idx,
    };

    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.flags = 0;
    info.surface = vk->surface;
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
    if (graphics_queue_family_idx != present_queue_family_idx) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = CTK_ARRAY_SIZE(queue_family_idxs);
        info.pQueueFamilyIndices = queue_family_idxs;
    }
    else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = NULL;
    }
    validate_result(vkCreateSwapchainKHR(vk->device, &info, NULL, &vk->swapchain.handle), "failed to create swapchain");

    // Store surface state used to create swapchain for future reference.
    vk->swapchain.image_format = selected_format.format;
    vk->swapchain.extent = surface_capabilities.currentExtent;

    ////////////////////////////////////////////////////////////
    /// Image View Creation
    ////////////////////////////////////////////////////////////
    auto swap_imgs = load_vk_objects<VkImage>(vk->mem.temp, vkGetSwapchainImagesKHR, vk->device, vk->swapchain.handle);

    CTK_ASSERT(swap_imgs->count <= get_size(&vk->swapchain.image_views));
    vk->swapchain.image_views.count = swap_imgs->count;
    vk->swapchain.image_count = swap_imgs->count;

    for (u32 i = 0; i < swap_imgs->count; ++i) {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.flags = 0;
        view_info.image = swap_imgs->data[i];
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
        validate_result(vkCreateImageView(vk->device, &view_info, NULL, vk->swapchain.image_views.data + i),
                        "failed to create image view");
    }

    pop_frame(vk->mem.temp);
}

static VkCommandPool create_cmd_pool(Vulkan *vk) {
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = vk->physical_device.queue_family_idxs.graphics;

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    validate_result(vkCreateCommandPool(vk->device, &info, NULL, &cmd_pool), "failed to create command pool");

    return cmd_pool;
}

static Vulkan *create_vulkan(Allocator *module_mem, Platform *platform, VulkanInfo info) {
    // Allocate memory for vk module.s
    auto vk = allocate<Vulkan>(module_mem, 1);
    vk->mem.module = module_mem;
    vk->mem.temp = create_stack_allocator(module_mem, megabyte(1));
    vk->pool.buffer = create_pool<Buffer>(vk->mem.module, info.max_buffers);
    vk->pool.region = create_pool<Region>(vk->mem.module, info.max_regions);
    vk->pool.image = create_pool<Image>(vk->mem.module, info.max_images);
    vk->pool.render_pass = create_pool<RenderPass>(vk->mem.module, info.max_render_passes);
    vk->pool.shader = create_pool<Shader>(vk->mem.module, info.max_shaders);
    vk->pool.pipeline = create_pool<Pipeline>(vk->mem.module, info.max_pipelines);

    // Initialization
    init_instance(vk, info.enable_validation);
    init_surface(vk, platform);

    // Physical/Logical Devices
    auto requested_feature = PhysicalDeviceFeature::geometryShader;
    load_physical_device(vk, &requested_feature, 1);
    init_device(vk, &requested_feature, 1);
    init_queues(vk);

    init_swapchain(vk);

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
    info.memoryTypeIndex = find_memory_type_index(vk->physical_device.mem_properties, mem_reqs, mem_property_flags);
    VkDeviceMemory mem = VK_NULL_HANDLE;
    validate_result(vkAllocateMemory(vk->device, &info, NULL, &mem), "failed to allocate memory");
    return mem;
}

static Buffer *create_buffer(Vulkan *vk, BufferInfo *buffer_info) {
    auto buffer = allocate(vk->pool.buffer);
    buffer->size = buffer_info->size;

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = buffer_info->size;
    info.usage = buffer_info->usage_flags;
    info.sharingMode = buffer_info->sharing_mode;
    info.queueFamilyIndexCount = 0;
    info.pQueueFamilyIndices = NULL; // Ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT.
    validate_result(vkCreateBuffer(vk->device, &info, NULL, &buffer->handle), "failed to create buffer");

    // Allocate / Bind Memory
    VkMemoryRequirements mem_reqs = {};
    vkGetBufferMemoryRequirements(vk->device, buffer->handle, &mem_reqs);
    buffer->mem = allocate_device_memory(vk, mem_reqs, buffer_info->mem_property_flags);
    validate_result(vkBindBufferMemory(vk->device, buffer->handle, buffer->mem, 0), "failed to bind buffer memory");

    return buffer;
}

static Region *allocate_region(Vulkan *vk, Buffer *buffer, u32 size, VkDeviceSize align) {
    VkDeviceSize align_offset = buffer->end % align;

    auto region = allocate(vk->pool.region);
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

static Region *allocate_uniform_buffer_region(Vulkan *vk, Buffer *buffer, u32 size) {
    return allocate_region(vk, buffer, size, vk->physical_device.min_uniform_buffer_offset_alignment);
}

static void write_to_host_region(VkDevice device, Region *region, u32 offset, void *data, u32 size) {
    CTK_ASSERT(size <= region->size);
    void *mapped_mem = NULL;
    vkMapMemory(device, region->buffer->mem, region->offset + offset, size, 0, &mapped_mem);
    memcpy(mapped_mem, data, size);
    vkUnmapMemory(device, region->buffer->mem);
}

static void write_to_device_region(Vulkan *vk, VkCommandBuffer cmd_buf,
                                   Region *staging_region, u32 staging_offset,
                                   Region *region, u32 offset,
                                   void *data, u32 size)
{
    write_to_host_region(vk->device, staging_region, staging_offset, data, size);

    VkBufferCopy copy = {};
    copy.srcOffset = staging_region->offset + staging_offset;
    copy.dstOffset = region->offset + offset;
    copy.size = size;

    vkCmdCopyBuffer(cmd_buf, staging_region->buffer->handle, region->buffer->handle, 1, &copy);
}

static Image *create_image(Vulkan *vk, ImageInfo info) {
    Image *image = allocate(vk->pool.image);
    validate_result(vkCreateImage(vk->device, &info.image, NULL, &image->handle), "failed to create image");

    image->extent = info.image.extent;

    // Allocate / Bind Memory
    VkMemoryRequirements mem_reqs = {};
    vkGetImageMemoryRequirements(vk->device, image->handle, &mem_reqs);
    image->mem = allocate_device_memory(vk, mem_reqs, info.mem_property_flags);
    validate_result(vkBindImageMemory(vk->device, image->handle, image->mem, 0), "failed to bind image memory");

    info.view.image = image->handle;
    validate_result(vkCreateImageView(vk->device, &info.view, NULL, &image->view), "failed to create image view");

    return image;
}

static void write_to_image(Vulkan *vk, VkCommandBuffer cmd_buf, Region *region, u32 offset, Image *image) {
    VkImageMemoryBarrier pre_mem_barrier = {};
    pre_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    pre_mem_barrier.srcAccessMask = 0;
    pre_mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    pre_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    pre_mem_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    pre_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    pre_mem_barrier.image = image->handle;
    pre_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    pre_mem_barrier.subresourceRange.baseMipLevel = 0;
    pre_mem_barrier.subresourceRange.levelCount = 1;
    pre_mem_barrier.subresourceRange.baseArrayLayer = 0;
    pre_mem_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd_buf,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, // Dependency Flags
                         0, NULL, // Memory Barriers
                         0, NULL, // Buffer Memory Barriers
                         1, &pre_mem_barrier); // Image Memory Barriers

    VkBufferImageCopy copy = {};
    copy.bufferOffset = offset;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset.x = 0;
    copy.imageOffset.y = 0;
    copy.imageOffset.z = 0;
    copy.imageExtent = image->extent;
    vkCmdCopyBufferToImage(cmd_buf, region->buffer->handle, image->handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    VkImageMemoryBarrier post_mem_barrier = {};
    post_mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    post_mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    post_mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    post_mem_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    post_mem_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    post_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    post_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    post_mem_barrier.image = image->handle;
    post_mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    post_mem_barrier.subresourceRange.baseMipLevel = 0;
    post_mem_barrier.subresourceRange.levelCount = 1;
    post_mem_barrier.subresourceRange.baseArrayLayer = 0;
    post_mem_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd_buf,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, // Dependency Flags
                         0, NULL, // Memory Barriers
                         0, NULL, // Buffer Memory Barriers
                         1, &post_mem_barrier); // Image Memory Barriers
}

static VkSampler create_sampler(VkDevice device, VkSamplerCreateInfo info) {
    VkSampler sampler = VK_NULL_HANDLE;
    validate_result(vkCreateSampler(device, &info, NULL, &sampler), "failed to create sampler");
    return sampler;
}

////////////////////////////////////////////////////////////
/// Resource Creation
////////////////////////////////////////////////////////////
static RenderPass *create_render_pass(Vulkan *vk, RenderPassInfo *info) {
    push_frame(vk->mem.temp);

    auto render_pass = allocate(vk->pool.render_pass);
    render_pass->attachment_clear_values =
        create_array<VkClearValue>(vk->mem.module, info->attachment.clear_values->count);

    // Clear Values
    concat(render_pass->attachment_clear_values, info->attachment.clear_values);

    // Subpass Descriptions
    auto subpass_descriptions = create_array<VkSubpassDescription>(vk->mem.temp, info->subpass.infos->count);
    for (u32 i = 0; i < info->subpass.infos->count; ++i) {
        SubpassInfo *subpass_info = info->subpass.infos->data + i;
        VkSubpassDescription *description = push(subpass_descriptions);
        description->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (subpass_info->input_attachment_refs) {
            description->inputAttachmentCount = subpass_info->input_attachment_refs->count;
            description->pInputAttachments = subpass_info->input_attachment_refs->data;
        }

        if (subpass_info->color_attachment_refs) {
            description->colorAttachmentCount = subpass_info->color_attachment_refs->count;
            description->pColorAttachments = subpass_info->color_attachment_refs->data;
        }

        if (subpass_info->preserve_attachment_indexes) {
            description->preserveAttachmentCount = subpass_info->preserve_attachment_indexes->count;
            description->pPreserveAttachments = subpass_info->preserve_attachment_indexes->data;
        }

        description->pResolveAttachments = NULL;
        description->pDepthStencilAttachment = &subpass_info->depth_attachment_ref;
    }

    // Render Pass
    VkRenderPassCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = info->attachment.descriptions->count;
    create_info.pAttachments = info->attachment.descriptions->data;
    create_info.subpassCount = subpass_descriptions->count;
    create_info.pSubpasses = subpass_descriptions->data;
    create_info.dependencyCount = info->subpass.dependencies->count;
    create_info.pDependencies = info->subpass.dependencies->data;
    validate_result(vkCreateRenderPass(vk->device, &create_info, NULL, &render_pass->handle),
                    "failed to create render pass");

    pop_frame(vk->mem.temp);
    return render_pass;
}

static Shader *create_shader(Vulkan *vk, cstr spirv_path, VkShaderStageFlagBits stage) {
    push_frame(vk->mem.temp);

    auto shader = allocate(vk->pool.shader);
    shader->stage = stage;

    Array<u8> *bytecode = read_file<u8>(vk->mem.temp, spirv_path);
    if (bytecode == NULL)
        CTK_FATAL("failed to load bytecode from \"%s\"", spirv_path);

    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.flags = 0;
    info.codeSize = byte_size(bytecode);
    info.pCode = (const u32 *)bytecode->data;
    validate_result(vkCreateShaderModule(vk->device, &info, NULL, &shader->handle),
                    "failed to create shader from SPIR-V bytecode in \"%p\"", spirv_path);

    pop_frame(vk->mem.temp);
    return shader;
}

static VkDescriptorPool create_descriptor_pool(Vulkan *vk, DescriptorPoolInfo info) {
    FixedArray<VkDescriptorPoolSize, 4> pool_sizes = {};

    if (info.descriptor_count.uniform_buffer) {
        push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = info.descriptor_count.uniform_buffer,
        });
    }

    if (info.descriptor_count.uniform_buffer_dynamic) {
        push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = info.descriptor_count.uniform_buffer_dynamic,
        });
    }

    if (info.descriptor_count.combined_image_sampler) {
        push(&pool_sizes, {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = info.descriptor_count.combined_image_sampler,
        });
    }

    if (info.descriptor_count.input_attachment) {
        push(&pool_sizes, {
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
    validate_result(vkCreateDescriptorPool(vk->device, &pool_info, NULL, &pool), "failed to create descriptor pool");

    return pool;
}

static VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device, VkDescriptorSetLayoutBinding *bindings,
                                                          u32 binding_count)
{
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = binding_count;
    info.pBindings = bindings;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    validate_result(vkCreateDescriptorSetLayout(device, &info, NULL, &layout), "error creating descriptor set layout");

    return layout;
}

static VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device,
                                                          Array<VkDescriptorSetLayoutBinding> *bindings)
{
    return create_descriptor_set_layout(device, bindings->data, bindings->count);
}

static VkDescriptorSetLayout create_descriptor_set_layout(Vulkan *vk, DescriptorInfo *descriptor_infos, u32 count) {
    push_frame(vk->mem.temp);

    auto bindings = create_array<VkDescriptorSetLayoutBinding>(vk->mem.temp, count);
    for (u32 i = 0; i < count; ++i) {
        DescriptorInfo *info = descriptor_infos + i;
        push(bindings, {
            .binding = i,
            .descriptorType = info->type,
            .descriptorCount = info->count,
            .stageFlags = info->stage,
            .pImmutableSamplers = NULL,
        });
    }

    VkDescriptorSetLayout layout = create_descriptor_set_layout(vk->device, bindings);

    pop_frame(vk->mem.temp);
    return layout;
}

static VkDescriptorSetLayout create_descriptor_set_layout(Vulkan *vk, Array<DescriptorInfo> *descriptor_infos) {
    create_descriptor_set_layout(vk, descriptor_infos->data, descriptor_infos->count);
}

static void allocate_descriptor_sets(Vulkan *vk, VkDescriptorPool pool, VkDescriptorSetLayout layout, u32 count,
                                     VkDescriptorSet *descriptor_sets)
{
    push_frame(vk->mem.temp);

    auto layouts = create_array<VkDescriptorSetLayout>(vk->mem.temp, count);
    CTK_REPEAT(count)
        push(layouts, layout);

    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pool;
    info.descriptorSetCount = count;
    info.pSetLayouts = layouts->data;
    validate_result(vkAllocateDescriptorSets(vk->device, &info, descriptor_sets), "failed to allocate descriptor sets");

    pop_frame(vk->mem.temp);
}

static VkDescriptorSet allocate_descriptor_set(Vulkan *vk, VkDescriptorPool pool, VkDescriptorSetLayout layout) {
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    allocate_descriptor_sets(vk, pool, layout, 1, &descriptor_set);
    return descriptor_set;
};

static void update_descriptor_set(Vulkan *vk, VkDescriptorSet descriptor_set,
                                  u32 binding_count, DescriptorBinding *bindings)
{
    push_frame(vk->mem.temp);

    auto buf_infos = create_array<VkDescriptorBufferInfo>(vk->mem.temp, binding_count);
    auto img_infos = create_array<VkDescriptorImageInfo>(vk->mem.temp, binding_count);
    auto writes = create_array<VkWriteDescriptorSet>(vk->mem.temp, binding_count);

    for (u32 i = 0; i < binding_count; ++i) {
        DescriptorBinding *binding = bindings + i;

        VkWriteDescriptorSet *write = push(writes);
        write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write->dstSet = descriptor_set;
        write->dstBinding = i;
        write->dstArrayElement = 0;
        write->descriptorCount = 1;
        write->descriptorType = binding->type;

        if (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
            binding->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            VkDescriptorBufferInfo *info = push(buf_infos);
            info->buffer = binding->uniform_buffer->buffer->handle;
            info->offset = binding->uniform_buffer->offset;
            info->range = binding->uniform_buffer->size;

            write->pBufferInfo = info;
        }
        else if (binding->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            VkDescriptorImageInfo *info = push(img_infos);
            info->sampler = binding->image_sampler->sampler;
            info->imageView = binding->image_sampler->image->view;
            info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            write->pImageInfo = info;
        }
        else {
            CTK_FATAL("unhandled descriptor type when updating descriptor set");
        }
    }

    vkUpdateDescriptorSets(vk->device, writes->count, writes->data, 0, NULL);

    pop_frame(vk->mem.temp);
}

static constexpr PipelineInfo DEFAULT_PIPELINE_INFO = {
    .input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    },
    .depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_NEVER,
            .compareMask = 0xFF,
            .writeMask = 0xFF,
            .reference = 1,
        },
        .back = {
            .failOp = VK_STENCIL_OP_KEEP,
            .passOp = VK_STENCIL_OP_KEEP,
            .depthFailOp = VK_STENCIL_OP_KEEP,
            .compareOp = VK_COMPARE_OP_NEVER,
            .compareMask = 0xFF,
            .writeMask = 0xFF,
            .reference = 1,
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    },
    .rasterization = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE, // Don't clamp fragments within depth range.
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL, // Only available mode on AMD gpus?
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    },
    .multisample = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    },
    .color_blend = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 0,
        .pAttachments    = NULL,
        .blendConstants = { 1.0f, 1.0f, 1.0f, 1.0f },
    },
};

static constexpr VkPipelineColorBlendAttachmentState DEFAULT_COLOR_BLEND_ATTACHMENT = {
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = COLOR_COMPONENT_RGBA,
};

static Pipeline *create_pipeline(Vulkan *vk, RenderPass *render_pass, u32 subpass, PipelineInfo *info) {
    push_frame(vk->mem.temp);
    Pipeline *pipeline = allocate(vk->pool.pipeline);

    // Shader Stages
    auto shader_stages = create_array<VkPipelineShaderStageCreateInfo>(vk->mem.temp, info->shaders.count);

    for (u32 i = 0; i < info->shaders.count; ++i) {
        Shader *shader = info->shaders.data[i];
        VkPipelineShaderStageCreateInfo *shader_stage_info = push(shader_stages);
        shader_stage_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info->flags = 0;
        shader_stage_info->stage = shader->stage;
        shader_stage_info->module = shader->handle;
        shader_stage_info->pName = "main";
        shader_stage_info->pSpecializationInfo = NULL;
    }

    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (info->descriptor_set_layouts) {
        layout_ci.setLayoutCount = info->descriptor_set_layouts->count;
        layout_ci.pSetLayouts = info->descriptor_set_layouts->data;
    }
    if (info->push_constant_ranges) {
        layout_ci.pushConstantRangeCount = info->push_constant_ranges->count;
        layout_ci.pPushConstantRanges = info->push_constant_ranges->data;
    }
    validate_result(vkCreatePipelineLayout(vk->device, &layout_ci, NULL, &pipeline->layout),
                    "failed to create graphics pipeline layout");

    // Vertex Attribute Descriptions
    // auto vertex_attrib_descs =
    //     create_array<VkVertexInputAttributeDescription>(vk->mem.temp, info->vertex_inputs->count);

    // for (u32 i = 0; i < info->vertex_inputs->count; ++i) {
    //     VertexInput *vertex_input = info->vertex_inputs->data + i;
    //     push(vertex_attrib_descs, {
    //         .location = vertex_input->location,
    //         .binding = vertex_input->binding,
    //         .format = vertex_input->attribute->format,
    //         .offset = vertex_input->attribute->offset,
    //     });
    // }

    VkPipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (info->vertex_bindings) {
        vertex_input.vertexBindingDescriptionCount = info->vertex_bindings->count;
        vertex_input.pVertexBindingDescriptions = info->vertex_bindings->data;
    }
    if (info->vertex_attributes) {
        vertex_input.vertexAttributeDescriptionCount = info->vertex_attributes->count;
        vertex_input.pVertexAttributeDescriptions = info->vertex_attributes->data;
    }

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
    //     viewport_state.pViewports    = NULL;
    // }
    // else {
    //     viewport_state.viewportCount = info->viewports.count;
    //     viewport_state.pViewports    = info->viewports.data;
    // }

    // if (dynamic_scissor) {
    //     viewport_state.scissorCount = 1;
    //     viewport_state.pScissors    = NULL;
    // }
    // else {
    //     viewport_state.scissorCount = info->scissors.count;
    //     viewport_state.pScissors    = info->scissors.data;
    // }

    VkPipelineViewportStateCreateInfo viewport = {};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    if (info->viewports) {
        viewport.viewportCount = info->viewports->count;
        viewport.pViewports = info->viewports->data;
    }
    if (info->scissors) {
        viewport.scissorCount = info->scissors->count;
        viewport.pScissors = info->scissors->data;
    }

    // Reference attachment array in color_blend struct.
    info->color_blend.attachmentCount = info->color_blend_attachments.count;
    info->color_blend.pAttachments    = info->color_blend_attachments.data;

    // VkPipelineDynamicStateCreateInfo dynamic_state = {};
    // dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    // dynamic_state.dynamicStateCount = info->dynamic_states.count;
    // dynamic_state.pDynamicStates    = info->dynamic_states.data;

    VkGraphicsPipelineCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = shader_stages->count;
    create_info.pStages = shader_stages->data;
    create_info.pVertexInputState = &vertex_input;
    create_info.pInputAssemblyState = &info->input_assembly;
    create_info.pTessellationState = NULL;
    create_info.pViewportState = &viewport;
    create_info.pRasterizationState = &info->rasterization;
    create_info.pMultisampleState = &info->multisample;
    create_info.pDepthStencilState = &info->depth_stencil;
    create_info.pColorBlendState = &info->color_blend;
    create_info.pDynamicState = NULL;//&dynamic;
    create_info.layout = pipeline->layout;
    create_info.renderPass = render_pass->handle;
    create_info.subpass = subpass;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = -1;
    validate_result(vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &create_info, NULL, &pipeline->handle),
                    "failed to create graphics pipeline");

    // Cleanup
    pop_frame(vk->mem.temp);

    return pipeline;
}

static VkFramebuffer create_framebuffer(VkDevice device, VkRenderPass rp, FramebufferInfo *info) {
    VkFramebufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = rp;
    create_info.attachmentCount = info->attachments->count;
    create_info.pAttachments = info->attachments->data;
    create_info.width = info->extent.width;
    create_info.height = info->extent.height;
    create_info.layers = info->layers;
    VkFramebuffer fb = VK_NULL_HANDLE;
    validate_result(vkCreateFramebuffer(device, &create_info, NULL, &fb), "failed to create framebuffer");
    return fb;
}

static VkSemaphore create_semaphore(Vulkan *vk) {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    VkSemaphore semaphore = VK_NULL_HANDLE;
    validate_result(vkCreateSemaphore(vk->device, &info, NULL, &semaphore), "failed to create semaphore");

    return semaphore;
}

static VkFence create_fence(Vulkan *vk) {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence fence = VK_NULL_HANDLE;
    validate_result(vkCreateFence(vk->device, &info, NULL, &fence), "failed to create fence");

    return fence;
}

static void allocate_cmd_bufs(Vulkan *vk, VkCommandBuffer *cmd_bufs, VkCommandBufferAllocateInfo info) {
    validate_result(vkAllocateCommandBuffers(vk->device, &info, cmd_bufs), "failed to allocate command buffer");
}

static Array<VkCommandBuffer> *create_cmd_buf_array(Vulkan *vk, Allocator *allocator,
                                                    VkCommandBufferAllocateInfo info)
{
    auto cmd_bufs = create_array_full<VkCommandBuffer>(allocator, info.commandBufferCount);
    allocate_cmd_bufs(vk, cmd_bufs->data, info);
    return cmd_bufs;
}

////////////////////////////////////////////////////////////
/// Command Buffer
////////////////////////////////////////////////////////////
static void begin_temp_cmd_buf(VkCommandBuffer cmd_buf) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = NULL;
    vkBeginCommandBuffer(cmd_buf, &info);
}

static void submit_temp_cmd_buf(VkCommandBuffer cmd_buf, VkQueue queue) {
    vkEndCommandBuffer(cmd_buf);
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;
    validate_result(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE), "failed to submit temp command buffer");
    vkQueueWaitIdle(queue);
}

////////////////////////////////////////////////////////////
/// Rendering
////////////////////////////////////////////////////////////
static u32 next_swap_img_idx(Vulkan *vk, VkSemaphore semaphore, VkFence fence) {
    u32 img_idx = U32_MAX;

    validate_result(vkAcquireNextImageKHR(vk->device, vk->swapchain.handle, U64_MAX, semaphore, fence, &img_idx),
                    "failed to aquire next swapchain image");

    return img_idx;
}
