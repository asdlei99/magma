/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2020 Victor Coda.

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
#include <algorithm>
#include "renderPass.h"
#include "device.h"
#include "physicalDevice.h"
#include "../allocator/allocator.h"
#include "../helpers/stackArray.h"
#include "../misc/format.h"
#include "../exceptions/errorResult.h"

namespace magma
{
RenderPass::RenderPass(std::shared_ptr<Device> device, const AttachmentDescription& attachment,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    RenderPass(std::move(device), std::vector<AttachmentDescription>{attachment}, std::move(allocator))
{}

RenderPass::RenderPass(std::shared_ptr<Device> device,
    const std::vector<AttachmentDescription>& attachments,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_RENDER_PASS, std::move(device), std::move(allocator)),
    attachments(attachments),
    hash(0)
{
    uint32_t multisampleAttachmentCount = 0;
    uint32_t resolveAttachmentCount = 0;
    for (const auto& attachmentDesc : attachments)
    {
        const Format format(attachmentDesc.format);
        if (!format.depth() && !format.stencil() && !format.depthStencil())
        {
            if (attachmentDesc.samples > 1)
                ++multisampleAttachmentCount;
            else
                ++resolveAttachmentCount;
        }
    }
    const uint32_t colorAttachmentCount = multisampleAttachmentCount ? multisampleAttachmentCount : resolveAttachmentCount;
    resolveAttachmentCount = std::max(0U, multisampleAttachmentCount);
    MAGMA_STACK_ARRAY(VkAttachmentReference, colorAttachments, colorAttachmentCount);
    MAGMA_STACK_ARRAY(VkAttachmentReference, resolveAttachments, resolveAttachmentCount);
    VkAttachmentReference depthStencilAttachment = {0, VK_IMAGE_LAYOUT_UNDEFINED};
    bool hasDepthStencilAttachment = false;
    uint32_t attachmentIndex = 0, colorIndex = 0, resolveIndex = 0;
    for (const auto& attachmentDesc : attachments)
    {
        const Format format(attachmentDesc.format);
        if (format.depth() || format.stencil() || format.depthStencil())
        {
            if (VK_IMAGE_LAYOUT_UNDEFINED == depthStencilAttachment.layout)
            {
                const VkImageLayout depthStencilLayout = optimalDepthStencilLayout(format);
                depthStencilAttachment = {attachmentIndex, depthStencilLayout};
                hasDepthStencilAttachment = true;
            }
        }
        else
        {
            if (attachmentDesc.samples > 1 || resolveAttachmentCount < 1)
                colorAttachments[colorIndex++] = {attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            else
                resolveAttachments[resolveIndex++] = {attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        ++attachmentIndex;
    }
    // Describe render pass
    SubpassDescription subpass;
    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = colorAttachmentCount;
    subpass.pColorAttachments = colorAttachments;
    subpass.pResolveAttachments = resolveAttachments;
    subpass.pDepthStencilAttachment = hasDepthStencilAttachment ? &depthStencilAttachment : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    SubpassDependency dependencies[] = {
        // Dependency at the beginning of the render pass
        subpassStartDependency(colorAttachmentCount > 0, hasDepthStencilAttachment),
        // Dependency at the end of the render pass
        subpassEndDependency(colorAttachmentCount > 0, hasDepthStencilAttachment)
    };
    // Create render pass
    VkRenderPassCreateInfo renderPassInfo;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = MAGMA_COUNT(attachments);
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;
    const VkResult result = vkCreateRenderPass(MAGMA_HANDLE(device), &renderPassInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create render pass");
    hash = core::hashArgs(
        renderPassInfo.sType,
        renderPassInfo.flags,
        renderPassInfo.attachmentCount,
        renderPassInfo.subpassCount,
        renderPassInfo.dependencyCount);
    for (const auto& attachment : attachments)
        core::hashCombine(hash, attachment.hash());
    core::hashCombine(hash, subpass.hash());
    core::memzero(subpass); // Aware destructor
    for (const auto& dependency : dependencies)
        core::hashCombine(hash, dependency.hash());
}

RenderPass::RenderPass(std::shared_ptr<Device> device,
    const std::vector<AttachmentDescription>& attachments,
    const std::vector<SubpassDescription>& subpasses,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    RenderPass(std::move(device), attachments, subpasses, {}, std::move(allocator))
{}

RenderPass::RenderPass(std::shared_ptr<Device> device,
    const std::vector<AttachmentDescription>& attachments,
    const std::vector<SubpassDescription>& subpasses,
    const std::vector<SubpassDependency>& dependencies,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_RENDER_PASS, std::move(device), std::move(allocator)),
    hash(0)
{
    VkRenderPassCreateInfo renderPassInfo;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.flags = 0;
    renderPassInfo.attachmentCount = MAGMA_COUNT(attachments);
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = MAGMA_COUNT(subpasses);
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = MAGMA_COUNT(dependencies);
    renderPassInfo.pDependencies = dependencies.data();
    const VkResult result = vkCreateRenderPass(MAGMA_HANDLE(device), &renderPassInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create render pass");
    hash = core::hashArgs(
        renderPassInfo.sType,
        renderPassInfo.flags,
        renderPassInfo.attachmentCount,
        renderPassInfo.subpassCount,
        renderPassInfo.dependencyCount);
    for (const auto& attachment : attachments)
        core::hashCombine(hash, attachment.hash());
    for (const auto& subpass : subpasses)
        core::hashCombine(hash, subpass.hash());
    for (const auto& dependency : dependencies)
        core::hashCombine(hash, dependency.hash());
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

bool RenderPass::hasClearOp() const noexcept
{
    for (const auto& attachment : attachments)
    {
        if (VK_ATTACHMENT_LOAD_OP_CLEAR == attachment.loadOp)
            return true;
    }
    return false;
}

VkImageLayout RenderPass::optimalDepthStencilLayout(const Format& format) const
{
#ifdef VK_KHR_separate_depth_stencil_layouts
    if (device->getPhysicalDevice()->checkExtensionSupport(VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME) &&
        device->separateDepthStencilLayoutsEnabled())
    {
        if (format.depth())
            return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
        if (format.stencil())
            return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR;
    }
#else
    MAGMA_UNUSED(format);
#endif // VK_KHR_separate_depth_stencil_layouts
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

SubpassDependency RenderPass::subpassStartDependency(bool colorAttachment, bool depthStencilAttachment) const noexcept
{
    SubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependency.dstStageMask = 0;
    subpassDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassDependency.dstAccessMask = 0;
    if (colorAttachment)
    {
        subpassDependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (depthStencilAttachment)
    {
        subpassDependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        subpassDependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    return subpassDependency;
}

SubpassDependency RenderPass::subpassEndDependency(bool colorAttachment, bool depthStencilAttachment) const noexcept
{
    SubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    if (colorAttachment)
    {
        subpassDependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subpassDependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (depthStencilAttachment)
    {
        subpassDependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    return subpassDependency;
}
/*
MultiviewRenderPass::MultiviewRenderPass(std::shared_ptr<Device> device, uint32_t viewMask, uint32_t correlationMask,
    std::shared_ptr<IAllocator> allocator * nullptr *)
{
    VkRenderPassMultiviewCreateInfoKHR renderPassMultiviewInfo;
    renderPassMultiviewInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO_KHR;
    renderPassMultiviewInfo.pNext = nullptr;
    renderPassMultiviewInfo.subpassCount = 1;
    renderPassMultiviewInfo.pViewMasks = &viewMask;
    renderPassMultiviewInfo.correlationMaskCount = 1;
    renderPassMultiviewInfo.pCorrelationMasks = &correlationMask;
    // Create render pass
    VkRenderPassCreateInfo renderPassInfo;
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pNext = &renderPassMultiviewInfo;
    renderPassInfo.flags = 0;
    //renderPassInfo.attachmentCount = MAGMA_COUNT(attachments);
    //renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = renderPassMultiviewInfo.subpassCount;
    //renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    //renderPassInfo.pDependencies = dependencies;
    const VkResult result = vkCreateRenderPass(MAGMA_HANDLE(device), &renderPassInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create multi-view render pass");
    hash = core::hashArgs(
        renderPassMultiviewInfo.sType,
        renderPassMultiviewInfo.subpassCount,
        *renderPassMultiviewInfo.pViewMasks,
        renderPassMultiviewInfo.correlationMaskCount,
        *renderPassMultiviewInfo.pCorrelationMasks,
        renderPassInfo.sType,
        renderPassInfo.flags,
        renderPassInfo.attachmentCount,
        renderPassInfo.subpassCount,
        renderPassInfo.dependencyCount);
    //for (const auto& attachment : attachments)
    //    core::hashCombine(hash, attachment.hash());
    //core::hashCombine(hash, subpass.hash());
    //core::memzero(subpass); // Aware destructor
    //for (const auto& dependency : dependencies)
    //    core::hashCombine(hash, dependency.hash());
}*/
} // namespace magma
