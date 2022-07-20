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
#include "../core/noncopyable.h"
#include "../shaders/rayTracingShaderGroup.h"

namespace magma
{
    class Device;
    class Pipeline;
    class PipelineLayout;
    class PipelineCache;
    class PipelineShaderStage;
    class GraphicsPipeline;
    class ComputePipeline;
    class RayTracingPipeline;
    class RenderPass;
    class IAllocator;

    class VertexInputState;
    struct InputAssemblyState;
    struct TesselationState;
    struct RasterizationState;
    struct MultisampleState;
    struct DepthStencilState;
    struct ColorBlendState;
    class ViewportState;

    /* Vulkan has an ability to create multiple pipeline objects in a single call.
       This is a base class for such implementation. */

    class PipelineCompiler final : core::NonCopyable
    {
    public:
        uint32_t newGraphicsPipeline(const std::vector<PipelineShaderStage>& shaderStages,
            const VertexInputState& vertexInputState,
            const InputAssemblyState& inputAssemblyState,
            const TesselationState& tesselationState,
            const ViewportState& viewportState,
            const RasterizationState& rasterizationState,
            const MultisampleState& multisampleState,
            const DepthStencilState& depthStencilState,
            const ColorBlendState& colorBlendState,
            const std::vector<VkDynamicState>& dynamicStates,
            std::shared_ptr<PipelineLayout> layout,
            std::shared_ptr<RenderPass> renderPass,
            uint32_t subpass,
            std::shared_ptr<GraphicsPipeline> basePipeline = nullptr,
            VkPipelineCreateFlags flags = 0);
        uint32_t newComputePipeline(const PipelineShaderStage& shaderStage,
            std::shared_ptr<PipelineLayout> layout,
            std::shared_ptr<ComputePipeline> basePipeline = nullptr,
            VkPipelineCreateFlags flags = 0);
    #ifdef VK_NV_ray_tracing
        uint32_t newRayTracingPipeline(const std::vector<PipelineShaderStage>& shaderStages,
            const std::vector<RayTracingShaderGroup>& shaderGroups,
            uint32_t maxRecursionDepth,
            std::shared_ptr<PipelineLayout> layout,
            std::shared_ptr<RayTracingPipeline> basePipeline = nullptr,
            VkPipelineCreateFlags flags = 0);
    #endif // VK_NV_ray_tracing
        void buildPipelines(std::shared_ptr<Device> device,
            std::shared_ptr<PipelineCache> pipelineCache,
            std::shared_ptr<IAllocator> allocator = nullptr);
        uint32_t getPipelineCount(VkPipelineBindPoint bindPoint) const noexcept;
        //std::shared_ptr<Pipeline> getPipeline(uint32_t index) const noexcept { return pipelines[index]; }

    private:
        template<typename Type>
        void fixup(std::vector<Type>& pipelineInfos) const;
        void gatherShaderStageInfos() const;
        void postCreateCleanup();
        void postBuildCleanup();

        std::list<std::vector<PipelineShaderStage>> stages;
        std::list<std::shared_ptr<PipelineLayout>> layouts;
        std::list<std::shared_ptr<Pipeline>> basePipelines;
    #ifdef VK_EXT_pipeline_creation_feedback
        std::list<VkPipelineCreationFeedbackEXT> creationFeedbacks;
        std::list<VkPipelineCreationFeedbackCreateInfoEXT> creationFeedbackInfos;
    #endif
        std::list<hash_t> hashes;

        std::list<std::vector<RayTracingShaderGroup>> groups;


        std::list<VertexInputState> vertexInputStates;
        std::list<InputAssemblyState> inputAssemblyStates;
        std::list<TesselationState> tesselationStates;
        std::list<ViewportState> viewportStates;
        std::list<RasterizationState> rasterizationStates;
        std::list<MultisampleState> multisampleStates;
        std::list<DepthStencilState> depthStencilStates;
        std::list<ColorBlendState> colorBlendStates;
        std::list<std::vector<VkDynamicState>> dynamicStates;
        std::list<VkPipelineDynamicStateCreateInfo> dynamicStateInfos;
        std::list<std::shared_ptr<RenderPass>> renderPasses;

        std::vector<VkGraphicsPipelineCreateInfo> graphicsPipelineInfos;
        std::vector<VkComputePipelineCreateInfo> computePipelineInfos;
    #ifdef VK_NV_ray_tracing
        std::vector<VkRayTracingPipelineCreateInfoNV> rayTracingPipelineInfos;
    #endif
        std::vector<std::shared_ptr<GraphicsPipeline>> graphicsPipelines;
        std::vector<std::shared_ptr<ComputePipeline>> computePipelines;
    #ifdef VK_NV_ray_tracing
        std::vector<std::shared_ptr<RayTracingPipeline>> rayTracingPipelines;
    #endif

        mutable std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
    };
} // namespace magma
