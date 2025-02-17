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
#include "pch.h"
#pragma hdrstop
#include "graphicsPipelineCache.h"
#include "../objects/graphicsPipeline.h"
#include "../objects/pipelineLayout.h"
#include "../objects/pipelineCache.h"
#include "../objects/renderPass.h"
#include "../states/vertexInputState.h"
#include "../states/inputAssemblyState.h"
#include "../states/tesselationState.h"
#include "../states/viewportState.h"
#include "../states/rasterizationState.h"
#include "../states/multisampleState.h"
#include "../states/depthStencilState.h"
#include "../states/colorBlendState.h"

namespace magma
{
namespace aux
{
GraphicsPipelineCache::GraphicsPipelineCache(std::shared_ptr<Device> device,
    std::shared_ptr<PipelineCache> pipelineCache,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    device(std::move(device)),
    pipelineCache(std::move(pipelineCache)),
    allocator(std::move(allocator))
{
    if (!this->pipelineCache)
        this->pipelineCache = std::make_shared<PipelineCache>(this->device, this->allocator);
}

std::shared_ptr<GraphicsPipeline> GraphicsPipelineCache::lookupPipeline(
    const std::vector<PipelineShaderStage>& shaderStages,
    const VertexInputState& vertexInputState,
    const InputAssemblyState& inputAssemblyState,
    const TesselationState& tesselationState,
    const ViewportState& viewportState,
    const RasterizationState& rasterizationState,
    const MultisampleState& multisampleState,
    const DepthStencilState& depthStencilState,
    const ColorBlendState& colorBlendState,
    const std::vector<VkDynamicState>& dynamicStates,
    std::shared_ptr<PipelineLayout> pipelineLayout,
    std::shared_ptr<RenderPass> renderPass,
    uint32_t subpass /* 0 */,
    VkPipelineCreateFlags flags /* 0 */)
{   // Compute hash of graphics pipeline object
    VkGraphicsPipelineCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = nullptr;
    // Specify that the pipeline to be created is allowed to be the parent of a pipeline that will be created
    info.flags = flags | VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    info.stageCount = MAGMA_COUNT(shaderStages);
    hash_t hash = core::hashArgs(
        info.sType,
        info.flags,
        info.stageCount);
    for (const auto& stage : shaderStages)
        hash = core::hashCombine(hash, stage.getHash());
    hash_t baseHash = core::combineHashList({
        vertexInputState.hash(),
        inputAssemblyState.hash(),
        tesselationState.hash(),
        viewportState.hash(),
        rasterizationState.hash(),
        multisampleState.hash(),
        depthStencilState.hash(),
        colorBlendState.hash()});
    for (auto state : dynamicStates)
        hash = core::hashCombine(baseHash, core::hash(state));
    hash = core::hashCombine(hash, baseHash);
    if (!pipelineLayout)
        pipelineLayout = std::make_shared<PipelineLayout>(device);
    hash = core::hashCombine(hash, pipelineLayout->getHash());
    if (renderPass)
    {
        hash = core::hashCombine(hash, renderPass->getHash());
        hash = core::hashCombine(hash, core::hash(subpass));
    }
    // Lookup for existing pipeline
    auto it = pipelines.find(hash);
    if (it != pipelines.end())
        return it->second;
    // Try to lookup base pipeline to speed up pipeline construction
    std::shared_ptr<GraphicsPipeline> basePipeline = lookupBasePipeline(
        vertexInputState, inputAssemblyState, tesselationState, viewportState,
        rasterizationState, multisampleState, depthStencilState, colorBlendState,
        dynamicStates);
    if (basePipeline)
    {   // A pipeline derivative is a child pipeline created from a parent pipeline,
        // where the child and parent are expected to have much commonality.
        info.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    }
    // Create new pipeline using cache and base pipeline to speed up construction time
    std::shared_ptr<GraphicsPipeline> pipeline = std::make_shared<GraphicsPipeline>(device,
        shaderStages, vertexInputState, inputAssemblyState, tesselationState, viewportState,
        rasterizationState, multisampleState, depthStencilState, colorBlendState, dynamicStates,
        std::move(pipelineLayout), std::move(renderPass), subpass,
        allocator, pipelineCache, std::move(basePipeline), info.flags);
    MAGMA_ASSERT(pipeline->getHash() == hash); // Check hash computation
    pipelines.emplace(hash, pipeline);
    basePipelines.emplace(baseHash, pipeline);
    return pipeline;
}

std::shared_ptr<GraphicsPipeline> GraphicsPipelineCache::lookupBasePipeline(
    const VertexInputState& vertexInputState,
    const InputAssemblyState& inputAssemblyState,
    const TesselationState& tesselationState,
    const ViewportState& viewportState,
    const RasterizationState& rasterizationState,
    const MultisampleState& multisampleState,
    const DepthStencilState& depthStencilState,
    const ColorBlendState& colorBlendState,
    const std::vector<VkDynamicState>& dynamicStates /* {} */) const noexcept
{
    if (!basePipelines.empty())
    {   // Compute hash of render and dynamic states
        hash_t hash = core::combineHashList({
            vertexInputState.hash(),
            inputAssemblyState.hash(),
            tesselationState.hash(),
            viewportState.hash(),
            rasterizationState.hash(),
            multisampleState.hash(),
            depthStencilState.hash(),
            colorBlendState.hash()});
        for (auto state : dynamicStates)
            hash = core::hashCombine(hash, core::hash(state));
        auto it = basePipelines.find(hash);
        if (it != basePipelines.end())
            return it->second;
    }
    return nullptr;
}
} // namespace aux
} // namespace magma
