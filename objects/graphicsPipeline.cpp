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
#include "graphicsPipeline.h"
#include "pipelineLayout.h"
#include "pipelineCache.h"
#include "device.h"
#include "renderPass.h"
#include "../shaders/pipelineShaderStage.h"
#include "../states/vertexInputState.h"
#include "../states/inputAssemblyState.h"
#include "../states/tesselationState.h"
#include "../states/viewportState.h"
#include "../states/rasterizationState.h"
#include "../states/multisampleState.h"
#include "../states/depthStencilState.h"
#include "../states/colorBlendState.h"
#include "../allocator/allocator.h"
#include "../exceptions/errorResult.h"
#include "../helpers/stackArray.h"

namespace magma
{
GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device,
    const std::vector<PipelineShaderStage>& shaderStages,
    const VertexInputState& vertexInputState,
    const InputAssemblyState& inputAssemblyState,
    const RasterizationState& rasterizationState,
    const MultisampleState& multisampleState,
    const DepthStencilState& depthStencilState,
    const ColorBlendState& colorBlendState,
    const std::vector<VkDynamicState>& dynamicStates,
    std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<RenderPass> renderPass,
    uint32_t subpass,
    std::shared_ptr<IAllocator> allocator /* nullptr */,
    std::shared_ptr<PipelineCache> pipelineCache /* nullptr */,
    std::shared_ptr<GraphicsPipeline> basePipeline /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */,
    const StructureChain& extendedInfo /* default */):
    GraphicsPipeline(std::move(device), shaderStages, vertexInputState, inputAssemblyState,
        TesselationState(), // No tesselation state
        ViewportState(), // No viewport state (supposed to be dynamic)
        rasterizationState, multisampleState, depthStencilState, colorBlendState, dynamicStates,
        std::move(layout), std::move(renderPass), subpass,
        std::move(allocator), std::move(pipelineCache), std::move(basePipeline),
        flags, extendedInfo)
{}

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device_,
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
    std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<RenderPass> renderPass,
    uint32_t subpass,
    std::shared_ptr<IAllocator> allocator /* nullptr */,
    std::shared_ptr<PipelineCache> pipelineCache /* nullptr */,
    std::shared_ptr<GraphicsPipeline> basePipeline_ /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */,
    const StructureChain& extendedInfo /* default */):
    Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, std::move(device_), std::move(layout), std::move(basePipeline_), std::move(allocator), MAGMA_COUNT(shaderStages))
{
    MAGMA_STACK_ARRAY(VkPipelineShaderStageCreateInfo, dereferencedStages, shaderStages.size());
    for (auto& stage : shaderStages)
        dereferencedStages.put(stage);
    VkGraphicsPipelineCreateInfo pipelineInfo;
    VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = extendedInfo.chainNodes();
    pipelineInfo.flags = flags;
    if (basePipeline)
        pipelineInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    pipelineInfo.stageCount = MAGMA_COUNT(dereferencedStages);
    pipelineInfo.pStages = dereferencedStages;
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pTessellationState = tesselationState.patchControlPoints ? &tesselationState : nullptr;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = dynamicStates.empty() ? nullptr : &pipelineDynamicStateInfo;
    pipelineInfo.layout = MAGMA_HANDLE(layout);
    pipelineInfo.renderPass = *renderPass;
    pipelineInfo.subpass = subpass;
    pipelineInfo.basePipelineHandle = MAGMA_OPTIONAL_HANDLE(basePipeline);
    pipelineInfo.basePipelineIndex = -1;
    pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateInfo.pNext = nullptr;
    pipelineDynamicStateInfo.flags = 0;
    pipelineDynamicStateInfo.dynamicStateCount = MAGMA_COUNT(dynamicStates);
    pipelineDynamicStateInfo.pDynamicStates = dynamicStates.data();
#ifdef VK_EXT_pipeline_creation_feedback
    VkPipelineCreationFeedbackCreateInfoEXT pipelineCreationFeedbackInfo;
    if (device->extensionEnabled(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME))
    {
        stageCreationFeedbacks.resize(pipelineInfo.stageCount);
        pipelineCreationFeedbackInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
        pipelineCreationFeedbackInfo.pNext = pipelineInfo.pNext;
        pipelineCreationFeedbackInfo.pPipelineCreationFeedback = &creationFeedback;
        pipelineCreationFeedbackInfo.pipelineStageCreationFeedbackCount = pipelineInfo.stageCount;
        pipelineCreationFeedbackInfo.pPipelineStageCreationFeedbacks = stageCreationFeedbacks.data();
        pipelineInfo.pNext = &pipelineCreationFeedbackInfo;
    }
#endif // VK_EXT_pipeline_creation_feedback
    const VkResult result = vkCreateGraphicsPipelines(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(pipelineCache),
        1, &pipelineInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_HANDLE_RESULT(result, "failed to create graphics pipeline");
    hash = core::hashArgs(
        pipelineInfo.sType,
        pipelineInfo.flags,
        pipelineInfo.stageCount);
    for (const auto& stage : shaderStages)
        hash = core::hashCombine(hash, stage.getHash());
    hash_t stateHash = core::combineHashList({
        vertexInputState.hash(),
        inputAssemblyState.hash(),
        tesselationState.hash(),
        viewportState.hash(),
        rasterizationState.chained()
            ? rasterizationState.chainedHash()
            : rasterizationState.hash(),
        multisampleState.hash(),
        depthStencilState.hash(),
        colorBlendState.hash()});
    for (auto state : dynamicStates)
        hash = core::hashCombine(stateHash, core::hash(state));
    hash = core::hashCombine(hash, stateHash);
    hash = core::hashCombine(hash, this->layout->getHash());
    if (renderPass)
    {
        hash = core::hashCombine(hash, renderPass->getHash());
        hash = core::hashCombine(hash, core::hash(subpass));
    }
}

GraphicsPipeline::GraphicsPipeline(VkPipeline handle_,
    std::shared_ptr<Device> device,
    std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<Pipeline> basePipeline,
    std::shared_ptr<IAllocator> allocator,
    uint32_t stageCount,
#ifdef VK_EXT_pipeline_creation_feedback
    VkPipelineCreationFeedbackEXT creationFeedback,
    const std::vector<VkPipelineCreationFeedbackEXT>& stageCreationFeedbacks,
#endif // VK_EXT_pipeline_creation_feedback
    hash_t hash):
    Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, std::move(device), std::move(layout), std::move(basePipeline), std::move(allocator),
        stageCount,
    #ifdef VK_EXT_pipeline_creation_feedback
        creationFeedback, stageCreationFeedbacks,
    #endif
        hash)
{
    handle = handle_;
}
} // namespace magma
