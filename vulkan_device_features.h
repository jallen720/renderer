#pragma once

#include <vulkan/vulkan.h>
#include "ctk/ctk.h"

using namespace ctk;

////////////////////////////////////////////////////////////
/// Data
////////////////////////////////////////////////////////////
enum struct PhysicalDeviceFeature {
    robustBufferAccess,
    fullDrawIndexUint32,
    imageCubeArray,
    independentBlend,
    geometryShader,
    tessellationShader,
    sampleRateShading,
    dualSrcBlend,
    logicOp,
    multiDrawIndirect,
    drawIndirectFirstInstance,
    depthClamp,
    depthBiasClamp,
    fillModeNonSolid,
    depthBounds,
    wideLines,
    largePoints,
    alphaToOne,
    multiViewport,
    samplerAnisotropy,
    textureCompressionETC2,
    textureCompressionASTC_LDR,
    textureCompressionBC,
    occlusionQueryPrecise,
    pipelineStatisticsQuery,
    vertexPipelineStoresAndAtomics,
    fragmentStoresAndAtomics,
    shaderTessellationAndGeometryPointSize,
    shaderImageGatherExtended,
    shaderStorageImageExtendedFormats,
    shaderStorageImageMultisample,
    shaderStorageImageReadWithoutFormat,
    shaderStorageImageWriteWithoutFormat,
    shaderUniformBufferArrayDynamicIndexing,
    shaderSampledImageArrayDynamicIndexing,
    shaderStorageBufferArrayDynamicIndexing,
    shaderStorageImageArrayDynamicIndexing,
    shaderClipDistance,
    shaderCullDistance,
    shaderFloat64,
    shaderInt64,
    shaderInt16,
    shaderResourceResidency,
    shaderResourceMinLod,
    sparseBinding,
    sparseResidencyBuffer,
    sparseResidencyImage2D,
    sparseResidencyImage3D,
    sparseResidency2Samples,
    sparseResidency4Samples,
    sparseResidency8Samples,
    sparseResidency16Samples,
    sparseResidencyAliased,
    variableMultisampleRate,
    inheritedQueries,
    COUNT,
};

static constexpr cstr PHYSICAL_DEVICE_FEATURE_NAMES[] = {
    "robustBufferAccess",
    "fullDrawIndexUint32",
    "imageCubeArray",
    "independentBlend",
    "geometryShader",
    "tessellationShader",
    "sampleRateShading",
    "dualSrcBlend",
    "logicOp",
    "multiDrawIndirect",
    "drawIndirectFirstInstance",
    "depthClamp",
    "depthBiasClamp",
    "fillModeNonSolid",
    "depthBounds",
    "wideLines",
    "largePoints",
    "alphaToOne",
    "multiViewport",
    "samplerAnisotropy",
    "textureCompressionETC2",
    "textureCompressionASTC_LDR",
    "textureCompressionBC",
    "occlusionQueryPrecise",
    "pipelineStatisticsQuery",
    "vertexPipelineStoresAndAtomics",
    "fragmentStoresAndAtomics",
    "shaderTessellationAndGeometryPointSize",
    "shaderImageGatherExtended",
    "shaderStorageImageExtendedFormats",
    "shaderStorageImageMultisample",
    "shaderStorageImageReadWithoutFormat",
    "shaderStorageImageWriteWithoutFormat",
    "shaderUniformBufferArrayDynamicIndexing",
    "shaderSampledImageArrayDynamicIndexing",
    "shaderStorageBufferArrayDynamicIndexing",
    "shaderStorageImageArrayDynamicIndexing",
    "shaderClipDistance",
    "shaderCullDistance",
    "shaderFloat64",
    "shaderInt64",
    "shaderInt16",
    "shaderResourceResidency",
    "shaderResourceMinLod",
    "sparseBinding",
    "sparseResidencyBuffer",
    "sparseResidencyImage2D",
    "sparseResidencyImage3D",
    "sparseResidency2Samples",
    "sparseResidency4Samples",
    "sparseResidency8Samples",
    "sparseResidency16Samples",
    "sparseResidencyAliased",
    "variableMultisampleRate",
    "inheritedQueries",
};

////////////////////////////////////////////////////////////
/// Interface
////////////////////////////////////////////////////////////
static cstr physical_device_feature_name(PhysicalDeviceFeature feat) {
    CTK_ASSERT(feat < PhysicalDeviceFeature::COUNT);
    return PHYSICAL_DEVICE_FEATURE_NAMES[(s32)feat];
}

static bool physical_device_feature_supported(PhysicalDeviceFeature feat, VkPhysicalDeviceFeatures *phys_dev_feats) {
    CTK_ASSERT(feat < PhysicalDeviceFeature::COUNT);
    return ((VkBool32 *)phys_dev_feats)[(s32)feat];
}
