/*
 * VKExtensionRegistry.cpp
 *
 * Copyright (c) 2015 Lukas Hermanns. All rights reserved.
 * Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
 */

#include "VKExtensionRegistry.h"
#include "../Vulkan.h"
#include <LLGL/Container/Strings.h>
#include <LLGL/Platform/Platform.h>
#include <LLGL/RendererConfiguration.h>
#include <cstring>


namespace LLGL
{


#ifndef VK_LAYER_KHRONOS_VALIDATION_NAME
#define VK_LAYER_KHRONOS_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"
#endif


static bool g_VKRegisteredExtensions[static_cast<std::size_t>(VKExt::Count)] = {};

// Required device extensions LLGL needs enabled on every VkDevice. Both the normal
// VKRenderSystem path (via VKPhysicalDevice::PickPhysicalDevice) and the OpenXR binding
// (which has to populate VkDeviceCreateInfo itself, since the runtime won't add these)
// reference this single list.
static const char* g_VKRequiredDeviceExtensions[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    nullptr,
};

static const char* g_VKOptionalExtensions[] =
{
    #if VK_EXT_conditional_rendering
    VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
    #endif
    #if VK_EXT_conservative_rasterization
    VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
    #endif
    #if VK_EXT_debug_marker
    VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
    #endif
    #if VK_EXT_debug_report
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    #endif
    #if VK_EXT_debug_utils
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    #endif
    #if VK_KHR_get_physical_device_properties2
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    #endif
    #if VK_KHR_imageless_framebuffer
    VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
    #endif
    #if VK_KHR_portability_enumeration
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    #endif
    #if VK_KHR_sampler_mirror_clamp_to_edge
    VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
    #endif
    #if VK_EXT_transform_feedback
    VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME,
    #endif
    #if VK_EXT_nested_command_buffer
    VK_EXT_NESTED_COMMAND_BUFFER_EXTENSION_NAME,
    #endif
    #if VK_EXT_headless_surface
    VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME,
    #endif
    #if VK_KHR_fragment_shading_rate
    VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
    #endif
    #if VK_KHR_multiview
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    #endif
    #if VK_EXT_mesh_shader
    VK_EXT_MESH_SHADER_EXTENSION_NAME,
    #endif
    nullptr,
};

void RegisterExtension(VKExt extension)
{
    g_VKRegisteredExtensions[static_cast<std::size_t>(extension)] = true;
}

bool HasExtension(const VKExt extension)
{
    return g_VKRegisteredExtensions[static_cast<std::size_t>(extension)];
}

const char** GetOptionalExtensions()
{
    return g_VKOptionalExtensions;
}

const char** GetRequiredDeviceExtensions()
{
    return g_VKRequiredDeviceExtensions;
}

static bool IsVulkanInstanceExtRequired(const StringView& name)
{
    return
    (
        name == VK_KHR_SURFACE_EXTENSION_NAME
        #if defined LLGL_OS_WIN32
        || name == VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        #elif defined LLGL_OS_LINUX
        || name == VK_KHR_XLIB_SURFACE_EXTENSION_NAME
        #if LLGL_LINUX_ENABLE_WAYLAND
        || name == VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
        #endif
        #elif defined LLGL_OS_ANDROID
        || name == VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
        #elif defined LLGL_OS_MACOS || defined LLGL_OS_IOS
        || name == VK_EXT_METAL_SURFACE_EXTENSION_NAME
        #endif
    );
}

static bool IsVulkanInstanceExtOptional(const StringView& name)
{
    return
    (
        false
        #if VK_KHR_get_physical_device_properties2
        || name == VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        #endif
        #if VK_KHR_portability_enumeration
        || name == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        #endif
        #if VK_EXT_headless_surface
        || name == VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME
        #endif
    );
}

static bool IsVulkanInstanceExtDebugOnly(const StringView& name)
{
    return
    (
        false
        #if VK_EXT_debug_marker
        || name == VK_EXT_DEBUG_MARKER_EXTENSION_NAME
        #endif
        #if VK_EXT_debug_report
        || name == VK_EXT_DEBUG_REPORT_EXTENSION_NAME
        #endif
        #if VK_EXT_debug_utils
        || name == VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        #endif
    );
}

VKExtSupport GetVulkanInstanceExtensionSupport(const char* extensionName)
{
    const StringView name = extensionName;
    if (IsVulkanInstanceExtRequired(name))
        return VKExtSupport::Required;
    if (IsVulkanInstanceExtOptional(name))
        return VKExtSupport::Optional;
    if (IsVulkanInstanceExtDebugOnly(name))
        return VKExtSupport::DebugOnly;
    return VKExtSupport::Unsupported;
}

bool IsVulkanInstanceLayerRequired(
    const char*                         layerName,
    bool                                isDebugLayerEnabled,
    const RendererConfigurationVulkan*  config)
{
    if (config != nullptr)
    {
        for (const char* layer : config->enabledLayers)
        {
            if (std::strcmp(layer, layerName) == 0)
                return true;
        }
    }

    if (isDebugLayerEnabled)
    {
        if (std::strcmp(layerName, VK_LAYER_KHRONOS_VALIDATION_NAME) == 0)
            return true;
    }
    return false;
}

void QuerySupportedInstanceExtensions(
    bool                                isDebugLayerEnabled,
    std::vector<VkExtensionProperties>& outProps,
    std::vector<const char*>&           outExtensionNames)
{
    std::uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS || count == 0)
        return;
    outProps.resize(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, outProps.data()) != VK_SUCCESS)
        return;

    outExtensionNames.reserve(outProps.size());
    for (const VkExtensionProperties& prop : outProps)
    {
        const VKExtSupport tier = GetVulkanInstanceExtensionSupport(prop.extensionName);
        const bool include =
            (tier == VKExtSupport::Required) ||
            (tier == VKExtSupport::Optional) ||
            (isDebugLayerEnabled && tier == VKExtSupport::DebugOnly);
        if (include)
            outExtensionNames.push_back(prop.extensionName);
    }
}

void QuerySupportedInstanceLayers(
    bool                                isDebugLayerEnabled,
    const RendererConfigurationVulkan*  config,
    std::vector<VkLayerProperties>&     outProps,
    std::vector<const char*>&           outLayerNames)
{
    std::uint32_t count = 0;
    if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS || count == 0)
        return;
    outProps.resize(count);
    if (vkEnumerateInstanceLayerProperties(&count, outProps.data()) != VK_SUCCESS)
        return;

    outLayerNames.reserve(outProps.size());
    for (const VkLayerProperties& prop : outProps)
    {
        if (IsVulkanInstanceLayerRequired(prop.layerName, isDebugLayerEnabled, config))
            outLayerNames.push_back(prop.layerName);
    }
}

void QuerySupportedDeviceExtensions(
    VkPhysicalDevice                    physicalDevice,
    std::vector<VkExtensionProperties>& outProps,
    std::vector<const char*>&           outExtensionNames)
{
    std::uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr) != VK_SUCCESS || count == 0)
        return;
    outProps.resize(count);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, outProps.data()) != VK_SUCCESS)
        return;

    auto IsAvailable = [&outProps](const char* name) -> bool
    {
        for (const VkExtensionProperties& p : outProps)
        {
            if (std::strcmp(p.extensionName, name) == 0)
                return true;
        }
        return false;
    };

    auto AddIfAvailableAndNew = [&](const char* name)
    {
        if (!IsAvailable(name))
            return;
        for (const char* added : outExtensionNames)
        {
            if (std::strcmp(added, name) == 0)
                return;
        }
        outExtensionNames.push_back(name);
    };

    if (const char** req = GetRequiredDeviceExtensions())
    {
        for (std::size_t i = 0; req[i] != nullptr; ++i)
            AddIfAvailableAndNew(req[i]);
    }

    if (const char** opts = GetOptionalExtensions())
    {
        for (std::size_t i = 0; opts[i] != nullptr; ++i)
            AddIfAvailableAndNew(opts[i]);
    }
}


} // /namespace LLGL



// ================================================================================
