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
#include "managedDeviceMemory.h"
#include "../allocator/allocator.h"
#include "../exceptions/errorResult.h"

namespace magma
{
ManagedDeviceMemory::ManagedDeviceMemory(std::shared_ptr<Device> device,
    const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags flags, float priority,
    const void *object, VkObjectType objectType,
    std::shared_ptr<Allocator> allocator /* nullptr */):
    DeviceMemory(std::move(device), memoryRequirements, flags, priority, MAGMA_HOST_ALLOCATOR(allocator), 0),
    deviceAllocator(MAGMA_DEVICE_ALLOCATOR(allocator))
{
    MAGMA_ASSERT(deviceAllocator != nullptr);
    block = deviceAllocator->alloc(memoryRequirements, flags, priority, object, objectType, &handle);
}

ManagedDeviceMemory::~ManagedDeviceMemory()
{
    MAGMA_ASSERT(!mappedRange);
    deviceAllocator->free(block);
}

void ManagedDeviceMemory::realloc(VkDeviceSize newSize, float priority, const void *object, VkObjectType objectType)
{
    MAGMA_ASSERT(!mappedRange);
    memoryRequirements.size = newSize;
    deviceAllocator->free(block);
    block = deviceAllocator->alloc(memoryRequirements, flags, priority, object, objectType);
    onDefragment();
}

void ManagedDeviceMemory::bind(const void *object, VkObjectType objectType,
    VkDeviceSize offset_ /* 0 */)
{
    MAGMA_ASSERT((VK_OBJECT_TYPE_BUFFER == objectType) || (VK_OBJECT_TYPE_IMAGE == objectType));
    VkResult result = deviceAllocator->bindMemory(block, offset_, object, objectType);
    MAGMA_THROW_FAILURE(result, VK_OBJECT_TYPE_BUFFER == objectType
        ? "failed to bind buffer memory"
        : "failed to bind image memory");
    offset = offset_;
}

void *ManagedDeviceMemory::map(
    VkDeviceSize offset /* 0 */,
    VkDeviceSize size /* VK_WHOLE_SIZE */,
    VkMemoryMapFlags flags /* 0 */) noexcept
{
    MAGMA_UNUSED(size);
    MAGMA_UNUSED(flags);
    if (!mappedRange)
    {
        const VkResult result = deviceAllocator->map(block, offset, &mappedRange);
        if (result != VK_SUCCESS)
        {   // VK_ERROR_OUT_OF_HOST_MEMORY
            // VK_ERROR_OUT_OF_DEVICE_MEMORY
            // VK_ERROR_MEMORY_MAP_FAILED
            return nullptr;
        }
    }
    return mappedRange;
}

void ManagedDeviceMemory::unmap() noexcept
{
    if (mappedRange)
    {
        deviceAllocator->unmap(block);
        mappedRange = nullptr;
    }
}

bool ManagedDeviceMemory::flushMappedRange(
    VkDeviceSize offset /* 0 */,
    VkDeviceSize size /* VK_WHOLE_SIZE */) noexcept
{
    const VkResult result = deviceAllocator->flushMappedRange(block, offset, size);
    return (VK_SUCCESS == result);
}

bool ManagedDeviceMemory::invalidateMappedRange(
    VkDeviceSize offset /* 0 */,
    VkDeviceSize size /* VK_WHOLE_SIZE */) noexcept
{
    VkResult result = deviceAllocator->invalidateMappedRange(block, offset, size);
    return (VK_SUCCESS == result);
}
} // namespace magma
