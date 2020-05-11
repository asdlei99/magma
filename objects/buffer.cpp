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
#include "buffer.h"
#include "srcTransferBuffer.h"
#include "device.h"
#include "deviceMemory.h"
#include "queue.h"
#include "fence.h"
#include "commandBuffer.h"
#include "../allocator/allocator.h"
#include "../misc/deviceExtension.h"
#include "../misc/exception.h"
#include "../core/copyMemory.h"

namespace magma
{
Buffer::Buffer(std::shared_ptr<Device> device, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags memFlags, VkBufferCreateFlags flags,
    const Sharing& sharing, std::shared_ptr<IAllocator> allocator):
    NonDispatchableResource(VK_OBJECT_TYPE_BUFFER, size, std::move(device), std::move(allocator)),
    usage(usage)
{
    VkBufferCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    info.size = size;
    info.usage = usage;
    info.sharingMode = sharing.getMode();
    info.queueFamilyIndexCount = sharing.getQueueFamiliesCount();
    info.pQueueFamilyIndices = sharing.getQueueFamilyIndices().data();
    const VkResult create = vkCreateBuffer(MAGMA_HANDLE(device), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create buffer");
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(MAGMA_HANDLE(device), handle, &memRequirements);
    std::shared_ptr<DeviceMemory> memory(std::make_shared<DeviceMemory>(
        this->device, memRequirements.size, memFlags));
    bindMemory(std::move(memory));
    MAGMA_REGISTER_RESOURCE(Buffer, this);
}

Buffer::~Buffer()
{
    MAGMA_UNREGISTER_RESOURCE(Buffer, this);
    vkDestroyBuffer(*device, handle, MAGMA_OPTIONAL_INSTANCE(allocator));
}

void Buffer::bindMemory(std::shared_ptr<DeviceMemory> memory,
    VkDeviceSize offset /* 0 */)
{
    const VkResult bind = vkBindBufferMemory(MAGMA_HANDLE(device), handle, *memory, offset);
    MAGMA_THROW_FAILURE(bind, "failed to bind buffer memory");
    this->offset = offset;
    this->memory = std::move(memory);
}

#ifdef VK_KHR_device_group
void Buffer::bindMemoryDeviceGroup(std::shared_ptr<DeviceMemory> memory,
    const std::vector<uint32_t>& deviceIndices,
    VkDeviceSize offset /* 0 */)
{
    VkBindBufferMemoryDeviceGroupInfo deviceGroupBindInfo;
    deviceGroupBindInfo.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO;
    deviceGroupBindInfo.pNext = nullptr;
    deviceGroupBindInfo.deviceIndexCount = MAGMA_COUNT(deviceIndices);
    deviceGroupBindInfo.pDeviceIndices = deviceIndices.data();
    VkBindBufferMemoryInfo bindInfo;
    bindInfo.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
    bindInfo.pNext = &deviceGroupBindInfo;
    bindInfo.buffer = handle;
    bindInfo.memory = *memory;
    bindInfo.memoryOffset = offset;
    MAGMA_DEVICE_EXTENSION(vkBindBufferMemory2KHR, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
    const VkResult bind = vkBindBufferMemory2KHR(MAGMA_HANDLE(device), 1, &bindInfo);
    MAGMA_THROW_FAILURE(bind, "failed to bind buffer memory within device group");
    this->memory = std::move(memory);
}
#endif // VK_KHR_device_group

VkMemoryRequirements Buffer::getMemoryRequirements() const noexcept
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(MAGMA_HANDLE(device), handle, &memRequirements);
    return memRequirements;
}

VkDescriptorBufferInfo Buffer::getDescriptor() const noexcept
{
    VkDescriptorBufferInfo info;
    info.buffer = handle;
    info.offset = 0;
    info.range = VK_WHOLE_SIZE;
    return info;
}

void Buffer::copyHost(const void *data, CopyMemoryFunction copyFn) noexcept
{
    void *buffer = memory->map();
    if (buffer)
    {
        if (!copyFn)
            copyFn = core::copyMemory;
        copyFn(buffer, data, static_cast<std::size_t>(size));
        memory->unmap();
    }
}

void Buffer::copyTransfer(std::shared_ptr<CommandBuffer> copyCmdBuffer,
    std::shared_ptr<SrcTransferBuffer> srcBuffer,
    VkDeviceSize srcOffset /* 0 */)
{
    copyCmdBuffer->begin();
    {
        VkBufferCopy region;
        region.srcOffset = srcOffset;
        region.dstOffset = 0;
        region.size = srcBuffer->getMemory()->getSize();
        vkCmdCopyBuffer(*copyCmdBuffer, *srcBuffer, handle, 1, &region);
    }
    copyCmdBuffer->end();
    std::shared_ptr<Queue> queue(device->getQueue(VK_QUEUE_TRANSFER_BIT, 0));
    std::shared_ptr<Fence> fence(copyCmdBuffer->getFence());
    if (!queue->submit(std::move(copyCmdBuffer), 0, nullptr, nullptr, fence))
        MAGMA_THROW("failed to submit command buffer to transfer queue");
    if (!fence->wait())
        MAGMA_THROW("failed to wait fence");
}
} // namespace magma
