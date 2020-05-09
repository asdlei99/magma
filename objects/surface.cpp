/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2019 Victor Coda.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "pch.h"
#pragma hdrstop
#include "surface.h"
#include "instance.h"
#include "device.h"
#include "displayMode.h"
#include "../allocator/allocator.h"
#include "../misc/instanceExtension.h"

namespace magma
{
Surface::Surface(std::shared_ptr<const Instance> instance, std::shared_ptr<IAllocator> allocator):
    NonDispatchable(VK_OBJECT_TYPE_SURFACE_KHR, nullptr, std::move(allocator)),
    instance(std::move(instance))
{}

Surface::~Surface()
{
    MAGMA_PROFILE_ENTRY(vkDestroySurfaceKHR);
    vkDestroySurfaceKHR(MAGMA_HANDLE(instance), handle, MAGMA_OPTIONAL_INSTANCE(allocator));
}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
Win32Surface::Win32Surface(std::shared_ptr<const Instance> instance,
    HINSTANCE hinstance,
    HWND hwnd,
    VkWin32SurfaceCreateFlagsKHR flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkWin32SurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = flags;
    info.hinstance = hinstance;
    info.hwnd = hwnd;
    MAGMA_PROFILE_ENTRY(vkCreateWin32SurfaceKHR);
    const VkResult create = vkCreateWin32SurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create Win32 surface");
}

#elif defined(VK_USE_PLATFORM_XLIB_KHR)
XlibSurface::XlibSurface(std::shared_ptr<const Instance> instance,
    ::Display *dpy,
    Window window,
    VkXlibSurfaceCreateFlagsKHR flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkXlibSurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = flags;
    info.dpy = dpy;
    info.window = window;
    MAGMA_PROFILE_ENTRY(vkCreateXlibSurfaceKHR);
    const VkResult create = vkCreateXlibSurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create Xlib surface");
}

#elif defined(VK_USE_PLATFORM_XCB_KHR)
XcbSurface::XcbSurface(std::shared_ptr<const Instance> instance,
    xcb_connection_t *connection,
    xcb_window_t window,
    VkXcbSurfaceCreateFlagsKHR flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkXcbSurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = flags;
    info.connection = connection;
    info.window = window;
    MAGMA_PROFILE_ENTRY(vkCreateXcbSurfaceKHR);
    const VkResult create = vkCreateXcbSurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create XCB surface");
}

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
WaylandSurface::WaylandSurface(std::shared_ptr<const Instance> instance,
    wl_display *display,
    wl_surface *surface,
    VkWaylandSurfaceCreateFlagsKHR flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkWaylandSurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = flags;
    info.display = display;
    info.surface = surface;
    MAGMA_PROFILE_ENTRY(vkCreateWaylandSurfaceKHR);
    const VkResult create = vkCreateWaylandSurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create Wayland surface");
}

#elif defined(VK_USE_PLATFORM_MIR_KHR)
MirSurface::MirSurface(std::shared_ptr<const Instance> instance,
    MirConnection *connection,
    MirSurface *surface,
    VkMirSurfaceCreateFlagsKHR flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkMirSurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = flags;
    info.connection = connection;
    info.mirSurface = surface;
    MAGMA_PROFILE_ENTRY(vkCreateMirSurfaceKHR);
    const VkResult create = vkCreateMirSurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create Mir surface");
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
AndroidSurface::AndroidSurface(std::shared_ptr<const Instance> instance,
    ANativeWindow *window,
    VkAndroidSurfaceCreateFlagsKHR flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkAndroidSurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = flags;
    info.window = window;
    MAGMA_PROFILE_ENTRY(vkCreateAndroidSurfaceKHR);
    const VkResult create = vkCreateAndroidSurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create Android surface");
}

#elif defined(VK_USE_PLATFORM_IOS_MVK)
iOSSurface::iOSSurface(std::shared_ptr<const Instance> instance,
    const void *view,
    VkIOSSurfaceCreateFlagsMVK flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkIOSSurfaceCreateInfoMVK info;
    info.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
    info.pNext = nullptr;
    info.flags = flags;
    info.pView = view;
    MAGMA_PROFILE_ENTRY(vkCreateIOSSurfaceMVK);
    const VkResult create = vkCreateIOSSurfaceMVK(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create iOS surface");
}

#elif defined(VK_USE_PLATFORM_MACOS_MVK)
MacOSSurface::MacOSSurface(std::shared_ptr<const Instance> instance,
    const void *view,
    VkMacOSSurfaceCreateFlagsMVK flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkMacOSSurfaceCreateInfoMVK info;
    info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    info.pNext = nullptr;
    info.flags = flags;
    info.pView = view;
    MAGMA_PROFILE_ENTRY(vkCreateMacOSSurfaceMVK);
    const VkResult create = vkCreateMacOSSurfaceMVK(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create macOS surface");
}

#elif defined(VK_USE_PLATFORM_METAL_EXT)
MetalSurface::MetalSurface(std::shared_ptr<const Instance> instance,
    const CAMetalLayer *layer,
    VkMacOSSurfaceCreateFlagsMVK flags /* 0 */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkMetalSurfaceCreateInfoEXT info;
    info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    info.pNext = nullptr;
    info.flags = flags;
    info.pLayer = layer;
    MAGMA_PROFILE_ENTRY(vkCreateMetalSurfaceEXT);
    const VkResult create = vkCreateMetalSurfaceEXT(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create Metal surface");
}
#endif // VK_USE_PLATFORM_METAL_EXT

#ifdef VK_KHR_display
DisplaySurface::DisplaySurface(std::shared_ptr<const Instance> instance,
    std::shared_ptr<const DisplayMode> displayMode,
    uint32_t planeIndex,
    uint32_t planeStackIndex,
    VkSurfaceTransformFlagBitsKHR transform,
    VkDisplayPlaneAlphaFlagBitsKHR alphaMode,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    Surface(std::move(instance), std::move(allocator))
{
    VkDisplaySurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
    info.pNext = nullptr;
    info.flags = 0;
    info.displayMode = *displayMode;
    info.planeIndex = planeIndex;
    info.planeStackIndex = planeStackIndex;
    info.transform = transform;
    info.globalAlpha = 1.f;
    info.alphaMode = alphaMode;
    info.imageExtent = displayMode->getVisibleRegion();
    MAGMA_INSTANCE_EXTENSION(vkCreateDisplayPlaneSurfaceKHR, VK_KHR_DISPLAY_EXTENSION_NAME);
    MAGMA_PROFILE_ENTRY(vkCreateDisplayPlaneSurfaceKHR);
    const VkResult create = vkCreateDisplayPlaneSurfaceKHR(MAGMA_HANDLE(instance), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create display surface");
}
#endif // VK_KHR_display
} // namespace magma
