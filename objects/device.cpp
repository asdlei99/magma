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
#include "device.h"
#include "physicalDevice.h"
#include "surface.h"
#include "queue.h"
#include "fence.h"
#include "timelineSemaphore.h"
#include "accelerationStructureKhr.h"
#include "resourcePool.h"
#include "../allocator/allocator.h"
#include "../exceptions/errorResult.h"
#include "../helpers/stackArray.h"

// Redefine macro because here we use handle directly
// instead of using circular reference to itself
#ifdef MAGMA_HANDLE
#undef MAGMA_HANDLE
#define MAGMA_HANDLE(p) handle
#endif
#include "../misc/extProcAddress.h"

namespace magma
{
Device::Device(std::shared_ptr<PhysicalDevice> physicalDevice,
    const std::vector<DeviceQueueDescriptor>& queueDescriptors,
    const std::vector<const char *>& enabledLayers,
    const std::vector<const char *>& enabledExtensions,
    const VkPhysicalDeviceFeatures& deviceFeatures,
    const std::vector<void *>& extendedDeviceFeatures,
    const StructureChain& extendedInfo /* default */,
    std::shared_ptr<IAllocator> allocator):
    Dispatchable<VkDevice>(VK_OBJECT_TYPE_DEVICE, nullptr, std::move(allocator)),
    physicalDevice(std::move(physicalDevice)),
    resourcePool(std::make_shared<ResourcePool>()),
    enabledFeatures(deviceFeatures)
{
#ifdef VK_KHR_get_physical_device_properties2
    VkPhysicalDeviceFeatures2KHR features;
    if (!extendedDeviceFeatures.empty())
    {
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
        features.features = deviceFeatures;
        features.pNext = extendedDeviceFeatures.front();
        auto curr = extendedDeviceFeatures.begin();
        auto next = curr; ++next;
        while (next != extendedDeviceFeatures.end())
        {   // Make linked list
            VkBaseInStructure *currNode = reinterpret_cast<VkBaseInStructure *>(*curr);
            VkBaseInStructure *nextNode = reinterpret_cast<VkBaseInStructure *>(*next);
            currNode->pNext = nextNode;
            ++curr; ++next;
            enabledExtendedFeatures[currNode->sType] = currNode;
        }
        VkBaseInStructure *lastNode = reinterpret_cast<VkBaseInStructure *>(*curr);
        lastNode->pNext = reinterpret_cast<const VkBaseInStructure *>(extendedInfo.getChainedNodes());
    }
#endif // VK_KHR_get_physical_device_properties2
    VkDeviceCreateInfo deviceInfo;
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
#ifdef VK_KHR_get_physical_device_properties2
    deviceInfo.pNext = extendedDeviceFeatures.empty() ? extendedInfo.getChainedNodes() : &features;
#else
    deviceInfo.pNext = extendedInfo.getChainedNodes();
#endif
    deviceInfo.flags = 0;
    deviceInfo.queueCreateInfoCount = MAGMA_COUNT(queueDescriptors);
    deviceInfo.pQueueCreateInfos = queueDescriptors.data();
    deviceInfo.enabledLayerCount = MAGMA_COUNT(enabledLayers);
    deviceInfo.ppEnabledLayerNames = enabledLayers.data();
    deviceInfo.enabledExtensionCount = MAGMA_COUNT(enabledExtensions);
    deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();
    deviceInfo.pEnabledFeatures = extendedDeviceFeatures.empty() ? &deviceFeatures : nullptr;
    const VkResult result = vkCreateDevice(*(this->physicalDevice), &deviceInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    if (VK_ERROR_INITIALIZATION_FAILED == result)
        throw exception::InitializationFailed("initialization of logical device failed");
    MAGMA_THROW_FAILURE(result, "failed to create logical device");
    queues.reserve(queueDescriptors.size());
    for (const auto& desc : queueDescriptors)
        queues.emplace_back(desc, std::weak_ptr<Queue>());
    // Store enabled layers and extensions
    for (const auto& layer: enabledLayers)
        this->enabledLayers.emplace(layer);
    for (const auto& extension: enabledExtensions)
        this->enabledExtensions.emplace(extension);
}

Device::~Device()
{
    MAGMA_ASSERT(resourcePool->hasAnyDeviceResource() == false);
    vkDestroyDevice(handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

std::shared_ptr<Queue> Device::getQueue(VkQueueFlagBits flags, uint32_t queueIndex) const
{
    const DeviceQueueDescriptor queueDesc(physicalDevice, flags);
    for (auto& pair : queues)
    {   // Check if queue family is present, otherwise vkGetDeviceQueue() throws an exception
        if (pair.first.queueFamilyIndex == queueDesc.queueFamilyIndex)
        {
            if (!pair.second.expired())
                return pair.second.lock();
            // Get queue that supports specified flags
            VkQueue queueHandle = VK_NULL_HANDLE;
            vkGetDeviceQueue(handle, queueDesc.queueFamilyIndex, queueIndex, &queueHandle);
            if (VK_NULL_HANDLE == queueHandle)
                MAGMA_THROW("failed to get device queue");
            auto queue = std::shared_ptr<Queue>(new Queue(queueHandle,
                std::const_pointer_cast<Device>(shared_from_this()),
                flags, queueDesc.queueFamilyIndex, queueIndex));
            // Cache using weak_ptr to break circular references
            pair.second = queue;
            return queue;
        }
    }
    MAGMA_THROW("failed to get device queue");
}

std::shared_ptr<Queue> Device::getQueueForFamily(uint32_t queueFamilyIndex) const
{
    auto it = std::find_if(queues.begin(), queues.end(),
        [queueFamilyIndex](const auto& pair)
        {
            return (pair.first.queueFamilyIndex == queueFamilyIndex) &&
                !pair.second.expired();
        });
    if (it != queues.end())
        return it->second.lock();
    for (auto flag: {
        VK_QUEUE_GRAPHICS_BIT,
        VK_QUEUE_COMPUTE_BIT,
        VK_QUEUE_TRANSFER_BIT})
    {   // Try to get new instance
        try {
            auto queue = device->getQueue(flag, 0);
            if (queue->getFamilyIndex() == queueFamilyIndex)
                return queue;
        } catch (...) {
        }
    }
    MAGMA_THROW("failed to get device queue");
}

void Device::updateDescriptorWrites(const VkWriteDescriptorSet *descriptorWrites, uint32_t descriptorWriteCount) const noexcept
{
    vkUpdateDescriptorSets(handle, descriptorWriteCount, descriptorWrites, 0, nullptr);
}

void Device::updateDescriptorCopies(const VkCopyDescriptorSet *descriptorCopies, uint32_t descriptorCopyCount) const noexcept
{
    vkUpdateDescriptorSets(handle, 0, nullptr, descriptorCopyCount, descriptorCopies);
}

void Device::updateDescriptorSets(const std::vector<VkWriteDescriptorSet>& descriptorWrites,
    const std::vector<VkCopyDescriptorSet>& descriptorCopies /* {} */) const noexcept
{
    vkUpdateDescriptorSets(handle,
        MAGMA_COUNT(descriptorWrites), descriptorWrites.data(),
        MAGMA_COUNT(descriptorCopies), descriptorCopies.data());
}

bool Device::waitIdle() const
{
    const VkResult result = vkDeviceWaitIdle(handle);
    MAGMA_THROW_FAILURE(result, "failed to wait for device become idle");
    return true;
}

bool Device::resetFences(std::vector<std::shared_ptr<Fence>>& fences) const noexcept
{
    MAGMA_STACK_ARRAY(VkFence, dereferencedFences, fences.size());
    for (const auto& fence : fences)
        dereferencedFences.put(*fence);
    const VkResult result = vkResetFences(handle, dereferencedFences.size(), dereferencedFences);
    return (VK_SUCCESS == result);
}

bool Device::waitForFences(const std::vector<std::shared_ptr<Fence>>& fences, bool waitAll,
    uint64_t timeout /* std::numeric_limits<uint64_t>::max() */) const
{
    MAGMA_STACK_ARRAY(VkFence, dereferencedFences, fences.size());
    for (const auto& fence : fences)
        dereferencedFences.put(*fence);
    const VkResult result = vkWaitForFences(handle, dereferencedFences.size(), dereferencedFences, MAGMA_BOOLEAN(waitAll), timeout);
    MAGMA_THROW_FAILURE(result, "failed to wait for fences");
    // VK_SUCCESS or VK_TIMEOUT
    return (result != VK_TIMEOUT);
}

#ifdef VK_KHR_timeline_semaphore
bool Device::waitSemaphores(const std::vector<std::shared_ptr<TimelineSemaphore>>& semaphores,
    const std::vector<uint64_t>& values, bool waitAll,
    uint64_t timeout /* std::numeric_limits<uint64_t>::max() */) const
{
    MAGMA_STACK_ARRAY(VkSemaphore, dereferencedSemaphores, semaphores.size());
    for (const auto& semaphore: semaphores)
        dereferencedSemaphores.put(*semaphore);
    VkSemaphoreWaitInfo waitInfo;
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;
    waitInfo.pNext = nullptr;
    waitInfo.flags = waitAll ? 0 : VK_SEMAPHORE_WAIT_ANY_BIT_KHR;
    waitInfo.semaphoreCount = dereferencedSemaphores.size();
    waitInfo.pSemaphores = dereferencedSemaphores;
    waitInfo.pValues = values.data();
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkWaitSemaphoresKHR, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
    vkWaitSemaphoresKHR(handle, &waitInfo, timeout);
    const VkResult result = vkWaitSemaphoresKHR(MAGMA_HANDLE(device), &waitInfo, timeout);
    MAGMA_THROW_FAILURE(result, "failed to wait timeline semaphores");
    // VK_SUCCESS or VK_TIMEOUT
    return (result != VK_TIMEOUT);
}
#endif // VK_KHR_timeline_semaphore

#ifdef VK_KHR_acceleration_structure
bool Device::writeAccelerationStructuresProperties(const std::vector<std::shared_ptr<const AccelerationStructure>>& accelerationStructures,
    VkQueryType queryType, std::vector<VkDeviceSize>& properties) const
{
    MAGMA_ASSERT((VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR == queryType) ||
        (VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR == queryType) ||
        (VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR == queryType) ||
        (VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR == queryType));
    MAGMA_STACK_ARRAY(VkAccelerationStructureKHR, dereferencedAccelerationStructures, accelerationStructures.size());
    for (const auto& as: accelerationStructures)
        dereferencedAccelerationStructures.put(*as);
    properties.resize(accelerationStructures.size());
    const size_t dataSize = properties.size() * sizeof(VkDeviceSize);
    constexpr size_t stride = sizeof(VkDeviceSize);
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkWriteAccelerationStructuresPropertiesKHR, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    const VkResult result = vkWriteAccelerationStructuresPropertiesKHR(MAGMA_HANDLE(device), dereferencedAccelerationStructures.size(), dereferencedAccelerationStructures,
        queryType, dataSize, properties.data(), stride);
    return MAGMA_SUCCEEDED(result);
}

bool Device::getAccelerationStructureCompatibility(const uint8_t versionData[2 * VK_UUID_SIZE]) const noexcept
{
    VkAccelerationStructureVersionInfoKHR versionInfo;
    versionInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_VERSION_INFO_KHR;
    versionInfo.pNext = nullptr;
    versionInfo.pVersionData = versionData; // pVersionData must be a valid pointer to an array of 2 x VK_UUID_SIZE uint8_t values
    VkAccelerationStructureCompatibilityKHR compatibility = VK_ACCELERATION_STRUCTURE_COMPATIBILITY_INCOMPATIBLE_KHR;
    vkGetDeviceAccelerationStructureCompatibilityKHR(MAGMA_HANDLE(device), &versionInfo, &compatibility);
    return (VK_ACCELERATION_STRUCTURE_COMPATIBILITY_COMPATIBLE_KHR == compatibility);
}
#endif // VK_KHR_acceleration_structure

#ifdef VK_KHR_device_group
VkDeviceGroupPresentModeFlagsKHR Device::getDeviceGroupSurfacePresentModes(std::shared_ptr<const Surface> surface) const
{
    VkDeviceGroupPresentModeFlagsKHR presentModes = 0;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetDeviceGroupSurfacePresentModesKHR, VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
    const VkResult result = vkGetDeviceGroupSurfacePresentModesKHR(handle, *surface, &presentModes);
    MAGMA_THROW_FAILURE(result, "failed to get surface present modes for a device group");
    return presentModes;
}

#ifdef VK_EXT_full_screen_exclusive
VkDeviceGroupPresentModeFlagsKHR Device::getDeviceGroupSurfaceFullScreenExclusivePresentModes(std::shared_ptr<const Surface> surface,
    VkFullScreenExclusiveEXT fullScreenExclusive, void *hMonitor /* nullptr */) const
{
    VkDeviceGroupPresentModeFlagsKHR presentModes = 0;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VkSurfaceFullScreenExclusiveWin32InfoEXT fullScreenExclusiveWin32SurfaceInfo;
    fullScreenExclusiveWin32SurfaceInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
    fullScreenExclusiveWin32SurfaceInfo.pNext = nullptr;
    fullScreenExclusiveWin32SurfaceInfo.hmonitor = reinterpret_cast<HMONITOR>(hMonitor);
#endif // VK_USE_PLATFORM_WIN32_KHR
    VkSurfaceFullScreenExclusiveInfoEXT fullScreenExclusiveSurfaceInfo;
    fullScreenExclusiveSurfaceInfo.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    fullScreenExclusiveSurfaceInfo.pNext = hMonitor ? &fullScreenExclusiveWin32SurfaceInfo : nullptr;
#else
    fullScreenExclusiveSurfaceInfo.pNext = nullptr;
    MAGMA_UNUSED(hMonitor);
#endif
    fullScreenExclusiveSurfaceInfo.fullScreenExclusive = fullScreenExclusive;
#ifdef VK_KHR_get_surface_capabilities2
    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo;
    surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    surfaceInfo.pNext = &fullScreenExclusiveSurfaceInfo;
    surfaceInfo.surface = *surface;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetDeviceGroupSurfacePresentModes2EXT, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME);
    const VkResult result = vkGetDeviceGroupSurfacePresentModes2EXT(handle, &surfaceInfo, &presentModes);
    MAGMA_THROW_FAILURE(result, "failed to get full-screen exclusive surface present modes for a device group");
#endif // VK_KHR_get_surface_capabilities2
    return presentModes;
}
#endif // VK_EXT_full_screen_exclusive

VkPeerMemoryFeatureFlags Device::getDeviceGroupPeerMemoryFeatures(uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex) const
{
    VkPeerMemoryFeatureFlags peerMemoryFeatures;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetDeviceGroupPeerMemoryFeaturesKHR, VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
    vkGetDeviceGroupPeerMemoryFeaturesKHR(handle, heapIndex, localDeviceIndex, remoteDeviceIndex, &peerMemoryFeatures);
    return peerMemoryFeatures;
}
#endif // VK_KHR_device_group

#ifdef VK_EXT_device_fault
DeviceFaultInfo Device::getFaultInfo() const
{
    VkDeviceFaultCountsEXT deviceFaultCounts;
    deviceFaultCounts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;
    deviceFaultCounts.pNext = nullptr;
    deviceFaultCounts.addressInfoCount = 0;
    deviceFaultCounts.vendorInfoCount = 0;
    deviceFaultCounts.vendorBinarySize = 0ull;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetDeviceFaultInfoEXT, VK_EXT_DEVICE_FAULT_EXTENSION_NAME);
    VkResult result = vkGetDeviceFaultInfoEXT(handle, &deviceFaultCounts, nullptr);
    MAGMA_THROW_FAILURE(result, "failed to get device fault counts");
    DeviceFaultInfo deviceFaultInfo(deviceFaultCounts);
    result = vkGetDeviceFaultInfoEXT(handle, &deviceFaultCounts, &deviceFaultInfo);
    MAGMA_THROW_FAILURE(result, "failed to get device fault info");
    return deviceFaultInfo;
}
#endif // VK_EXT_device_fault

bool Device::extensionEnabled(const char *extensionName) const noexcept
{
    if (!physicalDevice->extensionSupported(extensionName))
        return false;
    const auto it = enabledExtensions.find(extensionName);
    return it != enabledExtensions.end();
}

bool Device::negativeViewportHeightEnabled(bool khronos) const noexcept
{
#if defined(VK_KHR_maintenance1) && defined(VK_AMD_negative_viewport_height)
    return extensionEnabled(khronos ?
        VK_KHR_MAINTENANCE1_EXTENSION_NAME :
        VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME);
#else
    MAGMA_UNUSED(khronos);
    return false;
#endif
}

bool Device::separateDepthStencilLayoutsEnabled() const noexcept
{
#ifdef VK_KHR_separate_depth_stencil_layouts
    auto separateDepthStencilFeatures = getEnabledExtendedFeatures<VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR>();
    if (separateDepthStencilFeatures)
        return (VK_TRUE == separateDepthStencilFeatures->separateDepthStencilLayouts);
#endif
    return false;
}

bool Device::stippledLinesEnabled() const noexcept
{
#ifdef VK_EXT_line_rasterization
    auto lineRasterizationFeatures = getEnabledExtendedFeatures<VkPhysicalDeviceLineRasterizationFeaturesEXT>();
    if (lineRasterizationFeatures)
    {
        return lineRasterizationFeatures->stippledRectangularLines ||
            lineRasterizationFeatures->stippledBresenhamLines ||
            lineRasterizationFeatures->stippledSmoothLines;
    }
#endif // VK_EXT_line_rasterization
    return false;
}
} // namespace magma
