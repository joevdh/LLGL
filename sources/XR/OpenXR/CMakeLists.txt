#
# CMakeLists.txt file for LLGL/OpenXR module
#
# Copyright (c) 2015 Lukas Hermanns. All rights reserved.
# Licensed under the terms of the BSD 3-Clause license (see LICENSE.txt).
#

if (NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
    cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
endif()

project(LLGL_XR_OpenXR)


# === Source files ===

# Backend-agnostic OpenXR core (under sources/XR)
find_source_files(FilesXR             CXX "${PROJECT_SOURCE_DIR}/..")
find_source_files(FilesXROpenXR       CXX "${PROJECT_SOURCE_DIR}")
find_source_files(FilesIncludeXR      "*.h" "${PROJECT_INCLUDE_DIR}/LLGL/Backend/OpenXR")

set(
    FilesXRAll
    ${FilesXR}
    ${FilesXROpenXR}
    ${FilesIncludeXR}
)


# === Source group folders ===

source_group("XR"           FILES ${FilesXR})
source_group("XR\\OpenXR"   FILES ${FilesXROpenXR})
source_group("Include\\Backend\\OpenXR" FILES ${FilesIncludeXR})


# === Project ===

if(LLGL_BUILD_XR_OPENXR)
    if(NOT OPENXR_FOUND)
        message(FATAL_ERROR "LLGL_BUILD_XR_OPENXR failed: OpenXR-SDK was not resolved (see FindOpenXRDeps.cmake).")
    endif()

    # Build as a static helper library; XR is not loaded as a runtime module.
    add_library(LLGL_XR_OpenXR STATIC ${FilesXRAll})
    set_target_properties(
        LLGL_XR_OpenXR PROPERTIES
            LINKER_LANGUAGE             CXX
            DEBUG_POSTFIX               "D"
            FOLDER                      "LLGL"
            ARCHIVE_OUTPUT_DIRECTORY    "${LLGL_OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY    "${LLGL_OUTPUT_DIR}"
    )
    set_llgl_unity_build_mode(LLGL_XR_OpenXR)

    target_link_libraries(LLGL_XR_OpenXR PUBLIC LLGL)
    target_link_libraries(LLGL_XR_OpenXR PUBLIC ${OPENXR_HEADER_TARGET})
    # PUBLIC loader linkage so consumers of this static helper library transitively link the loader too.
    target_link_libraries(LLGL_XR_OpenXR PUBLIC ${OPENXR_LOADER_TARGET})

    # When the Vulkan backend is also built, it provides the Vulkan graphics binding
    # implementation - link the binding into the XR target so registration happens.
    if(LLGL_BUILD_RENDERER_VULKAN)
        target_link_libraries(LLGL_XR_OpenXR PRIVATE LLGL_Vulkan)
        target_compile_definitions(LLGL_XR_OpenXR PRIVATE LLGL_XR_OPENXR_BIND_VULKAN)
    endif()

    # Same pattern for the Direct3D 11 backend (Windows only).
    if(LLGL_BUILD_RENDERER_DIRECT3D11)
        target_link_libraries(LLGL_XR_OpenXR PRIVATE LLGL_Direct3D11)
        target_compile_definitions(LLGL_XR_OpenXR PRIVATE LLGL_XR_OPENXR_BIND_DIRECT3D11)
    endif()

    # And for Direct3D 12 (Windows only).
    if(LLGL_BUILD_RENDERER_DIRECT3D12)
        target_link_libraries(LLGL_XR_OpenXR PRIVATE LLGL_Direct3D12)
        target_compile_definitions(LLGL_XR_OpenXR PRIVATE LLGL_XR_OPENXR_BIND_DIRECT3D12)
    endif()

    target_compile_definitions(LLGL_XR_OpenXR PRIVATE LLGL_BUILD_XR_OPENXR)
    # NOTE: XR_USE_GRAPHICS_API_VULKAN and XR_USE_PLATFORM_ANDROID are set by sources/XR/OpenXR/OpenXRPlatform.h
    # based on the build environment, not from CMake. See that header for details.

    # When the loader is a shared library (typical Windows case from the SDK), stage it
    # so consumer executables can find it at runtime.
    if(WIN32 AND TARGET ${OPENXR_LOADER_TARGET})
        get_target_property(_oxr_loader_type ${OPENXR_LOADER_TARGET} TYPE)
        if(_oxr_loader_type STREQUAL "SHARED_LIBRARY")
            # 1) Always copy next to LLGL's TARGET_FILE_DIR. When LLGL is built shared this is
            #    where LLGL.dll lives, so anything that loads LLGL.dll can find the loader too.
            #    When LLGL is static this is the .lib folder - harmless but not useful on its own.
            add_custom_command(
                TARGET LLGL_XR_OpenXR POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${OPENXR_LOADER_TARGET}>"
                    "$<TARGET_FILE_DIR:LLGL>/$<TARGET_FILE_NAME:${OPENXR_LOADER_TARGET}>"
                COMMENT "Copying OpenXR loader DLL next to LLGL"
            )

            # 2) If the consuming project centralises runtime output via
            #    CMAKE_RUNTIME_OUTPUT_DIRECTORY (typical for solutions that put all
            #    executables/DLLs into a single bin/ folder), mirror the loader DLL there
            #    too. This is essential when LLGL is built as a static library: there's no
            #    LLGL.dll to copy alongside, and the consumer's exes need the loader DLL
            #    in their own runtime directory to launch.
            if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
                if(CMAKE_CONFIGURATION_TYPES)
                    # Multi-config generators (Visual Studio, Xcode) append the per-config
                    # subfolder (Debug/Release/...) to RUNTIME_OUTPUT_DIRECTORY automatically.
                    set(_oxr_mirror_dst "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
                else()
                    # Single-config (Ninja, Make): one output directory.
                    set(_oxr_mirror_dst "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
                endif()
                add_custom_command(
                    TARGET LLGL_XR_OpenXR POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${_oxr_mirror_dst}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${OPENXR_LOADER_TARGET}>"
                        "${_oxr_mirror_dst}/$<TARGET_FILE_NAME:${OPENXR_LOADER_TARGET}>"
                    COMMENT "Mirroring OpenXR loader DLL to consumer runtime output directory (${_oxr_mirror_dst})"
                )
            endif()
        endif()
    endif()

    append_global_property(LLGL_GLOBAL_MODULE_LIST None LLGL_XR_OpenXR)
endif()
