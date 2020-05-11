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
#pragma once
#include "../core/noncopyable.h"

namespace magma
{
    class Buffer;
    class Image;
    class DeviceMemory;

    class ResourcePool final : public core::NonCopyable
    {
    public:
        template<typename Type>
        class ResourceSet final : public core::NonCopyable
        {
            std::set<const Type *> resources;

        public:
            void registerResource(const Type *resource) noexcept;
            void unregisterResource(const Type *resouce) noexcept;
            uint32_t resourceCount() const noexcept;
            void forEach(const std::function<void(const Type *resource)>& fn) const noexcept;
        };

        template<typename Type> ResourceSet<Type>& getResourceSet() noexcept;
        VkDeviceSize countAllocatedDeviceLocalMemory() const noexcept;
        VkDeviceSize countAllocatedHostVisibleMemory() const noexcept;

    private:
        ResourceSet<Buffer> buffers;
        ResourceSet<Image> images;
        ResourceSet<DeviceMemory> deviceMemories;
    };
} // namespace magma

#include "resourcePool.inl"
