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
#include "rayTracingPipeline.h"
#include "pipelineLayout.h"
#include "pipelineCache.h"
#include "device.h"
#include "physicalDevice.h"
#include "../shaders/pipelineShaderStage.h"
#include "../allocator/allocator.h"
#include "../misc/extProcAddress.h"
#include "../exceptions/errorResult.h"
#include "../helpers/stackArray.h"

namespace magma
{
#ifdef VK_NV_ray_tracing
RayTracingPipelineNV::RayTracingPipelineNV(std::shared_ptr<Device> device,
    const std::vector<PipelineShaderStage>& shaderStages,
    const std::vector<RayTracingShaderGroup>& shaderGroups,
    uint32_t maxRecursionDepth,
    std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<IAllocator> allocator /* nullptr */,
    std::shared_ptr<PipelineCache> pipelineCache /* nullptr */,
    std::shared_ptr<RayTracingPipelineNV> basePipeline /* nullptr */,
    VkPipelineCreateFlags flags /* 0 */):
    Pipeline(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, std::move(device), std::move(layout), std::move(basePipeline), std::move(allocator)),
    shaderGroupCount(MAGMA_COUNT(shaderGroups)),
    maxRecursionDepth(maxRecursionDepth)
{
    if (shaderStages.empty())
        MAGMA_THROW("shader stages are empty");
    if (shaderGroups.empty())
        MAGMA_THROW("shader groups are empty");
    VkRayTracingPipelineCreateInfoNV pipelineInfo;
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.flags = flags;
    if (this->basePipeline)
        pipelineInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    MAGMA_STACK_ARRAY(VkPipelineShaderStageCreateInfo, dereferencedStages, shaderStages.size());
    for (auto& stage : shaderStages)
        dereferencedStages.put(stage);
    pipelineInfo.stageCount = MAGMA_COUNT(dereferencedStages);
    pipelineInfo.pStages = dereferencedStages;
    pipelineInfo.groupCount = shaderGroupCount;
    pipelineInfo.pGroups = shaderGroups.data();
    pipelineInfo.maxRecursionDepth = maxRecursionDepth;
    pipelineInfo.layout = MAGMA_HANDLE(layout);
    pipelineInfo.basePipelineHandle = MAGMA_OPTIONAL_HANDLE(this->basePipeline);
    pipelineInfo.basePipelineIndex = -1;
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
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkCreateRayTracingPipelinesNV, VK_NV_RAY_TRACING_EXTENSION_NAME);
    const VkResult result = vkCreateRayTracingPipelinesNV(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(pipelineCache), 1, &pipelineInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create ray tracing pipeline");
    hash = core::hashArgs(
        pipelineInfo.sType,
        pipelineInfo.flags,
        pipelineInfo.stageCount,
        pipelineInfo.groupCount,
        pipelineInfo.maxRecursionDepth);
    for (const auto& stage : shaderStages)
        hash = core::hashCombine(hash, stage.getHash());
    for (const auto& group : shaderGroups)
        hash = core::hashCombine(hash, group.hash());
    hash = core::hashCombine(hash, this->layout->getHash());
}

RayTracingPipelineNV::RayTracingPipelineNV(VkPipeline pipeline,
    std::shared_ptr<Device> device,
    std::shared_ptr<PipelineLayout> layout,
    std::shared_ptr<Pipeline> basePipeline,
    std::shared_ptr<IAllocator> allocator,
    uint32_t shaderGroupCount,
    uint32_t maxRecursionDepth,
#ifdef VK_EXT_pipeline_creation_feedback
    VkPipelineCreationFeedbackEXT creationFeedback,
#endif
    hash_t hash):
    Pipeline(VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, std::move(device), std::move(layout), std::move(basePipeline), std::move(allocator),
    #ifdef VK_EXT_pipeline_creation_feedback
        creationFeedback,
    #endif
        hash),
    shaderGroupCount(shaderGroupCount),
    maxRecursionDepth(maxRecursionDepth)
{
    handle = pipeline;
}

std::vector<uint8_t> RayTracingPipelineNV::getShaderGroupHandles() const
{
    const VkPhysicalDeviceRayTracingPropertiesNV& rayTracingProperties = device->getPhysicalDevice()->getRayTracingProperties();
    std::vector<uint8_t> shaderGroupHandles(shaderGroupCount * rayTracingProperties.shaderGroupHandleSize);
    MAGMA_DEVICE_EXTENSION(vkGetRayTracingShaderGroupHandlesNV);
    const VkResult result = vkGetRayTracingShaderGroupHandlesNV(MAGMA_HANDLE(device), handle, 0, shaderGroupCount, shaderGroupHandles.size(), shaderGroupHandles.data());
    MAGMA_THROW_FAILURE(result, "failed to get ray tracing shader handles");
    return shaderGroupHandles;
}

void RayTracingPipelineNV::compileDeferred(uint32_t shaderIndex)
{
    MAGMA_DEVICE_EXTENSION(vkCompileDeferredNV);
    const VkResult result = vkCompileDeferredNV(MAGMA_HANDLE(device), handle, shaderIndex);
    MAGMA_THROW_FAILURE(result, "failed to compile shader deferred");
}
#endif // VK_NV_ray_tracing
} // namespace magma
