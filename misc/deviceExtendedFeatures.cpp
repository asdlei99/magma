/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2022 Victor Coda.

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
#include "deviceExtendedFeatures.h"
#include "../objects/device.h"
#include "../objects/physicalDevice.h"

namespace magma
{
bool DeviceExtendedFeatures::maintenanceLevelEnabled(MaintenanceLevel level) const noexcept
{
    std::shared_ptr<const Device> device = deviceRef.lock();
    if (device)
    {
        switch (level)
        {
    #ifdef VK_KHR_maintenance1
        case MaintenanceLevel::One:
            return device->extensionEnabled(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
    #endif
    #ifdef VK_KHR_maintenance2
        case MaintenanceLevel::Two:
            return device->extensionEnabled(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
    #endif
    #ifdef VK_KHR_maintenance3
        case MaintenanceLevel::Three:
            return device->extensionEnabled(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    #endif
        default:
            return false;
        }
    }
    return false;
}

bool DeviceExtendedFeatures::negativeViewportHeightEnabled() const noexcept
{
#ifdef VK_AMD_negative_viewport_height
    std::shared_ptr<const Device> device = deviceRef.lock();
    if (device)
        return device->extensionEnabled(VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
#else // VK_AMD_negative_viewport_height
    MAGMA_UNUSED(khronos);
#endif
    return false;
}

bool DeviceExtendedFeatures::separateDepthStencilLayoutsEnabled() const noexcept
{
#ifdef VK_KHR_separate_depth_stencil_layouts
    std::shared_ptr<const Device> device = deviceRef.lock();
    if (device)
    {
        std::shared_ptr<const PhysicalDevice> physicalDevice = device->getPhysicalDevice();
        if (physicalDevice->extensionSupported(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME))
        {
            auto separateDepthStencilFeatures = device->getEnabledExtendedFeatures<VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR>();
            if (separateDepthStencilFeatures)
                return (VK_TRUE == separateDepthStencilFeatures->separateDepthStencilLayouts);
        }
    }
#endif // VK_KHR_separate_depth_stencil_layouts
    return false;
}

bool DeviceExtendedFeatures::stippledLinesEnabled() const noexcept
{
#ifdef VK_EXT_line_rasterization
    std::shared_ptr<const Device> device = deviceRef.lock();
    if (device)
    {
        std::shared_ptr<const PhysicalDevice> physicalDevice = device->getPhysicalDevice();
        if (physicalDevice->extensionSupported(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME))
        {
            auto lineRasterizationFeatures = device->getEnabledExtendedFeatures<VkPhysicalDeviceLineRasterizationFeaturesEXT>();
            if (lineRasterizationFeatures)
            {
                return lineRasterizationFeatures->stippledRectangularLines ||
                    lineRasterizationFeatures->stippledBresenhamLines ||
                    lineRasterizationFeatures->stippledSmoothLines;
            }
        }
    }
#endif // VK_EXT_line_rasterization
    return false;
}
} // namespace magma
