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
#include "pipelineCompiler.h"
#include "pipelineLayout.h"
#include "pipelineCache.h"
#include "graphicsPipeline.h"
#include "computePipeline.h"
#include "rayTracingPipeline.h"
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

#undef MAGMA_HANDLE
#define MAGMA_HANDLE(p) *(p)

#include "../misc/extProcAddress.h"

namespace magma
{
PipelineCompiler::PipelineCompiler(uint32_t preAllocCount /* 0 */)
{
    if (preAllocCount)
    {
        graphicsPipelines.reserve(preAllocCount);
        computePipelines.reserve(preAllocCount >> 4);
    #ifdef VK_NV_ray_tracing
        rtPipelines.reserve(preAllocCount >> 4);
    #endif
    }
}

uint32_t PipelineCompiler::newGraphicsPipeline(const std::vector<PipelineShaderStage>& shaderStages,
    const VertexInputState& vertexInputState,
    const InputAssemblyState& inputAssemblyState,
    const TesselationState& tesselationState,
    const ViewportState& viewportState,
    const RasterizationState& rasterizationState,
    const MultisampleState& multisampleState,
    const DepthStencilState& depthStencilState,
    const ColorBlendState& colorBlendState,
    const std::vector<VkDynamicState>& dynamicRenderStates,
    std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<RenderPass> renderPass,
    uint32_t subpass,
    std::shared_ptr<GraphicsPipeline> basePipeline /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */)
{
    graphics.stages.push_back(shaderStages);
    vertexInputStates.push_back(vertexInputState);
    inputAssemblyStates.push_back(inputAssemblyState);
    tesselationStates.push_back(tesselationState);
    viewportStates.push_back(viewportState);
    rasterizationStates.push_back(rasterizationState);
    multisampleStates.push_back(multisampleState);
    depthStencilStates.push_back(depthStencilState);
    colorBlendStates.push_back(colorBlendState);
    dynamicStates.push_back(dynamicRenderStates);
    graphics.layouts.push_back(layout);
    renderPasses.push_back(renderPass);
    graphics.basePipelines.push_back(basePipeline);
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.pNext = 0;
    dynamicStateInfo.flags = 0;
    dynamicStateInfo.dynamicStateCount = MAGMA_COUNT(dynamicStates.back());
    dynamicStateInfo.pDynamicStates = dynamicStates.back().data();
    dynamicStateInfos.push_back(dynamicStateInfo);
    VkGraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = flags;
    if (basePipeline)
        pipelineInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    pipelineInfo.stageCount = MAGMA_COUNT(shaderStages);
    pipelineInfo.pStages = nullptr; // Fixup later
    pipelineInfo.pVertexInputState = &vertexInputStates.back();
    pipelineInfo.pInputAssemblyState = &inputAssemblyStates.back();
    pipelineInfo.pTessellationState = &tesselationStates.back();
    pipelineInfo.pViewportState = &viewportStates.back();
    pipelineInfo.pRasterizationState = &rasterizationStates.back();
    pipelineInfo.pMultisampleState = &multisampleStates.back();
    pipelineInfo.pDepthStencilState = &depthStencilStates.back();
    pipelineInfo.pColorBlendState = &colorBlendStates.back();
    pipelineInfo.pDynamicState = &dynamicStateInfos.back();
    pipelineInfo.layout = MAGMA_OPTIONAL_HANDLE(graphics.layouts.back());
    pipelineInfo.renderPass = MAGMA_HANDLE(renderPasses.back());
    pipelineInfo.subpass = subpass;
    pipelineInfo.basePipelineHandle = MAGMA_OPTIONAL_HANDLE(graphics.basePipelines.back());
    pipelineInfo.basePipelineIndex = -1;
#ifdef VK_EXT_pipeline_creation_feedback
    if (layout->getDevice()->extensionEnabled(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME))
    {
        graphics.creationFeedbacks.push_back(VkPipelineCreationFeedbackEXT());
        VkPipelineCreationFeedbackCreateInfoEXT creationFeedbackInfo;
        creationFeedbackInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
        creationFeedbackInfo.pNext = nullptr;
        creationFeedbackInfo.pPipelineCreationFeedback = &graphics.creationFeedbacks.back();
        creationFeedbackInfo.pipelineStageCreationFeedbackCount = 0;
        creationFeedbackInfo.pPipelineStageCreationFeedbacks = nullptr;
        creationFeedbackInfos.push_back(creationFeedbackInfo);
        pipelineInfo.pNext = &creationFeedbackInfos.back();
    }
#endif // VK_EXT_pipeline_creation_feedback
    graphicsPipelineInfos.push_back(pipelineInfo);
    hash_t hash = core::hashArgs(
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
        rasterizationState.hash(),
        multisampleState.hash(),
        depthStencilState.hash(),
        colorBlendState.hash()});
    for (auto state : dynamicRenderStates)
        hash = core::hashCombine(stateHash, core::hash(state));
    hash = core::hashCombine(hash, stateHash);
    hash = core::hashCombine(hash, layout->getHash());
    if (renderPass)
    {
        hash = core::hashCombine(hash, renderPass->getHash());
        hash = core::hashCombine(hash, core::hash(subpass));
    }
    graphics.hashes.push_back(hash);
    return MAGMA_COUNT(graphicsPipelineInfos) - 1;
}

uint32_t PipelineCompiler::newComputePipeline(const PipelineShaderStage& shaderStage, std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<ComputePipeline> basePipeline /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */)
{
    compute.stages.push_back(std::vector<PipelineShaderStage>{shaderStage});
    compute.layouts.push_back(layout);
    compute.basePipelines.push_back(basePipeline);
    VkComputePipelineCreateInfo pipelineInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = flags;
    if (basePipeline)
        pipelineInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    pipelineInfo.stage = compute.stages.back().front();
    pipelineInfo.layout = MAGMA_HANDLE(compute.layouts.back());
    pipelineInfo.basePipelineHandle = MAGMA_OPTIONAL_HANDLE(compute.basePipelines.back());
    pipelineInfo.basePipelineIndex = -1;
#ifdef VK_EXT_pipeline_creation_feedback
    if (layout->getDevice()->extensionEnabled(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME))
    {
        compute.creationFeedbacks.push_back(VkPipelineCreationFeedbackEXT());
        VkPipelineCreationFeedbackCreateInfoEXT creationFeedbackInfo;
        creationFeedbackInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
        creationFeedbackInfo.pNext = nullptr;
        creationFeedbackInfo.pPipelineCreationFeedback = &compute.creationFeedbacks.back();
        creationFeedbackInfo.pipelineStageCreationFeedbackCount = 0;
        creationFeedbackInfo.pPipelineStageCreationFeedbacks = nullptr;
        creationFeedbackInfos.push_back(creationFeedbackInfo);
        pipelineInfo.pNext = &creationFeedbackInfos.back();
    }
#endif // VK_EXT_pipeline_creation_feedback
    computePipelineInfos.push_back(pipelineInfo);
    hash_t hash = core::hashArgs(
        pipelineInfo.sType,
        pipelineInfo.flags);
    hash = core::hashCombine(hash, shaderStage.getHash());
    hash = core::hashCombine(hash, layout->getHash());
    compute.hashes.push_back(hash);
    return MAGMA_COUNT(computePipelineInfos) - 1;
}

#ifdef VK_NV_ray_tracing
uint32_t PipelineCompiler::newRayTracingPipeline(const std::vector<PipelineShaderStage>& shaderStages,
    const std::vector<RayTracingShaderGroup>& shaderGroups, uint32_t maxRecursionDepth, std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<RayTracingPipeline> basePipeline /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */)
{
    rt.stages.push_back(shaderStages);
    rt.groups.push_back(shaderGroups);
    rt.layouts.push_back(layout);
    rt.basePipelines.push_back(basePipeline);
    VkRayTracingPipelineCreateInfoNV pipelineInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = flags;
    pipelineInfo.stageCount = MAGMA_COUNT(shaderStages);
    pipelineInfo.pStages = nullptr; // Fixup later
    pipelineInfo.groupCount = MAGMA_COUNT(shaderGroups);
    pipelineInfo.pGroups = rt.groups.back().data();
    pipelineInfo.maxRecursionDepth = maxRecursionDepth;
    pipelineInfo.layout = MAGMA_HANDLE(rt.layouts.back());
    pipelineInfo.basePipelineHandle = MAGMA_OPTIONAL_HANDLE(rt.basePipelines.back());
    pipelineInfo.basePipelineIndex = -1;
#ifdef VK_EXT_pipeline_creation_feedback
    if (layout->getDevice()->extensionEnabled(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME))
    {
        rt.creationFeedbacks.push_back(VkPipelineCreationFeedbackEXT());
        VkPipelineCreationFeedbackCreateInfoEXT creationFeedbackInfo;
        creationFeedbackInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
        creationFeedbackInfo.pNext = nullptr;
        creationFeedbackInfo.pPipelineCreationFeedback = &rt.creationFeedbacks.back();
        creationFeedbackInfo.pipelineStageCreationFeedbackCount = 0;
        creationFeedbackInfo.pPipelineStageCreationFeedbacks = nullptr;
        creationFeedbackInfos.push_back(creationFeedbackInfo);
        pipelineInfo.pNext = &creationFeedbackInfos.back();
    }
#endif // VK_EXT_pipeline_creation_feedback
    rtPipelineInfos.push_back(pipelineInfo);
    hash_t hash = core::hashArgs(
        pipelineInfo.sType,
        pipelineInfo.flags,
        pipelineInfo.stageCount,
        pipelineInfo.groupCount,
        pipelineInfo.maxRecursionDepth);
    for (const auto& stage : shaderStages)
        hash = core::hashCombine(hash, stage.getHash());
    for (const auto& group : shaderGroups)
        hash = core::hashCombine(hash, group.hash());
    hash = core::hashCombine(hash, layout->getHash());
    rt.hashes.push_back(hash);
    return MAGMA_COUNT(rtPipelineInfos) - 1;
}
#endif // VK_NV_ray_tracing

void PipelineCompiler::buildPipelines(std::shared_ptr<Device> device, std::shared_ptr<PipelineCache> pipelineCache,
    std::shared_ptr<IAllocator> allocator /* nullptr */)
{
    graphicsPipelines.clear();
    computePipelines.clear();
#ifdef VK_NV_ray_tracing
    rtPipelines.clear();
#endif
    fixupStagePointers();
    VkResult graphicsResult = VK_NOT_READY;
    if (!graphicsPipelineInfos.empty())
    {
        graphics.pipelineHandles.resize(graphicsPipelineInfos.size(), VK_NULL_HANDLE);
        graphicsResult = vkCreateGraphicsPipelines(*device, MAGMA_OPTIONAL_HANDLE(pipelineCache),
            MAGMA_COUNT(graphicsPipelineInfos), graphicsPipelineInfos.data(), allocator.get(),
            graphics.pipelineHandles.data());
        // Free temporarily allocated storage that had to be preserved until vkCreateGraphicsPipelines() call
        vertexInputStates.clear();
        inputAssemblyStates.clear();
        tesselationStates.clear();
        viewportStates.clear();
        rasterizationStates.clear();
        multisampleStates.clear();
        depthStencilStates.clear();
        colorBlendStates.clear();
        dynamicStates.clear();
        dynamicStateInfos.clear();
        renderPasses.clear();
        graphicsPipelineInfos.clear();
        computePipelineInfos.clear();
    }
    VkResult computeResult = VK_NOT_READY;
    if (!computePipelineInfos.empty())
    {
        compute.pipelineHandles.resize(computePipelineInfos.size(), VK_NULL_HANDLE);
        computeResult = vkCreateComputePipelines(*device, MAGMA_OPTIONAL_HANDLE(pipelineCache),
            MAGMA_COUNT(computePipelineInfos), computePipelineInfos.data(), allocator.get(),
            compute.pipelineHandles.data());
    }
#ifdef VK_NV_ray_tracing
    VkResult rtResult = VK_NOT_READY;
    if (!rtPipelineInfos.empty())
    {
        rt.pipelineHandles.resize(rtPipelineInfos.size(), VK_NULL_HANDLE);
        MAGMA_REQUIRED_DEVICE_EXTENSION(vkCreateRayTracingPipelinesNV, VK_NV_RAY_TRACING_EXTENSION_NAME);
        rtResult = vkCreateRayTracingPipelinesNV(*device, MAGMA_OPTIONAL_HANDLE(pipelineCache),
            MAGMA_COUNT(rtPipelineInfos), rtPipelineInfos.data(), allocator.get(),
            rt.pipelineHandles.data());
    }
#endif // VK_NV_ray_tracing
#ifdef VK_EXT_pipeline_creation_feedback
    creationFeedbackInfos.clear();
#endif
    if (VK_SUCCESS == graphicsResult)
    {
        auto handle = graphics.pipelineHandles.cbegin();
        auto layout = graphics.layouts.cbegin();
        auto basePipeline = graphics.basePipelines.cbegin();
    #ifdef VK_EXT_pipeline_creation_feedback
        auto creationFeedback = graphics.creationFeedbacks.cbegin();
    #endif
        auto hash = graphics.hashes.cbegin();
        graphicsPipelines.reserve(graphics.pipelineHandles.size());
        while (handle != graphics.pipelineHandles.cend())
        {
            graphicsPipelines.emplace_back(new GraphicsPipeline(
                *handle++,
                device,
                *layout++,
                *basePipeline++,
                allocator,
            #ifdef VK_EXT_pipeline_creation_feedback
                *creationFeedback++,
            #endif
                *hash++));
        }
    }
    if (VK_SUCCESS == computeResult)
    {
        auto handle = compute.pipelineHandles.cbegin();
        auto layout = compute.layouts.cbegin();
        auto basePipeline = compute.basePipelines.cbegin();
    #ifdef VK_EXT_pipeline_creation_feedback
        auto creationFeedback = compute.creationFeedbacks.cbegin();
    #endif
        auto hash = compute.hashes.cbegin();
        computePipelines.reserve(compute.pipelineHandles.size());
        while (handle != compute.pipelineHandles.cend())
        {
            computePipelines.emplace_back(new ComputePipeline(
                *handle++,
                device,
                *layout++,
                *basePipeline++,
                allocator,
            #ifdef VK_EXT_pipeline_creation_feedback
                *creationFeedback++,
            #endif
                *hash++));
        }
    }
#ifdef VK_NV_ray_tracing
    if (VK_SUCCESS == rtResult)
    {
        auto handle = rt.pipelineHandles.cbegin();
        auto layout = rt.layouts.cbegin();
        auto basePipeline = rt.basePipelines.cbegin();
    #ifdef VK_EXT_pipeline_creation_feedback
        auto creationFeedback = rt.creationFeedbacks.cbegin();
    #endif
        auto hash = rt.hashes.cbegin();
        auto info = rtPipelineInfos.cbegin();
        rtPipelines.reserve(rt.pipelineHandles.size());
        while (handle != rt.pipelineHandles.cend())
        {
            rtPipelines.emplace_back(new RayTracingPipeline(
                *handle++,
                device,
                *layout++,
                *basePipeline++,
                allocator,
                info->groupCount,
                info->maxRecursionDepth,
            #ifdef VK_EXT_pipeline_creation_feedback
                *creationFeedback++,
            #endif
                *hash++));
            ++info;
        }
    }
    rtPipelineInfos.clear();
    // Free temporarily allocated storage that had to be preserved until ctor calls
    rt.clear();
#endif // VK_NV_ray_tracing
    graphics.clear();
    compute.clear();
    if (!graphicsPipelineInfos.empty())
        MAGMA_THROW_FAILURE(graphicsResult, "failed to compile graphics pipelines");
    if (!computePipelineInfos.empty())
        MAGMA_THROW_FAILURE(computeResult, "failed to compile compute pipelines");
#ifdef VK_NV_ray_tracing
    if (!rtPipelineInfos.empty())
        MAGMA_THROW_FAILURE(rtResult, "failed to compile ray tracing pipelines");
#endif
}

void PipelineCompiler::fixupStagePointers()
{
    uint32_t i = 0;
    graphics.compactShaderStages();
    for (auto& pipelineInfo : graphicsPipelineInfos)
    {
        pipelineInfo.pStages = &graphics.shaderStageInfos[i];
        i += pipelineInfo.stageCount;
    }
#ifdef VK_NV_ray_tracing
    i = 0;
    rt.compactShaderStages();
    for (auto& pipelineInfo : rtPipelineInfos)
    {
        pipelineInfo.pStages = &rt.shaderStageInfos[i];
        i += pipelineInfo.stageCount;
    }
#endif // VK_NV_ray_tracing
}

void PipelineCompiler::PipelineData::compactShaderStages()
{
    std::size_t stageCount = 0;
    for (const auto& shaderStages : stages)
        stageCount += shaderStages.size();
    shaderStageInfos.clear();
    shaderStageInfos.reserve(stageCount);
    for (const auto& shaderStages : stages)
    {   // Copy to array of Vulkan structures due to alignment of magma::PipelineShaderStage class
        for (const auto& stage : shaderStages)
            shaderStageInfos.push_back(stage);
    }
}

void PipelineCompiler::PipelineData::clear()
{
    stages.clear();
#ifdef VK_NV_ray_tracing
    groups.clear();
#endif
    layouts.clear();
    basePipelines.clear();
#ifdef VK_EXT_pipeline_creation_feedback
    creationFeedbacks.clear();
#endif
    hashes.clear();
    shaderStageInfos.clear();
    pipelineHandles.clear();
}
} // namespace magma
