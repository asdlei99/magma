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
#ifdef VK_NV_ray_tracing
    class AccelerationStructure;
#endif
    class Fence;
    class Event;
    class Semaphore;

    class ResourcePool final : public core::NonCopyable
    {
        friend DeviceMemory;
        friend Buffer;
        friend Image;
        friend Framebuffer;
        friend Pipeline;
        friend PipelineLayout;
        friend DescriptorSet;
        friend DescriptorSetLayout;
        friend CommandBuffer;
#ifdef VK_NV_ray_tracing
        friend AccelerationStructure;
#endif
        friend Fence;
        friend Event;
        friend Semaphore;

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

    public:
        struct InstanceCount
        {
            uint32_t deviceMemoryCount = 0;
            uint32_t bufferCount = 0;
            uint32_t imageCount = 0;
            uint32_t framebufferCount = 0;
            uint32_t graphicsPipelineCount = 0;
            uint32_t computePipelineCount = 0;
            uint32_t rayTracingPipelineCount = 0;
            uint32_t pipelineLayoutCount = 0;
            uint32_t descriptorSetCount = 0;
            uint32_t descriptorSetLayoutCount = 0;
            uint32_t primaryCommandBufferCount = 0;
            uint32_t secondaryCommandBufferCount = 0;
            uint32_t accelerationStructureCount = 0;
            uint32_t fenceCount = 0;
            uint32_t eventCount = 0;
            uint32_t semaphoreCount = 0;
        };

        InstanceCount countResourceInstances() const noexcept;
        VkDeviceSize countAllocatedDeviceLocalMemory() const noexcept;
        VkDeviceSize countAllocatedHostVisibleMemory() const noexcept;
        bool hasAnyResource() const noexcept;

    private:
        template<typename Type> ResourceSet<Type>& getAccessor() noexcept;

        ResourceSet<DeviceMemory> deviceMemories;
        ResourceSet<Buffer> buffers;
        ResourceSet<Image> images;
        ResourceSet<Framebuffer> framebuffers;
        ResourceSet<Pipeline> pipelines;
        ResourceSet<PipelineLayout> pipelineLayouts;
        ResourceSet<DescriptorSet> descriptorSets;
        ResourceSet<DescriptorSetLayout> descriptorSetLayouts;
        ResourceSet<CommandBuffer> commandBuffers;
#ifdef VK_NV_ray_tracing
        ResourceSet<AccelerationStructure> accelerationStructures;
#endif
        ResourceSet<Fence> fences;
        ResourceSet<Event> events;
        ResourceSet<Semaphore> semaphores;
    };
} // namespace magma

#include "resourcePool.inl"
