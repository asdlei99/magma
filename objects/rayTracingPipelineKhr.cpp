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
#include "rayTracingPipelineKhr.h"
#include "pipelineLayout.h"
#include "pipelineCache.h"
#include "device.h"
#include "physicalDevice.h"
#include "deferredOperation.h"
#include "../shaders/pipelineShaderStage.h"
#include "../allocator/allocator.h"
#include "../misc/extProcAddress.h"
#include "../exceptions/errorResult.h"
#include "../helpers/stackArray.h"

namespace magma
{
#ifdef VK_KHR_ray_tracing_pipeline
RayTracingPipeline::RayTracingPipeline(std::shared_ptr<Device> device,
    const std::vector<PipelineShaderStage>& shaderStages,
    const std::vector<RayTracingShaderGroup>& shaderGroups,
    uint32_t maxRayRecursionDepth,
    std::shared_ptr<PipelineLayout> layout_,
    const std::vector<VkDynamicState>& dynamicStates,
    std::shared_ptr<IAllocator> allocator /* nullptr */,
    std::shared_ptr<PipelineCache> pipelineCache /* nullptr */,
    std::shared_ptr<RayTracingPipeline> basePipeline_ /* nullptr */,
    std::shared_ptr<DeferredOperation> deferredOp /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */):
    Pipeline(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, std::move(device), std::move(layout_), std::move(basePipeline_), std::move(allocator)),
    flags(flags),
    shaderGroupCount(MAGMA_COUNT(shaderGroups))
{
    MAGMA_STACK_ARRAY(VkPipelineShaderStageCreateInfo, dereferencedStages, shaderStages.size());
    for (const auto& stage: shaderStages)
        dereferencedStages.put(stage);
    VkRayTracingPipelineCreateInfoKHR pipelineInfo;
    VkPipelineDynamicStateCreateInfo pipelineDynamicStateInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = flags;
    pipelineInfo.stageCount = MAGMA_COUNT(dereferencedStages);
    pipelineInfo.pStages = dereferencedStages;
    pipelineInfo.groupCount = shaderGroupCount;
    pipelineInfo.pGroups = shaderGroups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = maxRayRecursionDepth;
    pipelineInfo.pLibraryInfo = nullptr;
    pipelineInfo.pLibraryInterface = nullptr;
    pipelineInfo.pDynamicState = dynamicStates.empty() ? nullptr : &pipelineDynamicStateInfo;
    pipelineInfo.layout = MAGMA_HANDLE(layout);
    pipelineInfo.basePipelineHandle = MAGMA_OPTIONAL_HANDLE(basePipeline);
    pipelineInfo.basePipelineIndex = -1;
    if (!dynamicStates.empty())
    {
        pipelineDynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        pipelineDynamicStateInfo.pNext = nullptr;
        pipelineDynamicStateInfo.flags = 0;
        pipelineDynamicStateInfo.dynamicStateCount = MAGMA_COUNT(dynamicStates);
        pipelineDynamicStateInfo.pDynamicStates = dynamicStates.data();
    }
#ifdef VK_EXT_pipeline_creation_feedback
    VkPipelineCreationFeedbackCreateInfoEXT pipelineCreationFeedbackInfo;
    MAGMA_STACK_ARRAY(VkPipelineCreationFeedbackEXT, stageCreationFeedbacks, shaderStages.size());
    if (getDevice()->extensionEnabled(VK_EXT_PIPELINE_CREATION_FEEDBACK_EXTENSION_NAME))
    {
        pipelineInfo.pNext = &pipelineCreationFeedbackInfo;
        pipelineCreationFeedbackInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO_EXT;
        pipelineCreationFeedbackInfo.pNext = nullptr;
        pipelineCreationFeedbackInfo.pPipelineCreationFeedback = &creationFeedback;
        pipelineCreationFeedbackInfo.pipelineStageCreationFeedbackCount = pipelineInfo.stageCount;
        pipelineCreationFeedbackInfo.pPipelineStageCreationFeedbacks = stageCreationFeedbacks;
    }
#endif // VK_EXT_pipeline_creation_feedback
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkCreateRayTracingPipelinesKHR, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    const VkResult result = vkCreateRayTracingPipelinesKHR(MAGMA_HANDLE(device),
        MAGMA_OPTIONAL_HANDLE(deferredOp),
        MAGMA_OPTIONAL_HANDLE(pipelineCache),
        1, &pipelineInfo, 
        MAGMA_OPTIONAL_INSTANCE(hostAllocator),
        &handle);
    MAGMA_THROW_FAILURE(result, "failed to create ray tracing pipeline");
    hash = core::hashArgs(
        pipelineInfo.sType,
        pipelineInfo.flags,
        pipelineInfo.stageCount,
        pipelineInfo.groupCount,
        pipelineInfo.maxPipelineRayRecursionDepth);
    for (const auto& stage: shaderStages)
        hash = core::hashCombine(hash, stage.getHash());
    for (const auto& group: shaderGroups)
        hash = core::hashCombine(hash, group.hash());
    hash = core::hashCombine(hash, layout->getHash());
}

std::vector<uint8_t> RayTracingPipeline::getShaderGroupHandles(uint32_t firstGroup, uint32_t groupCount) const
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = device->getPhysicalDevice()->getRayTracingPipelineProperties();
    const size_t dataSize = groupCount * rayTracingPipelineProperties.shaderGroupHandleSize;
    std::vector<uint8_t> shaderGroupHandles(dataSize);
    MAGMA_DEVICE_EXTENSION(vkGetRayTracingShaderGroupHandlesKHR);
    const VkResult result = vkGetRayTracingShaderGroupHandlesKHR(MAGMA_HANDLE(device), handle, firstGroup, groupCount, dataSize, shaderGroupHandles.data());
    MAGMA_THROW_FAILURE(result, "failed to get ray tracing shader group handles");
    return shaderGroupHandles;
}

std::vector<uint8_t> RayTracingPipeline::getCaptureReplayShaderGroupHandles(uint32_t firstGroup, uint32_t groupCount) const
{
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties = device->getPhysicalDevice()->getRayTracingPipelineProperties();
    const size_t dataSize = groupCount * rayTracingPipelineProperties.shaderGroupHandleCaptureReplaySize;
    std::vector<uint8_t> captureReplayShaderGroupHandles(dataSize);
    MAGMA_DEVICE_EXTENSION(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR);
    const VkResult result = vkGetRayTracingCaptureReplayShaderGroupHandlesKHR(MAGMA_HANDLE(device), handle, firstGroup, groupCount, dataSize, captureReplayShaderGroupHandles.data());
    MAGMA_THROW_FAILURE(result, "failed to get ray tracing shader group handles of capture replay");
    return captureReplayShaderGroupHandles;
}

VkDeviceSize RayTracingPipeline::getShaderGroupStackSize(uint32_t group, VkShaderGroupShaderKHR groupShader) const noexcept
{
    MAGMA_DEVICE_EXTENSION(vkGetRayTracingShaderGroupStackSizeKHR);
    return vkGetRayTracingShaderGroupStackSizeKHR(MAGMA_HANDLE(device), handle, group, groupShader);
}
#endif // VK_KHR_ray_tracing_pipeline
} // namespace magma
