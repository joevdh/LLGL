/*
 * VKExtensionRegistry.h
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#ifndef LLGL_VK_EXTENSION_REGISTRY_H
#define LLGL_VK_EXTENSION_REGISTRY_H


#include <LLGL/ForwardDecls.h>
#include "../Vulkan.h"   // Wraps <vulkan/vulkan.h>, ensuring the platform VK_USE_PLATFORM_*_KHR
                         // defines are set so VK_KHR_WIN32_SURFACE_EXTENSION_NAME etc. are visible.
#include <vector>


#define LLGL_ASSERT_VK_EXT(EXT) \
    if (!LLGL::HasExtension(LLGL::VKExt::EXT)) { LLGL::TrapVKExtensionNotSupported(__FUNCTION__, "VK_" #EXT); }


namespace LLGL
{


// Vulkan extension enumeration.
enum class VKExt
{
    /* Required surface extensions */
    KHR_android_surface,
    KHR_win32_surface,
    KHR_xlib_surface,

    #if LLGL_LINUX_ENABLE_WAYLAND
    KHR_wayland_surface,
    #endif

    /* Khronos extensions */
    KHR_maintenance1,
    KHR_get_physical_device_properties2,
    KHR_imageless_framebuffer,
    KHR_multiview,              // Needed for EXT_mesh_shader
    KHR_fragment_shading_rate,  // Needed for EXT_mesh_shader

    /* Multivendor extensions */
    EXT_conditional_rendering,
    EXT_conservative_rasterization,
    EXT_debug_marker,
    EXT_debug_utils,
    EXT_nested_command_buffer,
    EXT_transform_feedback,
    EXT_headless_surface,
    EXT_mesh_shader,

    /* Enumeration entry counter */
    Count,
};

// Vulkan extension support enumeration.
enum class VKExtSupport
{
    Unsupported,    // Vulkan extension is unsupported and will not be loaded.
    Optional,       // Vulkan extension is supported but optional.
    DebugOnly,      // Vulkan extension is supported but only used for debugging.
    Required,       // Vulkan extension is supported and required.
};


// Registers the specified Vulkan extension support.
void RegisterExtension(VKExt extension);

// Returns true if the specified Vulkan extension is supported.
bool HasExtension(const VKExt extension);

// Returns the null-terminated list of optional extensions.
const char** GetOptionalExtensions();

// Returns the null-terminated list of device extensions LLGL requires on every VkDevice.
const char** GetRequiredDeviceExtensions();

// Returns the type of support for the specified Vulkan instance extension.
VKExtSupport GetVulkanInstanceExtensionSupport(const char* extensionName);

// Returns true if the named Vulkan instance layer should be enabled. A layer is enabled if it
// appears in `config->enabledLayers` (when `config != nullptr`), or if it's the validation
// layer and `isDebugLayerEnabled` is true.
bool IsVulkanInstanceLayerRequired(
    const char*                          layerName,
    bool                                 isDebugLayerEnabled,
    const RendererConfigurationVulkan*   config = nullptr
);

// Selects the set of Vulkan instance extensions LLGL wants enabled, given the available
// extensions reported by the loader. Required + Optional are always included; DebugOnly
// extensions are included only when `isDebugLayerEnabled` is true.
//
// `outProps` MUST outlive `outExtensionNames`: the c-string pointers in `outExtensionNames`
// reference VkExtensionProperties::extensionName arrays inside `outProps`.
void QuerySupportedInstanceExtensions(
    bool                                 isDebugLayerEnabled,
    std::vector<VkExtensionProperties>&  outProps,
    std::vector<const char*>&            outExtensionNames
);

// Selects the set of Vulkan instance layers LLGL wants enabled, by filtering the available
// layers through IsVulkanInstanceLayerRequired.
//
// `outProps` MUST outlive `outLayerNames`.
void QuerySupportedInstanceLayers(
    bool                                 isDebugLayerEnabled,
    const RendererConfigurationVulkan*   config,
    std::vector<VkLayerProperties>&      outProps,
    std::vector<const char*>&            outLayerNames
);

// Selects the set of Vulkan device extensions LLGL wants enabled on the given physical device.
// Always includes everything from GetRequiredDeviceExtensions() that the device reports as
// supported, plus any Optional extensions from GetOptionalExtensions() that are supported.
//
// `outProps` MUST outlive `outExtensionNames`.
void QuerySupportedDeviceExtensions(
    VkPhysicalDevice                     physicalDevice,
    std::vector<VkExtensionProperties>&  outProps,
    std::vector<const char*>&            outExtensionNames
);


} // /namespace LLGL


#endif



// ================================================================================
