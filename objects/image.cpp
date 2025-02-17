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
#include "image.h"
#include "srcTransferBuffer.h"
#include "device.h"
#include "physicalDevice.h"
#include "deviceMemory.h"
#include "managedDeviceMemory.h"
#include "queue.h"
#include "fence.h"
#include "commandBuffer.h"
#include "../barriers/imageMemoryBarrier.h"
#include "../misc/format.h"
#include "../misc/structureChain.h"
#include "../exceptions/errorResult.h"

// Vulkan validation layers may complain about image regions for block-compressed formats. See:
// https://vulkan.lunarg.com/doc/view/1.3.224.1/windows/1.3-extensions/vkspec.html#VUID-vkCmdCopyBufferToImage-pRegions-06218
// "For each element of pRegions, imageOffset.x and (imageExtent.width + imageOffset.x) must both be
// greater than or equal to 0 and less than or equal to the width of the specified imageSubresource of dstImage".
#define MAGMA_VIRTUAL_MIP_EXTENT 1

namespace magma
{
Image::Image(std::shared_ptr<Device> device, VkImageType imageType, VkFormat format,
    const VkExtent3D& extent, uint32_t mipLevels, uint32_t arrayLayers, uint32_t samples,
    VkImageCreateFlags flags, VkImageUsageFlags usage, VkImageTiling tiling,
    const Descriptor& optional, const Sharing& sharing, std::shared_ptr<Allocator> allocator):
    NonDispatchableResource(VK_OBJECT_TYPE_IMAGE, device, sharing, allocator),
    flags(flags),
    imageType(imageType),
    format(format),
    layout(VK_IMAGE_LAYOUT_UNDEFINED),
    extent(extent),
    mipLevels(mipLevels),
    arrayLayers(arrayLayers),
    samples(samples),
    tiling(tiling),
    usage(usage)
{
    VkImageCreateInfo imageInfo;
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.flags = flags | optional.flags;
    imageInfo.imageType = imageType;
    imageInfo.format = format;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.samples = getSampleCountBit(samples);
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = sharing.getMode();
    imageInfo.queueFamilyIndexCount = sharing.getQueueFamiliesCount();
    imageInfo.pQueueFamilyIndices = sharing.getQueueFamilyIndices().data();
    imageInfo.initialLayout = layout;
#ifdef VK_KHR_image_format_list
    VkImageFormatListCreateInfoKHR imageFormatListInfo;
    viewFormats = optional.viewFormats;
    if (device->extensionEnabled(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME) && !viewFormats.empty())
    {
        imageInfo.pNext = &imageFormatListInfo;
        imageFormatListInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
        imageFormatListInfo.pNext = nullptr;
        imageFormatListInfo.viewFormatCount = MAGMA_COUNT(viewFormats);
        imageFormatListInfo.pViewFormats = viewFormats.data();
    }
#endif // VK_KHR_image_format_list
    const VkResult result = vkCreateImage(MAGMA_HANDLE(device), &imageInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_HANDLE_RESULT(result, "failed to create image");
    // Allocate image memory
    StructureChain extendedMemoryInfo;
    VkMemoryRequirements memoryRequirements;
#if defined(VK_KHR_get_memory_requirements2) && defined(VK_KHR_dedicated_allocation)
    if (device->extensionEnabled(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME) &&
        device->extensionEnabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
    {
        VkMemoryDedicatedRequirementsKHR dedicatedRequirements = {};
        dedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;
        memoryRequirements = getMemoryRequirements2(&dedicatedRequirements);
        if (dedicatedRequirements.prefersDedicatedAllocation ||
            dedicatedRequirements.requiresDedicatedAllocation)
        {   // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_dedicated_allocation.html
            VkMemoryDedicatedAllocateInfoKHR dedicatedAllocateInfo;
            dedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
            dedicatedAllocateInfo.pNext = nullptr;
            dedicatedAllocateInfo.image = handle;
            dedicatedAllocateInfo.buffer = VK_NULL_HANDLE;
            extendedMemoryInfo.addNode(dedicatedAllocateInfo);
        }
    }
    else
#endif // VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation
    {
        memoryRequirements = getMemoryRequirements();
    }
#ifdef VK_KHR_device_group
    if (device->extensionEnabled(VK_KHR_DEVICE_GROUP_EXTENSION_NAME))
    {
        VkMemoryAllocateFlagsInfoKHR memoryAllocateFlagsInfo;
        memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        memoryAllocateFlagsInfo.pNext = nullptr;
        memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT_KHR;
        memoryAllocateFlagsInfo.deviceMask = optional.deviceMask;
        extendedMemoryInfo.addNode(memoryAllocateFlagsInfo);
    }
#endif // VK_KHR_device_group
#ifdef VK_EXT_memory_priority
    if (device->extensionEnabled(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
    {
        VkMemoryPriorityAllocateInfoEXT memoryPriorityAllocateInfo;
        memoryPriorityAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
        memoryPriorityAllocateInfo.pNext = nullptr;
        memoryPriorityAllocateInfo.priority = optional.memoryPriority;
        extendedMemoryInfo.addNode(memoryPriorityAllocateInfo);
    }
#endif // VK_EXT_memory_priority
    const VkMemoryPropertyFlags memoryFlags = (VK_IMAGE_TILING_LINEAR == tiling)
        ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | (optional.lazy ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : 0);
    std::shared_ptr<IDeviceMemory> memory;
    if (MAGMA_DEVICE_ALLOCATOR(allocator))
    {
        memory = std::make_shared<ManagedDeviceMemory>(device,
            VK_OBJECT_TYPE_IMAGE, handle,
            memoryRequirements, memoryFlags,
            MAGMA_HOST_ALLOCATOR(allocator),
            MAGMA_DEVICE_ALLOCATOR(allocator),
            extendedMemoryInfo);
    }
    else
    {
        memory = std::make_shared<DeviceMemory>(device,
            memoryRequirements, memoryFlags,
            MAGMA_HOST_ALLOCATOR(allocator),
            extendedMemoryInfo);
    }
    bindMemory(std::move(memory));
}

Image::Image(std::shared_ptr<Device> device, VkImage handle_, VkImageType imageType, VkFormat format, const VkExtent3D& extent):
    NonDispatchableResource(VK_OBJECT_TYPE_IMAGE, std::move(device), Sharing(), std::shared_ptr<Allocator>() /* FIX IT */),
    flags(0),
    imageType(imageType),
    format(format),
    layout(VK_IMAGE_LAYOUT_UNDEFINED),
    extent(extent),
    mipLevels(1),
    arrayLayers(1),
    samples(1),
    tiling(VK_IMAGE_TILING_OPTIMAL),
    usage(0)
{
    handle = handle_;
}

Image::~Image()
{
    vkDestroyImage(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

VkExtent3D Image::calculateMipExtent(uint32_t level) const noexcept
{
    MAGMA_ASSERT(level < mipLevels);
    if (0 == level)
        return extent;
    if (level >= mipLevels)
        return VkExtent3D{0, 0, 0};
    VkExtent3D mipExtent = extent;
    mipExtent.width = std::max(1u, extent.width >> level);
    mipExtent.height = std::max(1u, extent.height >> level);
    mipExtent.depth = std::max(1u, extent.depth >> level);
    const Format imageFormat(format);
    if (imageFormat.blockCompressed())
    {   // Extents must be a multiple of the compressed texel block footprint
        const auto blockSize = imageFormat.blockFootprint();
        mipExtent.width = core::roundUp(mipExtent.width, blockSize.first);
        if (imageType > VK_IMAGE_TYPE_1D)
        {
            mipExtent.height = core::roundUp(mipExtent.height, blockSize.second);
            if (imageType > VK_IMAGE_TYPE_2D)
                mipExtent.depth = core::roundUp(mipExtent.depth, blockSize.second); // ?
        }
    }
    return mipExtent;
}

VkSubresourceLayout Image::getSubresourceLayout(uint32_t mipLevel, uint32_t arrayLayer /* 0 */) const noexcept
{
    VkImageSubresource subresource;
    const Format imageFormat(format);
    if (imageFormat.depth())
        subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (imageFormat.stencil())
        subresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    else if (imageFormat.depthStencil())
        subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = mipLevel;
    subresource.arrayLayer = this->arrayLayers > 1 ? arrayLayer : 0; // Ignore for non-arrays
    VkSubresourceLayout subresourceLayout;
    vkGetImageSubresourceLayout(MAGMA_HANDLE(device), handle, &subresource, &subresourceLayout);
    return subresourceLayout;
}

VkImageSubresourceLayers Image::getSubresourceLayers(uint32_t mipLevel, uint32_t arrayLayer /* 0 */) const noexcept
{
    VkImageSubresourceLayers subresourceLayers;
    const Format imageFormat(format);
    if (imageFormat.depth())
        subresourceLayers.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (imageFormat.stencil())
        subresourceLayers.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    else if (imageFormat.depthStencil())
        subresourceLayers.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        subresourceLayers.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceLayers.mipLevel = mipLevel;
    subresourceLayers.baseArrayLayer = this->arrayLayers > 1 ? arrayLayer : 0; // Ignore for non-arrays
    subresourceLayers.layerCount = this->arrayLayers;
    return subresourceLayers;
}

VkMemoryRequirements Image::getMemoryRequirements() const noexcept
{
    VkMemoryRequirements memoryRequirements = {};
    vkGetImageMemoryRequirements(MAGMA_HANDLE(device), handle, &memoryRequirements);
    return memoryRequirements;
}

std::vector<VkSparseImageMemoryRequirements> Image::getSparseMemoryRequirements() const
{
    uint32_t sparseMemoryRequirementCount = 0;
    vkGetImageSparseMemoryRequirements(MAGMA_HANDLE(device), handle, &sparseMemoryRequirementCount, nullptr);
    std::vector<VkSparseImageMemoryRequirements> sparseMemoryRequirements;
    if (sparseMemoryRequirementCount)
    {
        sparseMemoryRequirements.resize(sparseMemoryRequirementCount);
        vkGetImageSparseMemoryRequirements(MAGMA_HANDLE(device), handle, &sparseMemoryRequirementCount, sparseMemoryRequirements.data());
    }
    return sparseMemoryRequirements;
}

#ifdef VK_KHR_get_memory_requirements2
VkMemoryRequirements Image::getMemoryRequirements2(void *memoryRequirements) const
{
    VkImageMemoryRequirementsInfo2KHR imageMemoryRequirementsInfo2;
    imageMemoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
    imageMemoryRequirementsInfo2.pNext = nullptr;
    imageMemoryRequirementsInfo2.image = handle;
    VkMemoryRequirements2KHR memoryRequirements2;
    memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;
    memoryRequirements2.pNext = memoryRequirements;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetImageMemoryRequirements2KHR, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    vkGetImageMemoryRequirements2KHR(MAGMA_HANDLE(device), &imageMemoryRequirementsInfo2, &memoryRequirements2);
    return memoryRequirements2.memoryRequirements;
}

std::vector<VkSparseImageMemoryRequirements2KHR> Image::getSparseMemoryRequirements2(void *memoryRequirements) const
{
    VkImageSparseMemoryRequirementsInfo2KHR imageSparseMemoryRequirementsInfo2;
    imageSparseMemoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2_KHR;
    imageSparseMemoryRequirementsInfo2.pNext = nullptr;
    imageSparseMemoryRequirementsInfo2.image = handle;
    uint32_t sparseMemoryRequirementCount = 0;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetImageSparseMemoryRequirements2KHR, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    vkGetImageSparseMemoryRequirements2KHR(MAGMA_HANDLE(device), &imageSparseMemoryRequirementsInfo2,
        &sparseMemoryRequirementCount, nullptr);
    std::vector<VkSparseImageMemoryRequirements2KHR> sparseMemoryRequirements2;
    if (sparseMemoryRequirementCount)
    {
        for (uint32_t i = 0; i < sparseMemoryRequirementCount; ++i)
        {
            VkSparseImageMemoryRequirements2KHR sparseImageMemoryRequirements2;
            sparseImageMemoryRequirements2.sType = VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2_KHR;
            sparseImageMemoryRequirements2.pNext = memoryRequirements; // TODO: unique requirements per element?
            sparseImageMemoryRequirements2.memoryRequirements = {};
            sparseMemoryRequirements2.push_back(sparseImageMemoryRequirements2);
        }
        vkGetImageSparseMemoryRequirements2KHR(MAGMA_HANDLE(device), &imageSparseMemoryRequirementsInfo2,
            &sparseMemoryRequirementCount, sparseMemoryRequirements2.data());
    }
    return sparseMemoryRequirements2;
}
#endif // VK_KHR_get_memory_requirements2

void Image::bindMemory(std::shared_ptr<IDeviceMemory> memory_,
    VkDeviceSize offset_ /* 0 */)
{
    memory_->bind(handle, VK_OBJECT_TYPE_IMAGE, offset_);
    memory = std::move(memory_);
    offset = offset_;
    size = memory->getSize();
}

#ifdef VK_KHR_device_group
void Image::bindMemoryDeviceGroup(std::shared_ptr<IDeviceMemory> memory_,
    const std::vector<uint32_t>& deviceIndices,
    const std::vector<VkRect2D>& splitInstanceBindRegions /* empty */,
    VkDeviceSize offset_ /* 0 */)
{
    memory_->bindDeviceGroup(handle, VK_OBJECT_TYPE_IMAGE, deviceIndices, splitInstanceBindRegions, offset_);
    memory = std::move(memory_);
    offset = offset_;
    size = memory->getSize();
}
#endif // VK_KHR_device_group

void Image::onDefragment()
{
    VkImageCreateInfo imageInfo;
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;
    imageInfo.flags = flags;
    imageInfo.imageType = imageType;
    imageInfo.format = format;
    imageInfo.extent = extent;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.samples = getSampleCountBit(samples);
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = sharing.getMode();
    imageInfo.queueFamilyIndexCount = sharing.getQueueFamiliesCount();
    imageInfo.pQueueFamilyIndices = sharing.getQueueFamilyIndices().data();
    imageInfo.initialLayout = layout;
    vkDestroyImage(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
    const VkResult result = vkCreateImage(MAGMA_HANDLE(device), &imageInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_HANDLE_RESULT(result, "failed to recreate defragmented image");
    bindMemory(std::move(memory), offset);
}

VkImageLayout Image::layoutTransition(VkImageLayout newLayout, std::shared_ptr<CommandBuffer> cmdBuffer)
{
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = arrayLayers;
    const VkImageLayout oldLayout = layout;
    layout = VK_IMAGE_LAYOUT_UNDEFINED; // Hack to assing 0 to srcAccessMask inside ImageMemoryBarrier
    const ImageMemoryBarrier memoryBarrier(shared_from_this(), newLayout, subresourceRange);
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, memoryBarrier);
    return oldLayout;
}

void Image::copyMip(std::shared_ptr<CommandBuffer> cmdBuffer, uint32_t mipLevel, uint32_t arrayLayer,
    std::shared_ptr<const SrcTransferBuffer> srcBuffer, const CopyLayout& bufferLayout, const VkOffset3D& imageOffset,
    VkImageLayout dstLayout, VkPipelineStageFlags dstStageMask /* VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT */)
{
    VkBufferImageCopy region;
    region.bufferOffset = bufferLayout.offset;
    region.bufferRowLength = bufferLayout.rowLength;
    region.bufferImageHeight = bufferLayout.imageHeight;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = arrayLayer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = imageOffset;
    // Note: Vulkan validation layer expects virtual mip extent for block-compressed formats
    region.imageExtent = virtualMipExtent(mipLevel);
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = mipLevel;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = arrayLayer;
    subresourceRange.layerCount = 1;
    // We couldn't call shared_from_this() from ctor, so use custom ref object w/ empty deleter
    const std::shared_ptr<Image> self = std::shared_ptr<Image>(this, [](Image *) {});
    // Transition image layout as a destination of a transfer command
    if (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == layout)
        layout = VK_IMAGE_LAYOUT_UNDEFINED; // Hack to select proper srcAccessMask inside ImageMemoryBarrier
    const ImageMemoryBarrier transferDst(shared_from_this(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, transferDst);
    cmdBuffer->copyBufferToImage(std::move(srcBuffer), self, region);
    // Transition image layout to read-only access in a shader as a sampled image
    const ImageMemoryBarrier shaderRead(self,
        dstLayout,
        subresourceRange);
    // Insert memory dependency between transfer and fragment shader stages
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, dstStageMask, shaderRead);
}

VkDeviceSize Image::setupMipMaps(std::vector<Mip>& dstMips, const std::vector<MipData>& srcMips) const
{
    VkDeviceSize bufferOffset = 0;
    Mip mip;
    mip.bufferOffset = bufferOffset;
    mip.extent = srcMips[0].extent;
    dstMips.reserve(srcMips.size());
    dstMips.push_back(mip);
    bufferOffset += MAGMA_ALIGN(srcMips[0].size); // By default memory copies are aligned
    for (std::size_t mipIndex = 1, mipCount = srcMips.size(); mipIndex < mipCount; ++mipIndex)
    {
        mip.bufferOffset = bufferOffset;
        mip.extent = srcMips[mipIndex].extent;
        dstMips.push_back(mip);
        bufferOffset += MAGMA_ALIGN(srcMips[mipIndex].size);
    }
    return bufferOffset;
}

void Image::copyMipMaps(std::shared_ptr<CommandBuffer> cmdBuffer, std::shared_ptr<const SrcTransferBuffer> srcBuffer,
    const std::vector<Mip>& mipMaps, const CopyLayout& bufferLayout)
{
    std::vector<VkBufferImageCopy> regions;
    regions.reserve(mipMaps.size());
    uint32_t mipIndex = 0;
    for (const Mip& mip: mipMaps)
    {   // Define buffer -> image copy region
        VkBufferImageCopy region;
        region.bufferOffset = bufferLayout.offset + mip.bufferOffset;
        region.bufferRowLength = bufferLayout.rowLength;
        region.bufferImageHeight = bufferLayout.imageHeight;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = mipIndex % mipLevels;
        region.imageSubresource.baseArrayLayer = mipIndex / mipLevels;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        // Note: Vulkan validation layer expects virtual mip extent for block-compressed formats
        region.imageExtent = virtualMipExtent(region.imageSubresource.mipLevel);
        regions.push_back(region);
        ++mipIndex;
    }
    // Define resource range
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = arrayLayers;
    // We couldn't call shared_from_this() from ctor, so use custom ref object w/ empty deleter
    const std::shared_ptr<Image> self = std::shared_ptr<Image>(this, [](Image *) {});
    // Transition image layout as a destination of a transfer command
    const ImageMemoryBarrier transferDst(self,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);
    // Insert memory dependency between host and transfer stages
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, transferDst);
    cmdBuffer->copyBufferToImage(srcBuffer, self, regions);
    // Transition image layout to read-only access in a shader as a sampled image
    const ImageMemoryBarrier shaderRead(self,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        subresourceRange);
    // Insert memory dependency between transfer and fragment shader stages
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, shaderRead);
}

VkExtent3D Image::virtualMipExtent(uint32_t level) const noexcept
{
    MAGMA_ASSERT(level < mipLevels);
    if (!level)
        return extent;
#if MAGMA_VIRTUAL_MIP_EXTENT
    return VkExtent3D{
        std::max(1u, extent.width >> level),
        std::max(1u, extent.height >> level),
        std::max(1u, extent.depth >> level)
    };
#else
    return calculateMipExtent(level); // Real extent
#endif // MAGMA_VIRTUAL_MIP_EXTENT
}

VkSampleCountFlagBits Image::getSampleCountBit(uint32_t samples) noexcept
{
    switch (samples)
    {
    case 1: return VK_SAMPLE_COUNT_1_BIT;
    case 2: return VK_SAMPLE_COUNT_2_BIT;
    case 4: return VK_SAMPLE_COUNT_4_BIT;
    case 8: return VK_SAMPLE_COUNT_8_BIT;
    case 16: return VK_SAMPLE_COUNT_16_BIT;
    case 32: return VK_SAMPLE_COUNT_32_BIT;
    case 64: return VK_SAMPLE_COUNT_64_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

VkFormat Image::checkFormatFeature(std::shared_ptr<Device> device, VkFormat format, VkFormatFeatureFlags requiredFeature)
{
    std::shared_ptr<PhysicalDevice> physicalDevice = device->getPhysicalDevice();
    const VkFormatProperties properties = physicalDevice->getFormatProperties(format);
    const bool hasFeature = (properties.optimalTilingFeatures & requiredFeature);
    if (!hasFeature)
        MAGMA_ERROR("format doesn't suport required feature");
    return format;
}
} // namespace magma
