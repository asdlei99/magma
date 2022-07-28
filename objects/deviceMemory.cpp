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
#include "deviceMemory.h"
#include "device.h"
#include "physicalDevice.h"
#include "../allocator/allocator.h"
#include "../misc/extProcAddress.h"
#include "../exceptions/errorResult.h"

namespace magma
{
DeviceMemory::DeviceMemory(std::shared_ptr<Device> device,
    const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags flags, float priority,
    std::shared_ptr<IAllocator> allocator, int /* overloading */):
    NonDispatchable(VK_OBJECT_TYPE_DEVICE_MEMORY, std::move(device), std::move(allocator)),
    memoryRequirements(memoryRequirements),
    flags(flags),
    priority(priority),
    subOffset(0ull),
    mappedRange(nullptr)
{}

DeviceMemory::DeviceMemory(std::shared_ptr<Device> device_,
    const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags flags, float priority,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_DEVICE_MEMORY, std::move(device_), std::move(allocator)),
    memoryRequirements(memoryRequirements),
    flags(flags),
    priority(priority),
    subOffset(0ull),
    mappedRange(nullptr)
{
    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = getTypeIndex(flags);
#ifdef VK_EXT_memory_priority
    VkMemoryPriorityAllocateInfoEXT memoryPriorityInfo;
    if (device->extensionEnabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
    {
        MAGMA_ASSERT((priority >= 0.f) && (priority <= 1.f));
        memoryPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        memoryPriorityInfo.pNext = nullptr;
        memoryPriorityInfo.priority = priority;
        allocInfo.pNext = &memoryPriorityInfo;
    }
#endif // VK_EXT_memory_priority
    const VkResult result = vkAllocateMemory(MAGMA_HANDLE(device), &allocInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to allocate device memory");
}

#ifdef VK_KHR_device_group
DeviceMemory::DeviceMemory(std::shared_ptr<Device> device_, uint32_t deviceMask,
    const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags flags, float priority,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_DEVICE_MEMORY, std::move(device_), std::move(allocator)),
    memoryRequirements(memoryRequirements),
    flags(flags),
    priority(priority),
    subOffset(0ull),
    mappedRange(nullptr)
{
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo;
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
    allocFlagsInfo.pNext = nullptr;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT_KHR;
    allocFlagsInfo.deviceMask = deviceMask;
    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = &allocFlagsInfo;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = getTypeIndex(flags);
#ifdef VK_EXT_memory_priority
    VkMemoryPriorityAllocateInfoEXT memoryPriorityInfo;
    if (device->extensionEnabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
    {
        MAGMA_ASSERT((priority >= 0.f) && (priority <= 1.f));
        memoryPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        memoryPriorityInfo.pNext = nullptr;
        memoryPriorityInfo.priority = priority;
        allocFlagsInfo.pNext = &memoryPriorityInfo;
    }
#endif // VK_EXT_memory_priority
    const VkResult result = vkAllocateMemory(MAGMA_HANDLE(device), &allocInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to allocate device memory within device group");
}
#endif // VK_KHR_device_group

DeviceMemory::~DeviceMemory()
{
    MAGMA_ASSERT(!mappedRange);
    if (handle != VK_NULL_HANDLE)
        vkFreeMemory(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

void DeviceMemory::setPriority(float priority_) noexcept
{
    MAGMA_ASSERT((priority_ >= 0.f) && (priority_ <= 1.f));
    MAGMA_UNUSED(priority_);
#ifdef VK_EXT_pageable_device_local_memory
    MAGMA_DEVICE_EXTENSION(vkSetDeviceMemoryPriorityEXT);
    if (vkSetDeviceMemoryPriorityEXT)
    {
        vkSetDeviceMemoryPriorityEXT(MAGMA_HANDLE(device), handle, priority_);
        priority = priority_;
    }
#endif // VK_EXT_pageable_device_local_memory
}

void DeviceMemory::realloc(VkDeviceSize newSize, float priority, const void *object, VkObjectType objectType)
{
    MAGMA_ASSERT(!mappedRange);
    MAGMA_UNUSED(object);
    MAGMA_UNUSED(objectType);
    vkFreeMemory(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
    handle = VK_NULL_HANDLE;
    memoryRequirements.size = newSize;
    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = newSize;
    allocInfo.memoryTypeIndex = getTypeIndex(flags);
#ifdef VK_EXT_memory_priority
    VkMemoryPriorityAllocateInfoEXT memoryPriorityInfo;
    if (device->extensionEnabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
    {
        MAGMA_ASSERT((priority >= 0.f) && (priority <= 1.f));
        memoryPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        memoryPriorityInfo.pNext = nullptr;
        memoryPriorityInfo.priority = priority;
        allocInfo.pNext = &memoryPriorityInfo;
    }
#endif // VK_EXT_memory_priority
    const VkResult result = vkAllocateMemory(MAGMA_HANDLE(device), &allocInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to allocate device memory");
}

void DeviceMemory::bind(const void *object, VkObjectType objectType,
    VkDeviceSize offset /* 0 */)
{
    MAGMA_ASSERT((VK_OBJECT_TYPE_BUFFER == objectType) || (VK_OBJECT_TYPE_IMAGE == objectType));
    VkResult result;
    if (VK_OBJECT_TYPE_BUFFER == objectType)
        result = vkBindBufferMemory(MAGMA_HANDLE(device), MAGMA_BUFFER_HANDLE(object), handle, offset);
    else // VK_OBJECT_TYPE_IMAGE
        result = vkBindImageMemory(MAGMA_HANDLE(device), MAGMA_IMAGE_HANDLE(object), handle, offset);
    MAGMA_THROW_FAILURE(result, VK_OBJECT_TYPE_BUFFER == objectType
        ? "failed to bind buffer memory"
        : "failed to bind image memory");
}

void *DeviceMemory::map(
    VkDeviceSize offset /* 0 */,
    VkDeviceSize size /* VK_WHOLE_SIZE */,
    VkMemoryMapFlags flags /* 0 */) noexcept
{
    MAGMA_ASSERT(hostVisible());
    if (!mappedRange)
    {
        const VkResult result = vkMapMemory(MAGMA_HANDLE(device), handle, offset, size, flags, &mappedRange);
        if (result != VK_SUCCESS)
        {   // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            // VK_ERROR_MEMORY_MAP_FAILED
            return nullptr;
        }
    }
    return mappedRange;
}

void DeviceMemory::unmap() noexcept
{
    MAGMA_ASSERT(hostVisible());
    if (mappedRange)
    {
        vkUnmapMemory(*device, handle);
        mappedRange = nullptr;
    }
}

bool DeviceMemory::flushMappedRange(
    VkDeviceSize offset /* 0 */,
    VkDeviceSize size /* VK_WHOLE_SIZE */) noexcept
{
    VkMappedMemoryRange memoryRange;
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.pNext = nullptr;
    memoryRange.memory = handle;
    memoryRange.offset = offset;
    memoryRange.size = size;
    const VkResult result = vkFlushMappedMemoryRanges(MAGMA_HANDLE(device), 1, &memoryRange);
    return (VK_SUCCESS == result);
}

bool DeviceMemory::invalidateMappedRange(
    VkDeviceSize offset /* 0 */,
    VkDeviceSize size /* VK_WHOLE_SIZE */) noexcept
{
    VkMappedMemoryRange memoryRange;
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.pNext = nullptr;
    memoryRange.memory = handle;
    memoryRange.offset = offset;
    memoryRange.size = size;
    const VkResult result = vkInvalidateMappedMemoryRanges(MAGMA_HANDLE(device), 1, &memoryRange);
    return (VK_SUCCESS == result);
}

void DeviceMemory::onDefragment() noexcept
{
}

uint32_t DeviceMemory::getTypeIndex(VkMemoryPropertyFlags flags) const
{
    const VkPhysicalDeviceMemoryProperties& properties = this->device->getPhysicalDevice()->getMemoryProperties();
    for (uint32_t memTypeIndex = 0; memTypeIndex < properties.memoryTypeCount; ++memTypeIndex)
    {   // Try exact match
        const VkMemoryType& memType = properties.memoryTypes[memTypeIndex];
        if (memType.propertyFlags == flags)
            return memTypeIndex;
    }
    for (uint32_t memTypeIndex = 0; memTypeIndex < properties.memoryTypeCount; ++memTypeIndex)
    {   // Try any suitable memory type
        const VkMemoryType& memType = properties.memoryTypes[memTypeIndex];
        if ((memType.propertyFlags & flags) == flags)
            return memTypeIndex;
    }
    MAGMA_THROW("failed to find suitable memory type");
}
} // namespace magma
