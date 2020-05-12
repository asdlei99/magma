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
#include "resourcePool.h"
#include "../objects/deviceMemory.h"
#include "../objects/pipeline.h"
#include "../objects/commandBuffer.h"

namespace magma
{
ResourcePool::InstanceCount ResourcePool::countResourceInstances() const noexcept
{
    InstanceCount instances;
    instances.deviceMemoryCount = deviceMemories.resourceCount();
    instances.bufferCount = buffers.resourceCount();
    instances.imageCount = images.resourceCount();
    instances.framebufferCount = framebuffers.resourceCount();
    pipelines.forEach([&instances](const Pipeline *pipeline) {
        switch (pipeline->getBindPoint())
        {
        case VK_PIPELINE_BIND_POINT_GRAPHICS:
            ++instances.graphicsPipelineCount; break;
        case VK_PIPELINE_BIND_POINT_COMPUTE:
            ++instances.computePipelineCount; break;
#ifdef VK_NV_ray_tracing
        case VK_PIPELINE_BIND_POINT_RAY_TRACING_NV:
            ++instances.rayTracingPipelineCount; break;
#endif
        }
    });
    instances.pipelineLayoutCount = pipelineLayouts.resourceCount();
    instances.descriptorSetCount = descriptorSets.resourceCount();
    instances.descriptorSetLayoutCount = descriptorSetLayouts.resourceCount();
    commandBuffers.forEach([&instances](const CommandBuffer *cmdBuffer) {
        if (cmdBuffer->primary())
            ++instances.primaryCommandBufferCount;
        else
            ++instances.secondaryCommandBufferCount;
    });
#ifdef VK_NV_ray_tracing
    instances.accelerationStructureCount = accelerationStructures.resourceCount();
#endif
    return instances;
}

VkDeviceSize ResourcePool::countAllocatedDeviceLocalMemory() const noexcept
{
    VkDeviceSize allocatedSize = 0;
    deviceMemories.forEach([&allocatedSize](const DeviceMemory *memory) {
        if (memory->local())
            allocatedSize += memory->getSize();
    });
    return allocatedSize;
}

VkDeviceSize ResourcePool::countAllocatedHostVisibleMemory() const noexcept
{
    VkDeviceSize allocatedSize = 0;
    deviceMemories.forEach([&allocatedSize](const DeviceMemory *memory) {
        if (memory->hostVisible())
            allocatedSize += memory->getSize();
    });
    return allocatedSize;
}

bool ResourcePool::hasAnyResource() const noexcept
{
    return deviceMemories.resourceCount() > 0 ||
        buffers.resourceCount() > 0 ||
        images.resourceCount() > 0 ||
        framebuffers.resourceCount() > 0 ||
        pipelines.resourceCount() > 0 ||
        pipelineLayouts.resourceCount() > 0 ||
        descriptorSets.resourceCount() > 0 ||
        descriptorSetLayouts.resourceCount() > 0 ||
        commandBuffers.resourceCount() > 0 ||
#ifdef VK_NV_ray_tracing
        accelerationStructures.resourceCount() > 0 ||
#endif
        false;
}
} // namespace magma
