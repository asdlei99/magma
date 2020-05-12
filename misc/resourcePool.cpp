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

uint32_t ResourcePool::countGraphicsPipelines() const noexcept
{
    uint32_t graphicsPipelineCount = 0;
    pipelines.forEach([&graphicsPipelineCount](const Pipeline *pipeline) {
        if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipeline->getBindPoint())
            ++graphicsPipelineCount;
    });
    return graphicsPipelineCount;
}

uint32_t ResourcePool::countComputePipelines() const noexcept
{
    uint32_t computePipelineCount = 0;
    pipelines.forEach([&computePipelineCount](const Pipeline *pipeline) {
        if (VK_PIPELINE_BIND_POINT_COMPUTE == pipeline->getBindPoint())
            ++computePipelineCount;
    });
    return computePipelineCount;
}

uint32_t ResourcePool::countPrimaryCommandBuffers() const noexcept
{
    uint32_t primaryCmdBufferCount = 0;
    commandBuffers.forEach([&primaryCmdBufferCount](const CommandBuffer *cmdBuffer) {
        if (cmdBuffer->primary())
            ++primaryCmdBufferCount;
    });
    return primaryCmdBufferCount;
}

uint32_t ResourcePool::countSecondaryCommandBuffers() const noexcept
{
    uint32_t secondaryCmdBufferCount = 0;
    commandBuffers.forEach([&secondaryCmdBufferCount](const CommandBuffer *cmdBuffer) {
        if (!cmdBuffer->primary())
            ++secondaryCmdBufferCount;
    });
    return secondaryCmdBufferCount;
}

bool ResourcePool::hasAnyResource() const noexcept
{
    return deviceMemories.resourceCount() > 0 ||
        buffers.resourceCount() > 0 ||
        images.resourceCount() > 0 ||
        framebuffers.resourceCount() > 0 ||
        renderPasses.resourceCount() > 0 ||
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
