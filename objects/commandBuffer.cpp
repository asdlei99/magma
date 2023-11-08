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
#include "commandBuffer.h"
#include "commandPool.h"
#include "framebuffer.h"
#include "imagelessFramebuffer.h"
#include "renderPass.h"
#include "imageView.h"
#include "fence.h"
#include "accelerationStructure.h"
#include "../shaders/shaderBindingTable.h"
#include "../misc/accelerationStructureGeometry.h"
#include "../misc/deviceFeatures.h"
#include "../exceptions/errorResult.h"
#include "../core/foreach.h"

namespace magma
{
CommandBuffer::CommandBuffer(VkCommandBufferLevel level, VkCommandBuffer handle, std::shared_ptr<CommandPool> cmdPool_):
    Dispatchable(VK_OBJECT_TYPE_COMMAND_BUFFER, handle, cmdPool_->getDevice(), nullptr),
    cmdPool(std::move(cmdPool_)),
    fence(std::make_shared<Fence>(device)),
    level(level),
    usageFlags(0),
    state(State::Initial),
    occlusionQueryEnable(VK_FALSE),
    conditionalRenderingEnable(VK_FALSE),
    negativeViewportHeightEnabled(device->getDeviceFeatures()->negativeViewportHeightEnabled()),
    withinRenderPass(VK_FALSE),
    withinConditionalRendering(VK_FALSE),
    withinTransformFeedback(VK_FALSE),
    queryFlags(0),
    pipelineStatistics(0)
{}

CommandBuffer::CommandBuffer(VkCommandBufferLevel level, std::shared_ptr<CommandPool> cmdPool_):
    Dispatchable(VK_OBJECT_TYPE_COMMAND_BUFFER, cmdPool_->getDevice(), nullptr),
    cmdPool(std::move(cmdPool_)),
    fence(std::make_shared<Fence>(device)),
    level(level),
    usageFlags(0),
    state(State::Initial),
    occlusionQueryEnable(VK_FALSE),
    conditionalRenderingEnable(VK_FALSE),
    negativeViewportHeightEnabled(device->getDeviceFeatures()->negativeViewportHeightEnabled()),
    withinRenderPass(VK_FALSE),
    withinConditionalRendering(VK_FALSE),
    withinTransformFeedback(VK_FALSE),
    queryFlags(0),
    pipelineStatistics(0)
{
    VkCommandBufferAllocateInfo cmdBufferAllocateInfo;
    cmdBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocateInfo.pNext = nullptr;
    cmdBufferAllocateInfo.commandPool = MAGMA_HANDLE(cmdPool);
    cmdBufferAllocateInfo.level = level;
    cmdBufferAllocateInfo.commandBufferCount = 1;
    const VkResult result = vkAllocateCommandBuffers(MAGMA_HANDLE(device), &cmdBufferAllocateInfo, &handle);
    MAGMA_HANDLE_RESULT(result, VK_COMMAND_BUFFER_LEVEL_PRIMARY == level ?
        "failed to allocate primary command buffer" : "failed to allocate secondary command buffer");
}

CommandBuffer::~CommandBuffer()
{   // Release if not freed through command pool
    if (handle)
        vkFreeCommandBuffers(MAGMA_HANDLE(device), MAGMA_HANDLE(cmdPool), 1, &handle);
}

bool CommandBuffer::begin(VkCommandBufferUsageFlags flags /* 0 */) noexcept
{
    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;
    cmdBufferBeginInfo.flags = flags;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;
    const VkResult result = vkBeginCommandBuffer(handle, &cmdBufferBeginInfo);
    MAGMA_ASSERT(VK_SUCCESS == result);
    if (VK_SUCCESS == result)
        state = State::Recording;
    usageFlags = flags;
    return (VK_SUCCESS == result);
}

bool CommandBuffer::beginInherited(const std::shared_ptr<RenderPass>& renderPass, uint32_t subpass, const std::shared_ptr<Framebuffer>& framebuffer,
    VkCommandBufferUsageFlags flags /* 0 */) noexcept
{
    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo;
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = nullptr;
    cmdBufferBeginInfo.flags = flags | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmdBufferBeginInfo.pInheritanceInfo = &cmdBufferInheritanceInfo;
    cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    cmdBufferInheritanceInfo.pNext = nullptr;
    cmdBufferInheritanceInfo.renderPass = *renderPass;
    cmdBufferInheritanceInfo.subpass = subpass;
    cmdBufferInheritanceInfo.framebuffer = *framebuffer;
    cmdBufferInheritanceInfo.occlusionQueryEnable = occlusionQueryEnable;
    cmdBufferInheritanceInfo.queryFlags = queryFlags;
    cmdBufferInheritanceInfo.pipelineStatistics = pipelineStatistics;
#ifdef VK_EXT_conditional_rendering
    VkCommandBufferInheritanceConditionalRenderingInfoEXT cmdBufferInheritanceConditionalRenderingInfo;
    if (device->extensionEnabled(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME))
    {
        cmdBufferInheritanceInfo.pNext = &cmdBufferInheritanceConditionalRenderingInfo;
        cmdBufferInheritanceConditionalRenderingInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT;
        cmdBufferInheritanceConditionalRenderingInfo.pNext = nullptr;
        cmdBufferInheritanceConditionalRenderingInfo.conditionalRenderingEnable = conditionalRenderingEnable;
    }
#endif // VK_EXT_conditional_rendering
    const VkResult result = vkBeginCommandBuffer(handle, &cmdBufferBeginInfo);
    MAGMA_ASSERT(VK_SUCCESS == result);
    if (VK_SUCCESS == result)
        state = State::Recording;
    usageFlags = flags;
    return (VK_SUCCESS == result);
}

void CommandBuffer::end()
{
    MAGMA_ASSERT(State::Recording == state);
    if (State::Recording == state)
    {
    #ifdef MAGMA_DEBUG_LABEL
        endDebugLabel();
    #endif // MAGMA_DEBUG_LABEL
        /* Performance - critical commands generally do not have return codes.
           If a run time error occurs in such commands, the implementation will defer
           reporting the error until a specified point. For commands that record
           into command buffers (vkCmd*), run time errors are reported by vkEndCommandBuffer. */
        const VkResult result = vkEndCommandBuffer(handle);
        // This is the only place where command buffer may throw an exception.
        MAGMA_HANDLE_RESULT(result, "failed to record command buffer");
        state = State::Executable;
    }
}

bool CommandBuffer::reset(bool releaseResources /* false */) noexcept
{   // The command buffer can be in any state other than pending
    MAGMA_ASSERT(state != State::Pending);
    VkCommandBufferResetFlags flags = 0;
    if (releaseResources)
        flags |= VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
    const VkResult result = vkResetCommandBuffer(handle, flags);
    MAGMA_ASSERT(VK_SUCCESS == result);
    if (VK_SUCCESS == result)
    {
        state = State::Initial;
        withinRenderPass = VK_FALSE;
        withinConditionalRendering = VK_FALSE;
        withinTransformFeedback = VK_FALSE;
        return true;
    }
    return false;
}

// inline void CommandBuffer::bindPipeline

void CommandBuffer::setViewport(float x, float y, float width, float height,
    float minDepth /* 0 */, float maxDepth /* 1 */) noexcept
{
    VkViewport viewport;
    viewport.x = x;
    viewport.y = y;
    if (height < 0)
    {
        if (device->getDeviceFeatures()->maintenanceEnabled(1))
            viewport.y = -height - y; // Move origin to bottom left
    }
    viewport.width = width;
    viewport.height = height;
    if (height < 0)
    {
        if (!negativeViewportHeightEnabled)
            viewport.height = -height; // Negative viewport height not supported
    }
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    vkCmdSetViewport(handle, 0, 1, &viewport);
}

// inline void CommandBuffer::setScissor
// inline void CommandBuffer::setLineWidth
// inline void CommandBuffer::setDepthBias
// inline void CommandBuffer::setBlendConstants
// inline void CommandBuffer::setDepthBounds
// inline void CommandBuffer::setStencilCompareMask
// inline void CommandBuffer::setStencilWriteMask
// inline void CommandBuffer::setStencilReference
// inline void CommandBuffer::bindDescriptorSet

void CommandBuffer::bindDescriptorSets(const std::shared_ptr<Pipeline>& pipeline, uint32_t firstSet, const std::initializer_list<std::shared_ptr<DescriptorSet>>& descriptorSets,
    const std::initializer_list<uint32_t>& dynamicOffsets /* {} */) noexcept
{
    MAGMA_ASSERT_FOR_EACH(descriptorSets, descriptorSet, pipeline->getLayout()->hasLayout(descriptorSet->getLayout()));
    MAGMA_ASSERT_FOR_EACH(descriptorSets, descriptorSet, !descriptorSet->dirty());
    MAGMA_STACK_ARRAY(VkDescriptorSet, dereferencedDescriptorSets, descriptorSets.size());
    for (const auto& descriptorSet: descriptorSets)
        dereferencedDescriptorSets.put(*descriptorSet);
    vkCmdBindDescriptorSets(handle, pipeline->getBindPoint(), *pipeline->getLayout(), firstSet, dereferencedDescriptorSets.size(), dereferencedDescriptorSets, MAGMA_COUNT(dynamicOffsets), dynamicOffsets.begin());
}

// inline void CommandBuffer::bindIndexBuffer
// inline void CommandBuffer::bindVertexBuffer
// inline void CommandBuffer::bindVertexBuffers

#ifdef VK_EXT_transform_feedback
void CommandBuffer::bindTransformFeedbackBuffer(uint32_t firstBinding, const std::shared_ptr<TransformFeedbackBuffer>& transformFeedbackBuffer,
    VkDeviceSize offset /* 0 */, VkDeviceSize size /* VK_WHOLE_SIZE */)
{
    MAGMA_DEVICE_EXTENSION(vkCmdBindTransformFeedbackBuffersEXT);
    if (vkCmdBindTransformFeedbackBuffersEXT)
        vkCmdBindTransformFeedbackBuffersEXT(handle, firstBinding, 1, transformFeedbackBuffer->getHandleAddress(), &offset, &size);
}

void CommandBuffer::bindTransformFeedbackBuffers(uint32_t firstBinding, const std::initializer_list<std::shared_ptr<TransformFeedbackBuffer>>& transformFeedbackBuffers,
    std::vector<VkDeviceSize> offsets /* empty */, const std::initializer_list<VkDeviceSize>& sizes /* empty */)
{
    MAGMA_ASSERT(transformFeedbackBuffers.size() > 0);
    if (offsets.size() > 0) {
        MAGMA_ASSERT(offsets.size() >= transformFeedbackBuffers.size());
    }
    if (sizes.size() > 0) {
        MAGMA_ASSERT(sizes.size() >= transformFeedbackBuffers.size());
    }
    MAGMA_DEVICE_EXTENSION(vkCmdBindTransformFeedbackBuffersEXT);
    if (vkCmdBindTransformFeedbackBuffersEXT)
    {
        MAGMA_STACK_ARRAY(VkBuffer, dereferencedBuffers, transformFeedbackBuffers.size());
        for (const auto& buffer : transformFeedbackBuffers)
            dereferencedBuffers.put(*buffer);
        if (offsets.empty())
            offsets.resize(transformFeedbackBuffers.size(), 0);
        vkCmdBindTransformFeedbackBuffersEXT(handle, firstBinding, dereferencedBuffers.size(), dereferencedBuffers, offsets.data(), sizes.begin());
    }
}
#endif // VK_EXT_transform_feedback

// inline void CommandBuffer::draw
// inline void CommandBuffer::drawInstanced
// inline void CommandBuffer::drawIndexed
// inline void CommandBuffer::drawIndexedInstanced
// inline void CommandBuffer::drawIndirect
// inline void CommandBuffer::drawIndexedIndirect
// inline void CommandBuffer::drawMulti
// inline void CommandBuffer::drawMultiInstanced
// inline void CommandBuffer::drawMultiIndexed
// inline void CommandBuffer::drawMultiIndexedInstanced
// inline void CommandBuffer::drawIndirectByteCount
// inline void CommandBuffer::dispatch
// inline void CommandBuffer::dispatchIndirect

void CommandBuffer::copyBuffer(const std::shared_ptr<const Buffer>& srcBuffer, const std::shared_ptr<Buffer>& dstBuffer,
    VkDeviceSize srcOffset /* 0 */, VkDeviceSize dstOffset /* 0 */, VkDeviceSize size /* VK_WHOLE_SIZE */) const noexcept
{
    VkBufferCopy bufferCopy;
    bufferCopy.srcOffset = srcOffset;
    bufferCopy.dstOffset = dstOffset;
    bufferCopy.size = (VK_WHOLE_SIZE == size) ? dstBuffer->getSize() : size;
    vkCmdCopyBuffer(handle, *srcBuffer, *dstBuffer, 1, &bufferCopy);
}

void CommandBuffer::copyImage(const std::shared_ptr<const Image>& srcImage, const std::shared_ptr<Image>& dstImage,
    uint32_t mipLevel /* 0 */, const VkOffset3D& srcOffset /* 0, 0, 0 */, const VkOffset3D& dstOffset /* 0, 0, 0 */) const noexcept
{
    VkImageCopy imageCopy;
    imageCopy.srcSubresource = srcImage->getSubresourceLayers(mipLevel);
    imageCopy.srcOffset = srcOffset;
    imageCopy.dstSubresource = dstImage->getSubresourceLayers(mipLevel);
    imageCopy.dstOffset = dstOffset;
    imageCopy.extent = dstImage->calculateMipExtent(mipLevel);
    vkCmdCopyImage(handle, *srcImage, srcImage->getLayout(), *dstImage, dstImage->getLayout(), 1, &imageCopy);
}

void CommandBuffer::blitImage(const std::shared_ptr<const Image>& srcImage, const std::shared_ptr<Image>& dstImage, VkFilter filter,
    uint32_t mipLevel /* 0 */, const VkOffset3D& srcOffset /* 0, 0, 0 */, const VkOffset3D& dstOffset /* 0, 0, 0 */) const noexcept
{
    const VkExtent3D srcExtent = srcImage->calculateMipExtent(mipLevel);
    const VkExtent3D dstExtent = dstImage->calculateMipExtent(mipLevel);
    VkImageBlit imageBlit;
    imageBlit.srcSubresource = srcImage->getSubresourceLayers(mipLevel);
    imageBlit.srcOffsets[0] = srcOffset;
    imageBlit.srcOffsets[1].x = srcExtent.width;
    imageBlit.srcOffsets[1].y = srcExtent.height,
    imageBlit.srcOffsets[1].z = 1;
    imageBlit.dstSubresource = dstImage->getSubresourceLayers(mipLevel);
    imageBlit.dstOffsets[0] = dstOffset;
    imageBlit.dstOffsets[1].x = dstExtent.width;
    imageBlit.dstOffsets[1].y = dstExtent.height,
    imageBlit.dstOffsets[1].z = 1;
    vkCmdBlitImage(handle, *srcImage, srcImage->getLayout(), *dstImage, dstImage->getLayout(), 1, &imageBlit, filter);
}

// inline void CommandBuffer::copyImage
// inline void CommandBuffer::blitImage
// inline void CommandBuffer::copyBufferToImage
// inline void CommandBuffer::copyImageToBuffer
// inline void CommandBuffer::updateBuffer

void CommandBuffer::fillBuffer(const std::shared_ptr<Buffer>& buffer, uint32_t value,
    VkDeviceSize size /* VK_WHOLE_SIZE */,
    VkDeviceSize offset /* 0 */) const noexcept
{
    vkCmdFillBuffer(handle, *buffer, offset, size, value);
}

// inline void CommandBuffer::clearColorImage
// inline void CommandBuffer::clearDepthStencilImage
// inline void CommandBuffer::clearAttachments
// inline void CommandBuffer::resolveImage
// inline void CommandBuffer::setEvent
// inline void CommandBuffer::resetEvent
// inline void CommandBuffer::waitEvent

void CommandBuffer::waitEvents(const std::vector<std::shared_ptr<Event>>& events, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    const std::vector<MemoryBarrier>& memoryBarriers /* {} */,
    const std::vector<BufferMemoryBarrier>& bufferMemoryBarriers /* {} */,
    const std::vector<ImageMemoryBarrier>& imageMemoryBarriers /* {} */) const noexcept
{
    MAGMA_ASSERT(srcStageMask);
    MAGMA_ASSERT(dstStageMask);
    MAGMA_STACK_ARRAY(VkEvent, dereferencedEvents, events.size());
    for (const auto& event : events)
        dereferencedEvents.put(*event);
    MAGMA_STACK_ARRAY(VkImageMemoryBarrier, dereferencedImageMemoryBarriers, imageMemoryBarriers.size());
    for (const auto& barrier : imageMemoryBarriers)
        dereferencedImageMemoryBarriers.put(barrier);
    vkCmdWaitEvents(handle, dereferencedEvents.size(), dereferencedEvents, srcStageMask, dstStageMask,
        MAGMA_COUNT(memoryBarriers),
        memoryBarriers.data(),
        MAGMA_COUNT(bufferMemoryBarriers),
        bufferMemoryBarriers.data(),
        MAGMA_COUNT(imageMemoryBarriers),
        dereferencedImageMemoryBarriers);
}

// inline void CommandBuffer::pipelineBarrier
// inline void CommandBuffer::pipelineBarrier
// inline void CommandBuffer::pipelineBarrier

void CommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
    const std::vector<MemoryBarrier>& memoryBarriers /* {} */,
    const std::vector<BufferMemoryBarrier>& bufferMemoryBarriers /* {} */,
    const std::vector<ImageMemoryBarrier>& imageMemoryBarriers /* {} */,
    VkDependencyFlags dependencyFlags /* 0 */) noexcept
{
    MAGMA_STACK_ARRAY(VkImageMemoryBarrier, dereferencedImageMemoryBarriers, imageMemoryBarriers.size());
    for (const auto& barrier : imageMemoryBarriers)
        dereferencedImageMemoryBarriers.put(barrier);
    vkCmdPipelineBarrier(handle, srcStageMask, dstStageMask, dependencyFlags,
        MAGMA_COUNT(memoryBarriers), memoryBarriers.data(),
        MAGMA_COUNT(bufferMemoryBarriers), bufferMemoryBarriers.data(),
        MAGMA_COUNT(imageMemoryBarriers), dereferencedImageMemoryBarriers);
    for (const auto& barrier : imageMemoryBarriers)
        barrier.resource->setLayout(barrier.newLayout);
}

// inline void CommandBuffer::beginQuery
// inline void CommandBuffer::endQuery
// inline void CommandBuffer::beginQueryIndexed
// inline void CommandBuffer::endQueryIndexed
// inline void CommandBuffer::resetQueryPool
// inline void CommandBuffer::writeTimestamp
// inline void CommandBuffer::copyQueryResults
// inline void CommandBuffer::copyQueryResultsWithAvailability
// inline void CommandBuffer::pushConstants()
// inline void CommandBuffer::pushConstantBlock()

void CommandBuffer::beginRenderPass(const std::shared_ptr<RenderPass>& renderPass, const std::shared_ptr<Framebuffer>& framebuffer,
    const std::vector<ClearValue>& clearValues /* {} */,
    const VkRect2D& renderArea /* {0, 0, 0, 0} */,
    VkSubpassContents contents /* VK_SUBPASS_CONTENTS_INLINE */) noexcept
{
    if (clearValues.empty()) {
        MAGMA_ASSERT(!renderPass->hasClearOp());
    }
    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = *renderPass;
    renderPassBeginInfo.framebuffer = *framebuffer;
    renderPassBeginInfo.renderArea.offset = renderArea.offset;
    renderPassBeginInfo.renderArea.extent = (renderArea.extent.width || renderArea.extent.height) ? renderArea.extent : framebuffer->getExtent();
    renderPassBeginInfo.clearValueCount = MAGMA_COUNT(clearValues);
    renderPassBeginInfo.pClearValues = reinterpret_cast<const VkClearValue *>(clearValues.data());
    vkCmdBeginRenderPass(handle, &renderPassBeginInfo, contents);
    withinRenderPass = VK_TRUE;
}

#ifdef VK_KHR_imageless_framebuffer
void CommandBuffer::beginRenderPass(const std::shared_ptr<RenderPass>& renderPass,
    const std::shared_ptr<ImagelessFramebuffer>& framebuffer,
    const std::vector<std::shared_ptr<ImageView>>& attachments,
    const std::vector<ClearValue>& clearValues /* {} */,
    const VkRect2D& renderArea /* {0, 0, 0, 0} */,
    VkSubpassContents contents /* VK_SUBPASS_CONTENTS_INLINE */) noexcept
{
    if (clearValues.empty()) {
        MAGMA_ASSERT(!renderPass->hasClearOp());
    }
    MAGMA_STACK_ARRAY(VkImageView, dereferencedAttachments, attachments.size());
    for (const auto& attachment : attachments)
        dereferencedAttachments.put(*attachment);
    VkRenderPassBeginInfo renderPassBeginInfo;
    VkRenderPassAttachmentBeginInfoKHR renderPassBeginAttachmentInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = &renderPassBeginAttachmentInfo;
    renderPassBeginInfo.renderPass = *renderPass;
    renderPassBeginInfo.framebuffer = *framebuffer;
    renderPassBeginInfo.renderArea.offset = renderArea.offset;
    renderPassBeginInfo.renderArea.extent = (renderArea.extent.width || renderArea.extent.height) ? renderArea.extent : framebuffer->getExtent();
    renderPassBeginInfo.clearValueCount = MAGMA_COUNT(clearValues);
    renderPassBeginInfo.pClearValues = reinterpret_cast<const VkClearValue *>(clearValues.data());
    renderPassBeginAttachmentInfo.sType =  VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR;
    renderPassBeginAttachmentInfo.pNext = nullptr;
    renderPassBeginAttachmentInfo.attachmentCount = MAGMA_COUNT(dereferencedAttachments);
    renderPassBeginAttachmentInfo.pAttachments = dereferencedAttachments;
    vkCmdBeginRenderPass(handle, &renderPassBeginInfo, contents);
    withinRenderPass = VK_TRUE;
}
#endif // VK_KHR_imageless_framebuffer

// inline CommandBuffer::nextSubpass
// inline CommandBuffer::endRenderPass

#ifdef VK_KHR_device_group
// inline CommandBuffer::setDeviceMask
// inline CommandBuffer::dispatchBase

bool CommandBuffer::beginDeviceGroup(uint32_t deviceMask,
    VkCommandBufferUsageFlags flags /* 0 */) noexcept
{
    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    VkDeviceGroupCommandBufferBeginInfo cmdBufferBeginDeviceGroupInfo;
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginInfo.pNext = &cmdBufferBeginDeviceGroupInfo;
    cmdBufferBeginInfo.flags = flags;
    cmdBufferBeginInfo.pInheritanceInfo = nullptr;
    cmdBufferBeginDeviceGroupInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferBeginDeviceGroupInfo.pNext = nullptr;
    cmdBufferBeginDeviceGroupInfo.deviceMask = deviceMask;
    const VkResult result = vkBeginCommandBuffer(handle, &cmdBufferBeginInfo);
    MAGMA_ASSERT(VK_SUCCESS == result);
    if (VK_SUCCESS == result)
        state = State::Recording;
    return (VK_SUCCESS == result);
}

void CommandBuffer::beginDeviceGroupRenderPass(uint32_t deviceMask,
    const std::shared_ptr<RenderPass>& renderPass, const std::shared_ptr<Framebuffer>& framebuffer,
    const std::vector<VkRect2D>& deviceRenderAreas /* {} */, const std::vector<ClearValue>& clearValues /* {} */,
    VkSubpassContents contents /* VK_SUBPASS_CONTENTS_INLINE */) noexcept
{
    if (clearValues.empty()) {
        MAGMA_ASSERT(!renderPass->hasClearOp());
    }
    VkRenderPassBeginInfo renderPassBeginInfo;
    VkDeviceGroupRenderPassBeginInfo renderPassBeginDeviceGroupInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = &renderPassBeginDeviceGroupInfo;
    renderPassBeginInfo.renderPass = *renderPass;
    renderPassBeginInfo.framebuffer = *framebuffer;
    renderPassBeginInfo.renderArea.offset = VkOffset2D{0, 0};
    renderPassBeginInfo.renderArea.extent = deviceRenderAreas.empty() ? framebuffer->getExtent() : VkExtent2D{0, 0};
    renderPassBeginInfo.clearValueCount = MAGMA_COUNT(clearValues);
    renderPassBeginInfo.pClearValues = reinterpret_cast<const VkClearValue *>(clearValues.data());
    renderPassBeginDeviceGroupInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO;
    renderPassBeginDeviceGroupInfo.pNext = nullptr;
    renderPassBeginDeviceGroupInfo.deviceMask = deviceMask;
    renderPassBeginDeviceGroupInfo.deviceRenderAreaCount = MAGMA_COUNT(deviceRenderAreas);
    renderPassBeginDeviceGroupInfo.pDeviceRenderAreas = deviceRenderAreas.data();
    vkCmdBeginRenderPass(handle, &renderPassBeginInfo, contents);
    withinRenderPass = VK_TRUE;
}

#ifdef VK_KHR_imageless_framebuffer
void CommandBuffer::beginDeviceGroupRenderPass(uint32_t deviceMask,
    const std::shared_ptr<RenderPass>& renderPass,
    const std::shared_ptr<ImagelessFramebuffer>& framebuffer,
    const std::vector<std::shared_ptr<ImageView>>& attachments,
    const std::vector<VkRect2D>& deviceRenderAreas /* {} */,
    const std::vector<ClearValue>& clearValues /* {} */,
    VkSubpassContents contents /* VK_SUBPASS_CONTENTS_INLINE */) noexcept
{
    if (clearValues.empty()) {
        MAGMA_ASSERT(!renderPass->hasClearOp());
    }
    MAGMA_STACK_ARRAY(VkImageView, dereferencedAttachments, attachments.size());
    for (const auto& attachment : attachments)
        dereferencedAttachments.put(*attachment);
    VkRenderPassBeginInfo renderPassBeginInfo;
    VkDeviceGroupRenderPassBeginInfo renderPassBeginDeviceGroupInfo;
    VkRenderPassAttachmentBeginInfoKHR renderPassBeginAttachmentInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = &renderPassBeginDeviceGroupInfo;
    renderPassBeginInfo.renderPass = *renderPass;
    renderPassBeginInfo.framebuffer = *framebuffer;
    renderPassBeginInfo.renderArea.offset = VkOffset2D{0, 0};
    renderPassBeginInfo.renderArea.extent = deviceRenderAreas.empty() ? framebuffer->getExtent() : VkExtent2D{0, 0};
    renderPassBeginInfo.clearValueCount = MAGMA_COUNT(clearValues);
    renderPassBeginInfo.pClearValues = reinterpret_cast<const VkClearValue *>(clearValues.data());
    renderPassBeginDeviceGroupInfo.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO;
    renderPassBeginDeviceGroupInfo.pNext = &renderPassBeginAttachmentInfo;
    renderPassBeginDeviceGroupInfo.deviceMask = deviceMask;
    renderPassBeginDeviceGroupInfo.deviceRenderAreaCount = MAGMA_COUNT(deviceRenderAreas);
    renderPassBeginDeviceGroupInfo.pDeviceRenderAreas = deviceRenderAreas.data();
    renderPassBeginAttachmentInfo.sType =  VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO_KHR;
    renderPassBeginAttachmentInfo.pNext = nullptr;
    renderPassBeginAttachmentInfo.attachmentCount = MAGMA_COUNT(dereferencedAttachments);
    renderPassBeginAttachmentInfo.pAttachments = dereferencedAttachments;
    vkCmdBeginRenderPass(handle, &renderPassBeginInfo, contents);
    withinRenderPass = VK_TRUE;
}
#endif // VK_KHR_imageless_framebuffer
#endif // VK_KHR_device_group

#ifdef VK_EXT_conditional_rendering
void CommandBuffer::beginConditionalRendering(const std::shared_ptr<Buffer>& buffer,
    VkDeviceSize offset /* 0 */,
    bool inverted /* false */) noexcept
{
    MAGMA_ASSERT(offset <= buffer->getSize() - sizeof(uint32_t));
    MAGMA_ASSERT(offset % sizeof(uint32_t) == 0);
    MAGMA_DEVICE_EXTENSION(vkCmdBeginConditionalRenderingEXT);
    if (vkCmdBeginConditionalRenderingEXT)
    {
        VkConditionalRenderingBeginInfoEXT conditionalRenderingBeginInfo;
        conditionalRenderingBeginInfo.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
        conditionalRenderingBeginInfo.pNext = nullptr;
        conditionalRenderingBeginInfo.buffer = *buffer;
        conditionalRenderingBeginInfo.offset = offset;
        conditionalRenderingBeginInfo.flags = inverted ? VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT : 0;
        vkCmdBeginConditionalRenderingEXT(handle, &conditionalRenderingBeginInfo);
        withinConditionalRendering = VK_TRUE;
    }
}

void CommandBuffer::endConditionalRendering() noexcept
{
    MAGMA_ASSERT(withinConditionalRendering);
    MAGMA_DEVICE_EXTENSION(vkCmdEndConditionalRenderingEXT);
    if (vkCmdEndConditionalRenderingEXT)
    {
        vkCmdEndConditionalRenderingEXT(handle);
        withinConditionalRendering = VK_FALSE;
    }
}
#endif // VK_EXT_conditional_rendering

#ifdef VK_EXT_transform_feedback
// inline void CommandBuffer::beginTransformFeedback()
// inline void CommandBuffer::endTransformFeedback()

void CommandBuffer::beginTransformFeedback(uint32_t firstCounterBuffer, const std::initializer_list<std::shared_ptr<TransformFeedbackCounterBuffer>>& counterBuffers,
    const std::initializer_list<VkDeviceSize>& counterBufferOffsets /* empty */) noexcept
{
    if (counterBufferOffsets.size() > 0) {
        MAGMA_ASSERT(counterBufferOffsets.size() >= counterBuffers.size());
    }
    MAGMA_DEVICE_EXTENSION(vkCmdBeginTransformFeedbackEXT);
    if (vkCmdBeginTransformFeedbackEXT)
    {
        MAGMA_STACK_ARRAY(VkBuffer, dereferencedCounterBuffers, counterBuffers.size());
        for (const auto& buffer : counterBuffers)
            dereferencedCounterBuffers.put(*buffer);
        vkCmdBeginTransformFeedbackEXT(handle, firstCounterBuffer, dereferencedCounterBuffers.size(), dereferencedCounterBuffers, counterBufferOffsets.begin());
        withinTransformFeedback = VK_TRUE;
    }
}

void CommandBuffer::endTransformFeedback(uint32_t firstCounterBuffer, const std::initializer_list<std::shared_ptr<TransformFeedbackCounterBuffer>>& counterBuffers,
    const std::initializer_list<VkDeviceSize>& counterBufferOffsets /* empty */) noexcept
{
    MAGMA_ASSERT(withinTransformFeedback);
    if (counterBufferOffsets.size() > 0) {
        MAGMA_ASSERT(counterBufferOffsets.size() >= counterBuffers.size());
    }
    MAGMA_DEVICE_EXTENSION(vkCmdEndTransformFeedbackEXT);
    if (vkCmdEndTransformFeedbackEXT)
    {
        MAGMA_STACK_ARRAY(VkBuffer, dereferencedCounterBuffers, counterBuffers.size());
        for (const auto& buffer : counterBuffers)
            dereferencedCounterBuffers.put(*buffer);
        vkCmdEndTransformFeedbackEXT(handle, firstCounterBuffer, dereferencedCounterBuffers.size(), dereferencedCounterBuffers, counterBufferOffsets.begin());
        withinTransformFeedback = VK_FALSE;
    }
}
#endif // VK_EXT_transform_feedback

#ifdef VK_KHR_acceleration_structure
void CommandBuffer::buildAccelerationStructure(std::shared_ptr<AccelerationStructure>& accelerationStructure, std::shared_ptr<Buffer> scratchBuffer,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<AccelerationStructureBuildRange>& buildRanges,
    VkBuildAccelerationStructureFlagsKHR flags) noexcept
{
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = nullptr;
    accelerationStructureBuildGeometryInfo.type = accelerationStructure->getType();
    accelerationStructureBuildGeometryInfo.flags = flags;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = *accelerationStructure;
    accelerationStructureBuildGeometryInfo.geometryCount = MAGMA_COUNT(geometries);
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();
    accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();
    const VkAccelerationStructureBuildRangeInfoKHR *buildRangeInfos[1] = {buildRanges.data()};
    MAGMA_DEVICE_EXTENSION(vkCmdBuildAccelerationStructuresKHR);
    if (vkCmdBuildAccelerationStructuresKHR)
        vkCmdBuildAccelerationStructuresKHR(handle, 1, &accelerationStructureBuildGeometryInfo, buildRangeInfos);
}

void CommandBuffer::updateAccelerationStructure(std::shared_ptr<AccelerationStructure>& accelerationStructure, std::shared_ptr<Buffer> scratchBuffer,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<AccelerationStructureBuildRange>& buildRanges,
    VkBuildAccelerationStructureFlagsKHR flags) noexcept
{
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = nullptr;
    accelerationStructureBuildGeometryInfo.type = accelerationStructure->getType();
    accelerationStructureBuildGeometryInfo.flags = flags;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = *accelerationStructure; // Update
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = *accelerationStructure; // in-place
    accelerationStructureBuildGeometryInfo.geometryCount = MAGMA_COUNT(geometries);
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();
    accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();
    const VkAccelerationStructureBuildRangeInfoKHR *buildRangeInfos[1] = {buildRanges.data()};
    MAGMA_DEVICE_EXTENSION(vkCmdBuildAccelerationStructuresKHR);
    if (vkCmdBuildAccelerationStructuresKHR)
        vkCmdBuildAccelerationStructuresKHR(handle, 1, &accelerationStructureBuildGeometryInfo, buildRangeInfos);
}

void CommandBuffer::buildAccelerationStructures(const std::vector<std::shared_ptr<AccelerationStructure>>& accelerationStructures, std::shared_ptr<Buffer> scratchBuffer,
    const std::list<std::vector<AccelerationStructureGeometry>>& geometryList, const std::list<std::vector<AccelerationStructureBuildRange>>& buildRangeList,
    VkBuildAccelerationStructureFlagsKHR flags)
{
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> accelerationStructureBuildGeometryInfos;
    accelerationStructureBuildGeometryInfos.reserve(accelerationStructures.size());
    core::forConstEach(accelerationStructures, geometryList,
        [&accelerationStructureBuildGeometryInfos, flags, &scratchBuffer](auto accelerationStructureIt, auto geometriesIt)
        {
            std::shared_ptr<AccelerationStructure> accelerationStructure = *accelerationStructureIt;
            VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
            accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            accelerationStructureBuildGeometryInfo.pNext = nullptr;
            accelerationStructureBuildGeometryInfo.type = accelerationStructure->getType();
            accelerationStructureBuildGeometryInfo.flags = flags;
            accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
            accelerationStructureBuildGeometryInfo.dstAccelerationStructure = *accelerationStructure;
            accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometriesIt->size());
            accelerationStructureBuildGeometryInfo.pGeometries = geometriesIt->data();
            accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
            accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress(); // TODO: offset!
            accelerationStructureBuildGeometryInfos.push_back(accelerationStructureBuildGeometryInfo);
        });
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR *> buildRangeInfos;
    buildRangeInfos.reserve(buildRangeList.size());
    for (const auto& range: buildRangeList)
        buildRangeInfos.push_back(range.data());
    MAGMA_DEVICE_EXTENSION(vkCmdBuildAccelerationStructuresKHR);
    if (vkCmdBuildAccelerationStructuresKHR)
    {
        vkCmdBuildAccelerationStructuresKHR(handle,
            MAGMA_COUNT(accelerationStructureBuildGeometryInfos),
            accelerationStructureBuildGeometryInfos.data(),
            buildRangeInfos.data());
    }
}

void CommandBuffer::updateAccelerationStructures(const std::vector<std::shared_ptr<AccelerationStructure>>& accelerationStructures, std::shared_ptr<Buffer> scratchBuffer,
    const std::list<std::vector<AccelerationStructureGeometry>>& geometryList, const std::list<std::vector<AccelerationStructureBuildRange>>& buildRangeList,
    VkBuildAccelerationStructureFlagsKHR flags)
{
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> accelerationStructureBuildGeometryInfos;
    accelerationStructureBuildGeometryInfos.reserve(accelerationStructures.size());
    core::forConstEach(accelerationStructures, geometryList,
        [&accelerationStructureBuildGeometryInfos, flags, &scratchBuffer](auto accelerationStructureIt, auto geometriesIt)
        {
            std::shared_ptr<AccelerationStructure> accelerationStructure = *accelerationStructureIt;
            VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
            accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            accelerationStructureBuildGeometryInfo.pNext = nullptr;
            accelerationStructureBuildGeometryInfo.type = accelerationStructure->getType();
            accelerationStructureBuildGeometryInfo.flags = flags;
            accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
            accelerationStructureBuildGeometryInfo.srcAccelerationStructure = *accelerationStructure; // Update
            accelerationStructureBuildGeometryInfo.dstAccelerationStructure = *accelerationStructure; // in-place
            accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometriesIt->size());
            accelerationStructureBuildGeometryInfo.pGeometries = geometriesIt->data();
            accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
            accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress(); // TODO: offset!
            accelerationStructureBuildGeometryInfos.push_back(accelerationStructureBuildGeometryInfo);
        });
    std::vector<const VkAccelerationStructureBuildRangeInfoKHR *> buildRangeInfos;
    buildRangeInfos.reserve(buildRangeList.size());
    for (const auto& range: buildRangeList)
        buildRangeInfos.push_back(range.data());
    MAGMA_DEVICE_EXTENSION(vkCmdBuildAccelerationStructuresKHR);
    if (vkCmdBuildAccelerationStructuresKHR)
    {
        vkCmdBuildAccelerationStructuresKHR(handle,
            MAGMA_COUNT(accelerationStructureBuildGeometryInfos),
            accelerationStructureBuildGeometryInfos.data(),
            buildRangeInfos.data());
    }
}

void CommandBuffer::buildAccelerationStructureIndirect(std::shared_ptr<AccelerationStructure>& accelerationStructure,
    std::shared_ptr<Buffer>& scratchBuffer, std::shared_ptr<const Buffer>& buildRangeInfos, uint32_t stride,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts_,
    VkBuildAccelerationStructureFlagsKHR flags) noexcept
{
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = nullptr;
    accelerationStructureBuildGeometryInfo.type = accelerationStructure->getType();
    accelerationStructureBuildGeometryInfo.flags = flags;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = *accelerationStructure;
    accelerationStructureBuildGeometryInfo.geometryCount = MAGMA_COUNT(geometries);
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();
    accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();
    const VkDeviceAddress indirectDeviceAddresses[1] = {buildRangeInfos->getDeviceAddress()};
    const uint32_t *maxPrimitiveCounts[1] = {maxPrimitiveCounts_.data()};
    MAGMA_DEVICE_EXTENSION(vkCmdBuildAccelerationStructuresIndirectKHR);
    if (vkCmdBuildAccelerationStructuresIndirectKHR)
    {
        vkCmdBuildAccelerationStructuresIndirectKHR(handle, 1, &accelerationStructureBuildGeometryInfo,
            indirectDeviceAddresses, &stride, maxPrimitiveCounts);
    }
}

void CommandBuffer::copyAccelerationStructure(std::shared_ptr<AccelerationStructure> dst, std::shared_ptr<const AccelerationStructure> src, VkCopyAccelerationStructureModeKHR mode) const noexcept
{
    MAGMA_ASSERT(dst);
    MAGMA_ASSERT(src);
    VkCopyAccelerationStructureInfoKHR copyAccelerationStructureInfo;
    copyAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    copyAccelerationStructureInfo.pNext = nullptr;
    copyAccelerationStructureInfo.src = *src;
    copyAccelerationStructureInfo.dst = *dst;
    copyAccelerationStructureInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCmdCopyAccelerationStructureKHR);
    if (vkCmdCopyAccelerationStructureKHR)
        vkCmdCopyAccelerationStructureKHR(handle, &copyAccelerationStructureInfo);
}

void CommandBuffer::copyAccelerationStructureToMemory(std::shared_ptr<Buffer> dst, std::shared_ptr<const AccelerationStructure> src, VkCopyAccelerationStructureModeKHR mode) const noexcept
{
    MAGMA_ASSERT(dst);
    MAGMA_ASSERT(src);
    VkCopyAccelerationStructureToMemoryInfoKHR copyAccelerationStructureToMemoryInfo;
    copyAccelerationStructureToMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
    copyAccelerationStructureToMemoryInfo.pNext = nullptr;
    copyAccelerationStructureToMemoryInfo.src = *src;
    copyAccelerationStructureToMemoryInfo.dst.deviceAddress = dst->getDeviceAddress();
    copyAccelerationStructureToMemoryInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCmdCopyAccelerationStructureToMemoryKHR);
    if (vkCmdCopyAccelerationStructureToMemoryKHR)
        vkCmdCopyAccelerationStructureToMemoryKHR(handle, &copyAccelerationStructureToMemoryInfo);
}

void CommandBuffer::copyAccelerationStructureToMemory(void *dst, std::shared_ptr<const AccelerationStructure> src, VkCopyAccelerationStructureModeKHR mode) const noexcept
{
    MAGMA_ASSERT(dst);
    MAGMA_ASSERT(src);
    VkCopyAccelerationStructureToMemoryInfoKHR copyAccelerationStructureToMemoryInfo;
    copyAccelerationStructureToMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
    copyAccelerationStructureToMemoryInfo.pNext = nullptr;
    copyAccelerationStructureToMemoryInfo.src = *src;
    copyAccelerationStructureToMemoryInfo.dst.hostAddress = dst;
    copyAccelerationStructureToMemoryInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCmdCopyAccelerationStructureToMemoryKHR);
    if (vkCmdCopyAccelerationStructureToMemoryKHR)
        vkCmdCopyAccelerationStructureToMemoryKHR(handle, &copyAccelerationStructureToMemoryInfo);
}

void CommandBuffer::copyMemoryToAccelerationStructure(std::shared_ptr<AccelerationStructure> dst, std::shared_ptr<const Buffer> src, VkCopyAccelerationStructureModeKHR mode) const noexcept
{
    MAGMA_ASSERT(dst);
    MAGMA_ASSERT(src);
    VkCopyMemoryToAccelerationStructureInfoKHR copyMemoryToAccelerationStructureInfo;
    copyMemoryToAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
    copyMemoryToAccelerationStructureInfo.pNext = nullptr;
    copyMemoryToAccelerationStructureInfo.src.deviceAddress = src->getDeviceAddress();
    copyMemoryToAccelerationStructureInfo.dst = *dst;
    copyMemoryToAccelerationStructureInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCmdCopyMemoryToAccelerationStructureKHR);
    if (vkCmdCopyMemoryToAccelerationStructureKHR)
        vkCmdCopyMemoryToAccelerationStructureKHR(handle, &copyMemoryToAccelerationStructureInfo);
}

void CommandBuffer::copyMemoryToAccelerationStructure(std::shared_ptr<AccelerationStructure> dst, const void *src, VkCopyAccelerationStructureModeKHR mode) const noexcept
{
    MAGMA_ASSERT(dst);
    MAGMA_ASSERT(src);
    VkCopyMemoryToAccelerationStructureInfoKHR copyMemoryToAccelerationStructureInfo;
    copyMemoryToAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
    copyMemoryToAccelerationStructureInfo.pNext = nullptr;
    copyMemoryToAccelerationStructureInfo.src.hostAddress = src;
    copyMemoryToAccelerationStructureInfo.dst = *dst;
    copyMemoryToAccelerationStructureInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCmdCopyMemoryToAccelerationStructureKHR);
    if (vkCmdCopyMemoryToAccelerationStructureKHR)
        vkCmdCopyMemoryToAccelerationStructureKHR(handle, &copyMemoryToAccelerationStructureInfo);
}

void CommandBuffer::writeAccelerationStructureProperties(std::shared_ptr<const AccelerationStructure> accelerationStructure, std::shared_ptr<QueryPool> queryPool,
    uint32_t firstQuery /* 0 */)
{
    const VkAccelerationStructureKHR dereferencedAccelerationStructures[1] = {*accelerationStructure};
    MAGMA_DEVICE_EXTENSION(vkCmdWriteAccelerationStructuresPropertiesKHR);
    if (vkCmdWriteAccelerationStructuresPropertiesKHR)
        vkCmdWriteAccelerationStructuresPropertiesKHR(handle, 1, dereferencedAccelerationStructures, queryPool->getType(), *queryPool, firstQuery);
}

void CommandBuffer::writeAccelerationStructuresProperties(std::vector<std::shared_ptr<const AccelerationStructure>> accelerationStructures, std::shared_ptr<QueryPool> queryPool,
    uint32_t firstQuery /* 0 */)
{
    MAGMA_STACK_ARRAY(VkAccelerationStructureKHR, dereferencedAccelerationStructures, accelerationStructures.size());
    for (const auto& as: accelerationStructures)
        dereferencedAccelerationStructures.put(*as);
    MAGMA_DEVICE_EXTENSION(vkCmdWriteAccelerationStructuresPropertiesKHR);
    if (vkCmdWriteAccelerationStructuresPropertiesKHR)
        vkCmdWriteAccelerationStructuresPropertiesKHR(handle, dereferencedAccelerationStructures.size(), dereferencedAccelerationStructures, queryPool->getType(), *queryPool, firstQuery);
}
#endif // VK_KHR_acceleration_structure

#ifdef VK_KHR_ray_tracing_pipeline
void CommandBuffer::setRayTracingPipelineStackSize(uint32_t pipelineStackSize) const noexcept
{
    MAGMA_DEVICE_EXTENSION(vkCmdSetRayTracingPipelineStackSizeKHR);
    if (vkCmdSetRayTracingPipelineStackSizeKHR)
        vkCmdSetRayTracingPipelineStackSizeKHR(handle, pipelineStackSize);
}

void CommandBuffer::traceRays(const std::shared_ptr<ShaderBindingTable>& raygenShaderBindingTable,
    const std::shared_ptr<ShaderBindingTable>& missShaderBindingTable,
    const std::shared_ptr<ShaderBindingTable>& hitShaderBindingTable,
    const std::shared_ptr<ShaderBindingTable>& callableShaderBindingTable,
    uint32_t width, uint32_t height, uint32_t depth) const noexcept
{
    MAGMA_DEVICE_EXTENSION(vkCmdTraceRaysKHR);
    if (vkCmdTraceRaysKHR)
    {
        vkCmdTraceRaysKHR(handle,
            &raygenShaderBindingTable->getDeviceAddressRegion(),
            &missShaderBindingTable->getDeviceAddressRegion(),
            &hitShaderBindingTable->getDeviceAddressRegion(),
            &callableShaderBindingTable->getDeviceAddressRegion(),
            width, height, depth);
    }
}

void CommandBuffer::traceRaysIndirect(const std::shared_ptr<ShaderBindingTable>& raygenShaderBindingTable,
    const std::shared_ptr<ShaderBindingTable>& missShaderBindingTable,
    const std::shared_ptr<ShaderBindingTable>& hitShaderBindingTable,
    const std::shared_ptr<ShaderBindingTable>& callableShaderBindingTable,
    const std::shared_ptr<Buffer>& indirectTraceRaysBuffer) const noexcept
{
    MAGMA_DEVICE_EXTENSION(vkCmdTraceRaysIndirectKHR);
    if (vkCmdTraceRaysIndirectKHR)
    {
        vkCmdTraceRaysIndirectKHR(handle,
            &raygenShaderBindingTable->getDeviceAddressRegion(),
            &missShaderBindingTable->getDeviceAddressRegion(),
            &hitShaderBindingTable->getDeviceAddressRegion(),
            &callableShaderBindingTable->getDeviceAddressRegion(),
            indirectTraceRaysBuffer->getDeviceAddress());
    }
}
#endif // VK_KHR_ray_tracing_pipeline
} // namespace magma
