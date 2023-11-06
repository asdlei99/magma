/*
Magma - Abstraction layer over Khronos Vulkan API.
Copyright (C) 2018-2023 Victor Coda.

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
#include "pipeline.h"
#include "../shaders/rayTracingShaderGroup.h"

namespace magma
{
    class PipelineShaderStage;
    class PipelineCache;
#ifdef VK_KHR_deferred_host_operations
    class DeferredOperation;
#endif

    /* Raytracing pipelines consist of multiple shader stages,
       fixed-function traversal stages, and a pipeline layout. */

#ifdef VK_KHR_ray_tracing_pipeline
    class RayTracingPipeline : public Pipeline
    {
    public:
        explicit RayTracingPipeline(std::shared_ptr<Device> device,
            const std::vector<PipelineShaderStage>& shaderStages,
            const std::vector<RayTracingShaderGroup>& shaderGroups,
            uint32_t maxRayRecursionDepth,
            std::shared_ptr<PipelineLayout> layout,
            const std::vector<VkDynamicState>& dynamicStates,
            std::shared_ptr<IAllocator> allocator = nullptr,
            std::shared_ptr<PipelineCache> pipelineCache = nullptr,
            std::shared_ptr<RayTracingPipeline> basePipeline = nullptr,
            std::shared_ptr<DeferredOperation> deferredOp = nullptr,
            VkPipelineCreateFlags flags = 0,
            const StructureChain& extendedInfo = StructureChain());
        uint32_t getShaderGroupCount() const noexcept { return shaderGroupCount; }
        VkDeviceSize getGeneralShaderStackSize(uint32_t group) const noexcept;
        VkDeviceSize getClosestHitShaderStackSize(uint32_t group) const noexcept;
        VkDeviceSize getAnyHitShaderStackSize(uint32_t group) const noexcept;
        VkDeviceSize getIntersectionShaderStackSize(uint32_t group) const noexcept;
        std::vector<uint8_t> getShaderGroupHandles() const;
        std::vector<uint8_t> getShaderGroupHandles(uint32_t firstGroup,
            uint32_t groupCount) const;
        std::vector<uint8_t> getCaptureReplayShaderGroupHandles() const;
        std::vector<uint8_t> getCaptureReplayShaderGroupHandles(uint32_t firstGroup,
            uint32_t groupCount) const;

    private:
        VkDeviceSize getShaderGroupStackSize(uint32_t group,
            VkShaderGroupShaderKHR groupShader) const noexcept;

        const VkPipelineCreateFlags flags; // TODO: move to pipeline
        const uint32_t shaderGroupCount;
    };
#endif // VK_KHR_ray_tracing_pipeline
} // namespace magma
