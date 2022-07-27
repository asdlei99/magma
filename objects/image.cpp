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
#include "buffer.h"
#include "device.h"
#include "deviceMemory.h"
#include "managedDeviceMemory.h"
#include "queue.h"
#include "fence.h"
#include "commandBuffer.h"
#include "../barriers/imageMemoryBarrier.h"
#include "../misc/extProcAddress.h"
#include "../misc/format.h"
#include "../exceptions/errorResult.h"

namespace magma
{
Image::Image(std::shared_ptr<Device> device, VkImageType imageType, VkFormat format,
    const VkExtent3D& extent, uint32_t mipLevels, uint32_t arrayLayers, uint32_t samples,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    const Sharing& sharing, std::shared_ptr<Allocator> allocator):
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
    const VkResult result = vkCreateImage(MAGMA_HANDLE(device), &imageInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create image");
    const VkMemoryRequirements memoryRequirements = getMemoryRequirements();
    const VkMemoryPropertyFlags memoryFlags = (VK_IMAGE_TILING_LINEAR == tiling)
        ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    std::shared_ptr<DeviceMemory> memory;
    if (MAGMA_DEVICE_ALLOCATOR(allocator))
    {
        memory = std::make_shared<ManagedDeviceMemory>(
            std::move(device),
            memoryRequirements, memoryFlags, MAGMA_MEMORY_PRIORITY,
            &handle, VK_OBJECT_TYPE_IMAGE,
            std::move(allocator));
    } else
    {
        memory = std::make_shared<DeviceMemory>(
            std::move(device),
            memoryRequirements, memoryFlags, MAGMA_MEMORY_PRIORITY,
            MAGMA_HOST_ALLOCATOR(allocator));
    }
    bindMemory(std::move(memory));
}

Image::Image(std::shared_ptr<Device> device, VkImage handle, VkImageType imageType, VkFormat format, const VkExtent3D& extent):
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
    this->handle = handle;
}

Image::~Image()
{
    vkDestroyImage(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

VkExtent3D Image::getMipExtent(uint32_t level) const noexcept
{
    MAGMA_ASSERT(level < mipLevels);
    if (0 == level)
        return extent;
    if (level >= mipLevels)
        return VkExtent3D{0, 0, 0};
    VkExtent3D mipExtent = extent;
    for (uint32_t i = 0; i < level; ++i)
    {
        if (mipExtent.width > 1)
            mipExtent.width >>= 1;
        if (mipExtent.height > 1)
            mipExtent.height >>= 1;
        if (mipExtent.depth > 1)
            mipExtent.depth >>= 1;
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
    uint32_t sparseMemoryRequirementCount;
    vkGetImageSparseMemoryRequirements(MAGMA_HANDLE(device), handle, &sparseMemoryRequirementCount, nullptr);
    std::vector<VkSparseImageMemoryRequirements> sparseMemoryRequirements(sparseMemoryRequirementCount);
    if (sparseMemoryRequirementCount > 0)
        vkGetImageSparseMemoryRequirements(MAGMA_HANDLE(device), handle, &sparseMemoryRequirementCount, sparseMemoryRequirements.data());
    return sparseMemoryRequirements;
}

void Image::bindMemory(std::shared_ptr<DeviceMemory> memory,
    VkDeviceSize offset /* 0 */)
{
    memory->bind(&handle, VK_OBJECT_TYPE_IMAGE, offset);
    this->size = memory->getSize();
    this->offset = offset;
    this->memory = std::move(memory);
}

#ifdef VK_KHR_device_group
void Image::bindMemoryDeviceGroup(std::shared_ptr<DeviceMemory> memory,
    const std::vector<uint32_t>& deviceIndices,
    VkDeviceSize offset /* 0 */)
{
    VkBindImageMemoryDeviceGroupInfoKHR deviceGroupBindInfo;
    deviceGroupBindInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO_KHR;
    deviceGroupBindInfo.pNext = nullptr;
    deviceGroupBindInfo.deviceIndexCount = MAGMA_COUNT(deviceIndices);
    deviceGroupBindInfo.pDeviceIndices = deviceIndices.data();
    deviceGroupBindInfo.splitInstanceBindRegionCount = 0;
    deviceGroupBindInfo.pSplitInstanceBindRegions = nullptr;
    VkBindImageMemoryInfoKHR bindInfo;
    bindInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR;
    bindInfo.pNext = &deviceGroupBindInfo;
    bindInfo.image = handle;
    bindInfo.memory = *memory;
    bindInfo.memoryOffset = memory->getOffset() + offset;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkBindImageMemory2KHR, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
    const VkResult result = vkBindImageMemory2KHR(MAGMA_HANDLE(device), 1, &bindInfo);
    MAGMA_THROW_FAILURE(result, "failed to bind image memory within device group");
    this->size = memory->getSize();
    this->offset = offset;
    this->memory = std::move(memory);
}

void Image::bindMemoryDeviceGroup(std::shared_ptr<DeviceMemory> memory,
    const std::vector<uint32_t>& deviceIndices,
    const std::vector<VkRect2D>& splitInstanceBindRegions,
    VkDeviceSize offset /* 0 */)
{
    VkBindImageMemoryDeviceGroupInfoKHR deviceGroupBindInfo;
    deviceGroupBindInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO_KHR;
    deviceGroupBindInfo.pNext = nullptr;
    deviceGroupBindInfo.deviceIndexCount = MAGMA_COUNT(deviceIndices);
    deviceGroupBindInfo.pDeviceIndices = deviceIndices.data();
    deviceGroupBindInfo.splitInstanceBindRegionCount = MAGMA_COUNT(splitInstanceBindRegions);
    deviceGroupBindInfo.pSplitInstanceBindRegions = splitInstanceBindRegions.data();
    VkBindImageMemoryInfoKHR bindInfo;
    bindInfo.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHR;
    bindInfo.pNext = &deviceGroupBindInfo;
    bindInfo.image = handle;
    bindInfo.memory = *memory;
    bindInfo.memoryOffset = memory->getOffset() + offset;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkBindImageMemory2KHR, VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
    const VkResult result = vkBindImageMemory2KHR(MAGMA_HANDLE(device), 1, &bindInfo);
    MAGMA_THROW_FAILURE(result, "failed to bind image memory within device group");
    this->size = memory->getSize();
    this->offset = offset;
    this->memory = std::move(memory);
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
    MAGMA_THROW_FAILURE(result, "failed to recreate defragmented image");
    bindMemory(std::move(memory), offset);
}

void Image::copyMipLevel(std::shared_ptr<CommandBuffer> cmdBuffer, uint32_t level,
    std::shared_ptr<const Buffer> buffer, const CopyLayout& bufferLayout, const VkOffset3D& imageOffset,
    VkImageLayout dstLayout, VkPipelineStageFlags barrierDstStageMask /* VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT */)
{
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = level;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;
    // We couldn't call shared_from_this() from ctor, so use custom ref object w/ empty deleter
    const auto weakThis = std::shared_ptr<Image>(this, [](Image *) {});
    // Image layout transition to transfer dest optimal
    const ImageMemoryBarrier preCopyBarrier(weakThis, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, preCopyBarrier);
    // Copy image
    VkBufferImageCopy region;
    region.bufferOffset = bufferLayout.offset;
    region.bufferRowLength = bufferLayout.rowLength;
    region.bufferImageHeight = bufferLayout.imageHeight;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = level;
    region.imageSubresource.baseArrayLayer = subresourceRange.baseArrayLayer;
    region.imageSubresource.layerCount = subresourceRange.layerCount;
    region.imageOffset = imageOffset;
    region.imageExtent = getMipExtent(level);
    cmdBuffer->copyBufferToImage(std::move(buffer), weakThis, region);
    // Image layout transition from transfer dest to specified layout
    const ImageMemoryBarrier postCopyBarrier(weakThis, dstLayout, subresourceRange);
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, barrierDstStageMask, postCopyBarrier);
}

Image::MipmapLayout Image::setupMipOffsets(const MipmapLayout& mipSizes, VkDeviceSize& bufferSize) const noexcept
{
    const uint32_t mipCount = MAGMA_COUNT(mipSizes);
    MAGMA_ASSERT(mipCount > 0);
    MAGMA_ASSERT(mipCount <= mipLevels * arrayLayers);
    MipmapLayout mipOffsets;
    mipOffsets.reserve(mipLevels * arrayLayers);
    mipOffsets.push_back(0);
    for (uint32_t layer = 0; layer < arrayLayers; ++layer)
    {   // Do not compute last mip offset for last array layer
        const uint32_t count = (layer < arrayLayers - 1) ? mipCount : mipCount - 1;
        for (uint32_t level = 1; level <= count; ++level)
        {   // By default memory copies are aligned
            const VkDeviceSize alignedMipSize = MAGMA_ALIGN(mipSizes[level - 1]);
            mipOffsets.push_back(alignedMipSize);
        }
    }
    // Compute buffer size
    bufferSize = 0;
    for (uint32_t level = 0; level < mipCount; ++level)
        bufferSize += MAGMA_ALIGN(mipSizes[level]);
    bufferSize *= arrayLayers;
    return mipOffsets;
}

std::vector<VkBufferImageCopy> Image::setupCopyRegions(const MipmapLayout& mipOffsets, const CopyLayout& bufferLayout) const noexcept
{
    const uint32_t mipCount = MAGMA_COUNT(mipOffsets);
    MAGMA_ASSERT(mipCount > 0);
    MAGMA_ASSERT(mipCount <= mipLevels * arrayLayers);
    std::vector<VkBufferImageCopy> copyRegions;
    copyRegions.reserve(mipCount);
    VkDeviceSize bufferOffset = bufferLayout.offset;
    for (uint32_t i = 0; i < mipCount; ++i)
    {
        bufferOffset += mipOffsets[i];
        VkBufferImageCopy region;
        region.bufferOffset = bufferOffset;
        region.bufferRowLength = bufferLayout.rowLength;
        region.bufferImageHeight = bufferLayout.imageHeight;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = i % mipLevels;
        region.imageSubresource.baseArrayLayer = i / mipLevels;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = getMipExtent(region.imageSubresource.mipLevel);
        copyRegions.push_back(region);
    }
    return copyRegions;
}

void Image::copyTransfer(std::shared_ptr<CommandBuffer> cmdBuffer, std::shared_ptr<const Buffer> srcBuffer,
    const std::vector<VkBufferImageCopy>& copyRegions)
{   // Define array layers to copy
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = arrayLayers;
    // We couldn't call shared_from_this() from ctor, so use custom ref object w/ empty deleter
    const auto weakThis = std::shared_ptr<Image>(this, [](Image *) {});
    // Change image layout to transfer dest optimal
    const ImageMemoryBarrier preCopyBarrier(weakThis, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, preCopyBarrier);
    // Copy image data
    cmdBuffer->copyBufferToImage(std::move(srcBuffer), weakThis, copyRegions);
    // Change image layout from transfer dest optimal to shader read only
    const ImageMemoryBarrier postCopyBarrier(weakThis, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
    cmdBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, postCopyBarrier);
}

VkSampleCountFlagBits Image::getSampleCountBit(uint32_t samples)
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
    default:
        MAGMA_THROW("invalid <samples> parameter");
    }
}
} // namespace magma
