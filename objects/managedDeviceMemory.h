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
#include "deviceMemory.h"

namespace magma
{
    class Allocator;
    class IDeviceMemoryAllocator;

    /* Opaque handle to memory sub-allocation.
       Each device memory allocator hides the implementation details under it. */
    typedef void *DeviceMemoryBlock;

    class ManagedDeviceMemory : public DeviceMemory
    {
    public:
        explicit ManagedDeviceMemory(std::shared_ptr<Device> device,
            const VkMemoryRequirements& memoryRequirements,
            VkMemoryPropertyFlags flags,
            float priority,
            const void *object,
            VkObjectType objectType,
            std::shared_ptr<Allocator> allocator = nullptr);
        ~ManagedDeviceMemory();
        std::shared_ptr<IDeviceMemoryAllocator> getDeviceAllocator() const noexcept { return deviceAllocator; }
        DeviceMemoryBlock getAllocation() const noexcept { return block; }
        void realloc(VkDeviceSize newSize,
            float priority,
            const void *object,
            VkObjectType objectType) override;
        void bind(const void *object,
            VkObjectType objectType,
            VkDeviceSize offset /* 0 */) override;
        void *map(VkDeviceSize offset = 0,
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkMemoryMapFlags flags = 0) noexcept override;
        void unmap() noexcept override;
        bool flushMappedRange(VkDeviceSize offset = 0,
            VkDeviceSize size = VK_WHOLE_SIZE) noexcept override;
        bool invalidateMappedRange(VkDeviceSize offset = 0,
            VkDeviceSize size = VK_WHOLE_SIZE) noexcept override;
        void onDefragment() noexcept override;

    private:
        DeviceMemoryBlock block;
        std::shared_ptr<IDeviceMemoryAllocator> deviceAllocator;
    };
} // namespace magma

