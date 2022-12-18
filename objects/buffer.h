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
#pragma once
#include "resource.h"
#include "typedefs.h"

namespace magma
{
    class CommandBuffer;
    class IDeviceMemoryAllocator;
#ifdef VK_KHR_acceleration_structure
    class AccelerationStructure;
#endif
#ifdef VK_KHR_deferred_host_operations
    class DeferredOperation;
#endif

    /* Buffers represent linear arrays of data which are used
       for various purposes by binding them to a graphics or compute
       pipeline via descriptor sets or via certain commands,
       or by directly specifying them as parameters to certain commands. */

    class Buffer : public NonDispatchableResource<Buffer, VkBuffer>
    {
    public:
        struct Descriptor;

    public:
        ~Buffer();
        void realloc(VkDeviceSize newSize,
            std::shared_ptr<Allocator> allocator = nullptr);
        VkBufferCreateFlags getFlags() const noexcept { return flags; }
        VkBufferUsageFlags getUsage() const noexcept { return usage; }
        VkMemoryRequirements getMemoryRequirements() const noexcept;
        VkDescriptorBufferInfo getDescriptor() const noexcept;
    #if defined(VK_KHR_buffer_device_address) || defined(VK_EXT_buffer_device_address)
        VkDeviceAddress getDeviceAddress() const;
    #endif
        virtual void bindMemory(std::shared_ptr<DeviceMemory> memory,
            VkDeviceSize offset = 0) override;
#ifdef VK_KHR_device_group
        virtual void bindMemoryDeviceGroup(std::shared_ptr<DeviceMemory> memory,
            const std::vector<uint32_t>& deviceIndices,
            VkDeviceSize offset = 0) override;
#endif
    #ifdef VK_KHR_acceleration_structure
        bool copyToAccelerationStructure(std::shared_ptr<AccelerationStructure> accelerationStructure,
            VkCopyAccelerationStructureModeKHR mode,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr) const noexcept;
    #endif
        virtual void onDefragment() override;

    protected:
        explicit Buffer(std::shared_ptr<Device> device,
            VkDeviceSize size,
            VkBufferCreateFlags flags,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags memoryFlags,
            const Descriptor& optional,
            const Sharing& sharing,
            std::shared_ptr<Allocator> allocator);
        void copyHost(const void *data,
            CopyMemoryFunction copyFn) noexcept;
        void copyTransfer(std::shared_ptr<CommandBuffer> cmdBuffer,
            std::shared_ptr<const Buffer> srcBuffer,
            VkDeviceSize size = 0,
            VkDeviceSize srcOffset = 0,
            VkDeviceSize dstOffset = 0);

        const VkBufferCreateFlags flags;
        const VkBufferUsageFlags usage;
    };

    struct Buffer::Descriptor
    {
        VkBufferCreateFlags flags = 0;
        bool lazy = false;
        // VK_EXT_memory_priority
        float memoryPriority = MAGMA_DEFAULT_MEMORY_PRIORITY;
    };
} // namespace magma
