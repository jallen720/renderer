#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"

////////////////////////////////////////////////////////////
/// Macros
////////////////////////////////////////////////////////////
#define _VK_RESULT_NAME(VK_RESULT) VK_RESULT, #VK_RESULT

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
struct _VkResultInfo {
    VkResult result;
    cstr name;
    cstr message;
};

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static void _print_result(VkResult result) {
    static _VkResultInfo VK_RESULT_DEBUG_INFOS[] = {
        { _VK_RESULT_NAME(VK_SUCCESS), "VULKAN SPEC ERROR MESSAGE: Command successfully completed." },
        { _VK_RESULT_NAME(VK_NOT_READY), "VULKAN SPEC ERROR MESSAGE: A fence or query has not yet completed." },
        { _VK_RESULT_NAME(VK_TIMEOUT), "VULKAN SPEC ERROR MESSAGE: A wait operation has not completed in the specified time." },
        { _VK_RESULT_NAME(VK_EVENT_SET), "VULKAN SPEC ERROR MESSAGE: An event is signaled." },
        { _VK_RESULT_NAME(VK_EVENT_RESET), "VULKAN SPEC ERROR MESSAGE: An event is unsignaled." },
        { _VK_RESULT_NAME(VK_INCOMPLETE), "VULKAN SPEC ERROR MESSAGE: A return array was too small for the result." },
        { _VK_RESULT_NAME(VK_SUBOPTIMAL_KHR), "VULKAN SPEC ERROR MESSAGE: A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully." },
        { _VK_RESULT_NAME(VK_ERROR_OUT_OF_HOST_MEMORY), "VULKAN SPEC ERROR MESSAGE: A host memory allocation has failed." },
        { _VK_RESULT_NAME(VK_ERROR_OUT_OF_DEVICE_MEMORY), "VULKAN SPEC ERROR MESSAGE: A device memory allocation has failed." },
        { _VK_RESULT_NAME(VK_ERROR_INITIALIZATION_FAILED), "VULKAN SPEC ERROR MESSAGE: Initialization of an object could not be completed for implementation-specific reasons." },
        { _VK_RESULT_NAME(VK_ERROR_DEVICE_LOST), "VULKAN SPEC ERROR MESSAGE: The logical or physical device has been lost." },
        { _VK_RESULT_NAME(VK_ERROR_MEMORY_MAP_FAILED), "VULKAN SPEC ERROR MESSAGE: Mapping of a memory object has failed." },
        { _VK_RESULT_NAME(VK_ERROR_LAYER_NOT_PRESENT), "VULKAN SPEC ERROR MESSAGE: A requested layer is not present or could not be loaded." },
        { _VK_RESULT_NAME(VK_ERROR_EXTENSION_NOT_PRESENT), "VULKAN SPEC ERROR MESSAGE: A requested extension is not supported." },
        { _VK_RESULT_NAME(VK_ERROR_FEATURE_NOT_PRESENT), "VULKAN SPEC ERROR MESSAGE: A requested feature is not supported." },
        { _VK_RESULT_NAME(VK_ERROR_INCOMPATIBLE_DRIVER), "VULKAN SPEC ERROR MESSAGE: The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons." },
        { _VK_RESULT_NAME(VK_ERROR_TOO_MANY_OBJECTS), "VULKAN SPEC ERROR MESSAGE: Too many objects of the type have already been created." },
        { _VK_RESULT_NAME(VK_ERROR_FORMAT_NOT_SUPPORTED), "VULKAN SPEC ERROR MESSAGE: A requested format is not supported on this device." },
        { _VK_RESULT_NAME(VK_ERROR_FRAGMENTED_POOL), "VULKAN SPEC ERROR MESSAGE: A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation." },
        { _VK_RESULT_NAME(VK_ERROR_SURFACE_LOST_KHR), "VULKAN SPEC ERROR MESSAGE: A surface is no longer available." },
        { _VK_RESULT_NAME(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR), "VULKAN SPEC ERROR MESSAGE: The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again." },
        { _VK_RESULT_NAME(VK_ERROR_OUT_OF_DATE_KHR), "VULKAN SPEC ERROR MESSAGE: A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface." },
        { _VK_RESULT_NAME(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR), "VULKAN SPEC ERROR MESSAGE: The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image." },
        { _VK_RESULT_NAME(VK_ERROR_INVALID_SHADER_NV), "VULKAN SPEC ERROR MESSAGE: One or more shaders failed to compile or link. More details are reported back to the application via https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VK_EXT_debug_report if enabled." },
        { _VK_RESULT_NAME(VK_ERROR_OUT_OF_POOL_MEMORY), "VULKAN SPEC ERROR MESSAGE: A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead." },
        { _VK_RESULT_NAME(VK_ERROR_INVALID_EXTERNAL_HANDLE), "VULKAN SPEC ERROR MESSAGE: An external handle is not a valid handle of the specified type." },
        // { _VK_RESULT_NAME(VK_ERROR_FRAGMENTATION), "VULKAN SPEC ERROR MESSAGE: A descriptor pool creation has failed due to fragmentation." },
        { _VK_RESULT_NAME(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT), "VULKAN SPEC ERROR MESSAGE: A buffer creation failed because the requested address is not available." },
        // { _VK_RESULT_NAME(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS), "VULKAN SPEC ERROR MESSAGE: A buffer creation or memory allocation failed because the requested address is not available." },
        { _VK_RESULT_NAME(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT), "VULKAN SPEC ERROR MESSAGE: An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control." },
        // { _VK_RESULT_NAME(VK_ERROR_UNKNOWN), "VULKAN SPEC ERROR MESSAGE: An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred." },
    };

    _VkResultInfo *info = NULL;
    for (u32 i = 0; i < CTK_ARRAY_SIZE(VK_RESULT_DEBUG_INFOS); ++i) {
        info = VK_RESULT_DEBUG_INFOS + i;

        if (info->result == result)
            break;
    }

    if (!info)
        CTK_FATAL("failed to find debug info for VkResult %d", result)

    if (info->result == 0)
        ctk_info("vulkan function returned %s: %s", info->name, info->message);
    else if (info->result > 0)
        ctk_warning("vulkan function returned %s: %s", info->name, info->message);
    else
        ctk_error("vulkan function returned %s: %s", info->name, info->message);
}

template<typename ...Args>
static void validate_result(VkResult result, cstr fail_message, Args... args) {
    if (result != VK_SUCCESS) {
        _print_result(result);
        CTK_FATAL(fail_message, args...)
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flag_bit,
                   VkDebugUtilsMessageTypeFlagsEXT message_type_flags,
                   VkDebugUtilsMessengerCallbackDataEXT const *callback_data,
                   void *user_data) {
    cstr message_id = callback_data->pMessageIdName ? callback_data->pMessageIdName : "";

    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT & message_severity_flag_bit)
        CTK_FATAL("VALIDATION LAYER [%s]: %s\n", message_id, callback_data->pMessage)
    else if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT & message_severity_flag_bit)
        ctk_warning("VALIDATION LAYER [%s]: %s\n", message_id, callback_data->pMessage);
    else
        ctk_info("VALIDATION LAYER [%s]: %s\n", message_id, callback_data->pMessage);

    return VK_FALSE;
}
