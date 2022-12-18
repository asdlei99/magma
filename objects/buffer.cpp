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
#include "buffer.h"
#include "device.h"
#include "deviceMemory.h"
#include "queue.h"
#include "fence.h"
#include "commandBuffer.h"
#include "accelerationStructureKhr.h"
#include "deferredOperation.h"
#include "../misc/extProcAddress.h"
#include "../exceptions/errorResult.h"
#include "../core/copyMemory.h"

namespace magma
{
Buffer::Buffer(std::shared_ptr<Device> device, VkDeviceSize size,
    VkBufferCreateFlags flags, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags,
    const Descriptor& optional, const Sharing& sharing, std::shared_ptr<Allocator> allocator):
    NonDispatchableResource(VK_OBJECT_TYPE_BUFFER, device, sharing, allocator),
    flags(flags),
    usage(usage)
{
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = flags | optional.flags;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharing.getMode();
    bufferInfo.queueFamilyIndexCount = sharing.getQueueFamiliesCount();
    bufferInfo.pQueueFamilyIndices = sharing.getQueueFamilyIndices().data();
    const VkResult result = vkCreateBuffer(MAGMA_HANDLE(device), &bufferInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create buffer");
    const VkMemoryRequirements memoryRequirements = getMemoryRequirements();
    if (optional.lazy && !(memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        memoryFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    std::shared_ptr<DeviceMemory> memory = std::make_shared<DeviceMemory>(
        std::move(device),
        memoryRequirements, memoryFlags, optional.memoryPriority,
        &handle, VK_OBJECT_TYPE_BUFFER,
        std::move(allocator));
    bindMemory(std::move(memory));
}

Buffer::~Buffer()
{
    vkDestroyBuffer(*device, handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

void Buffer::realloc(VkDeviceSize newSize,
    std::shared_ptr<Allocator> allocator /* nullptr */)
{
    if (getSize() == newSize)
        return;
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = flags;
    bufferInfo.size = newSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharing.getMode();
    bufferInfo.queueFamilyIndexCount = sharing.getQueueFamiliesCount();
    bufferInfo.pQueueFamilyIndices = sharing.getQueueFamilyIndices().data();
    vkDestroyBuffer(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
    hostAllocator = MAGMA_HOST_ALLOCATOR(allocator);
    deviceAllocator = MAGMA_DEVICE_ALLOCATOR(allocator);
    const VkResult result = vkCreateBuffer(MAGMA_HANDLE(device), &bufferInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to reallocate buffer");
    memory->realloc(newSize, memory->getPriority(), &handle, VK_OBJECT_TYPE_BUFFER, std::move(allocator));
    bindMemory(std::move(memory), offset);
}

VkMemoryRequirements Buffer::getMemoryRequirements() const noexcept
{
    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(MAGMA_HANDLE(device), handle, &memoryRequirements);
    return memoryRequirements;
}

VkDescriptorBufferInfo Buffer::getDescriptor() const noexcept
{
    VkDescriptorBufferInfo bufferDescriptorInfo;
    bufferDescriptorInfo.buffer = handle;
    bufferDescriptorInfo.offset = 0;
    bufferDescriptorInfo.range = VK_WHOLE_SIZE;
    return bufferDescriptorInfo;
}

#if defined(VK_KHR_buffer_device_address) || defined(VK_EXT_buffer_device_address)
VkDeviceAddress Buffer::getDeviceAddress() const
{
#ifdef VK_KHR_buffer_device_address
    MAGMA_DEVICE_EXTENSION(vkGetBufferDeviceAddressKHR);
    if (vkGetBufferDeviceAddressKHR)
    {
        VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo;
        bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
        bufferDeviceAddressInfo.pNext = nullptr;
        bufferDeviceAddressInfo.buffer = handle;
        return vkGetBufferDeviceAddressKHR(MAGMA_HANDLE(device), &bufferDeviceAddressInfo);
    }
#endif // VK_KHR_buffer_device_address
#ifdef VK_EXT_buffer_device_address
    VkBufferDeviceAddressInfoEXT bufferDeviceAddressInfo;
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_EXT;
    bufferDeviceAddressInfo.pNext = nullptr;
    bufferDeviceAddressInfo.buffer = handle;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetBufferDeviceAddressEXT, VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    return vkGetBufferDeviceAddressEXT(MAGMA_HANDLE(device), &bufferDeviceAddressInfo);
#else
    return 0ull;
#endif // VK_EXT_buffer_device_address
}
#endif // VK_KHR_buffer_device_address || VK_EXT_buffer_device_address

void Buffer::bindMemory(std::shared_ptr<DeviceMemory> memory,
    VkDeviceSize offset /* 0 */)
{
    memory->bind(&handle, VK_OBJECT_TYPE_BUFFER, offset);
    this->size = memory->getSize();
    this->offset = offset;
    this->memory = std::move(memory);
}

#ifdef VK_KHR_device_group
void Buffer::bindMemoryDeviceGroup(std::shared_ptr<DeviceMemory> memory,
    const std::vector<uint32_t>& deviceIndices,
    VkDeviceSize offset /* 0 */)
{
    VkBindBufferMemoryInfoKHR bindMemoryInfo;
    VkBindBufferMemoryDeviceGroupInfoKHR bindMemoryDeviceGroupInfo;
    bindMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHR;
    bindMemoryInfo.pNext = &bindMemoryDeviceGroupInfo;
    bindMemoryInfo.buffer = handle;
    bindMemoryInfo.memory = *memory;
    bindMemoryInfo.memoryOffset = memory->getOffset() + offset;
    bindMemoryDeviceGroupInfo.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO_KHR;
    bindMemoryDeviceGroupInfo.pNext = nullptr;
    bindMemoryDeviceGroupInfo.deviceIndexCount = MAGMA_COUNT(deviceIndices);
    bindMemoryDeviceGroupInfo.pDeviceIndices = deviceIndices.data();
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkBindBufferMemory2KHR, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
    const VkResult result = vkBindBufferMemory2KHR(MAGMA_HANDLE(device), 1, &bindMemoryInfo);
    MAGMA_THROW_FAILURE(result, "failed to bind buffer memory within device group");
    this->size = memory->getSize();
    this->offset = offset;
    this->memory = std::move(memory);
}
#endif // VK_KHR_device_group

#ifdef VK_KHR_acceleration_structure
bool Buffer::copyToAccelerationStructure(std::shared_ptr<AccelerationStructure> accelerationStructure,
    VkCopyAccelerationStructureModeKHR mode,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) const noexcept
{
    VkCopyMemoryToAccelerationStructureInfoKHR copyMemoryToAccelerationStructureInfo;
    copyMemoryToAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
    copyMemoryToAccelerationStructureInfo.pNext = nullptr;
    copyMemoryToAccelerationStructureInfo.src.deviceAddress = getDeviceAddress();
    copyMemoryToAccelerationStructureInfo.dst = *accelerationStructure;
    copyMemoryToAccelerationStructureInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCopyMemoryToAccelerationStructureKHR);
    const VkResult result = vkCopyMemoryToAccelerationStructureKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation), &copyMemoryToAccelerationStructureInfo);
    return MAGMA_SUCCEEDED(result);
}
#endif // VK_KHR_acceleration_structure

void Buffer::onDefragment()
{
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = flags;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = sharing.getMode();
    bufferInfo.queueFamilyIndexCount = sharing.getQueueFamiliesCount();
    bufferInfo.pQueueFamilyIndices = sharing.getQueueFamilyIndices().data();
    vkDestroyBuffer(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
    const VkResult result = vkCreateBuffer(MAGMA_HANDLE(device), &bufferInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to recreate defragmented buffer");
    bindMemory(std::move(memory), offset);
}

void Buffer::copyHost(const void *data, CopyMemoryFunction copyFn) noexcept
{
    if (void *buffer = memory->map())
    {
        if (!copyFn)
            copyFn = core::copyMemory;
        copyFn(buffer, data, static_cast<std::size_t>(size));
        memory->unmap();
    }
}

void Buffer::copyTransfer(std::shared_ptr<CommandBuffer> cmdBuffer, std::shared_ptr<const Buffer> srcBuffer,
    VkDeviceSize size /* 0 */,
    VkDeviceSize srcOffset /* 0 */,
    VkDeviceSize dstOffset /* 0 */)
{
    VkBufferCopy region;
    region.srcOffset = srcOffset;
    region.dstOffset = dstOffset;
    if (!size)
        size = srcBuffer->getSize() - srcOffset;
    region.size = std::min(this->getSize(), size);
    // We couldn't call shared_from_this() from ctor, so use custom ref object w/ empty deleter
    const auto weakThis = std::shared_ptr<Buffer>(this, [](Buffer *) {});
    cmdBuffer->copyBuffer(srcBuffer, weakThis, region);
}
} // namespace magma
