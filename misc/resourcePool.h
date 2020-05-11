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
    class DeviceMemory;
    class Buffer;
    class Image;
    class Framebuffer;
    class Pipeline;
    class PipelineLayout;
    class DescriptorSet;
    class DescriptorSetLayout;
    class CommandBuffer;

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

        template<typename Type>
        ResourceSet<Type>& getResourceSet() noexcept;
        template<typename Type>
        const ResourceSet<Type>& getResourceSet() const noexcept;
        VkDeviceSize countAllocatedDeviceLocalMemory() const noexcept;
        VkDeviceSize countAllocatedHostVisibleMemory() const noexcept;

    private:
        ResourceSet<DeviceMemory> deviceMemories;
        ResourceSet<Buffer> buffers;
        ResourceSet<Image> images;
        ResourceSet<Framebuffer> framebuffers;
        ResourceSet<Pipeline> pipelines;
        ResourceSet<PipelineLayout> pipelineLayouts;
        ResourceSet<DescriptorSet> descriptorSets;
        ResourceSet<DescriptorSetLayout> descriptorSetLayouts;
        ResourceSet<CommandBuffer> commandBuffers;
    };
} // namespace magma

#include "resourcePool.inl"
