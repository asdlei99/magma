/*
Magma - C++1x interface over Khronos Vulkan API.
Copyright (C) 2018 Victor Coda.

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
#include <unordered_set>
#include "managedDescriptorSet.h"
#include "../objects/device.h"
#include "../objects/buffer.h"
#include "../objects/bufferView.h"
#include "../objects/uniformBuffer.h"
#include "../objects/image.h"
#include "../objects/imageView.h"
#include "../objects/sampler.h"
#include "../objects/descriptorPool.h"
#include "../objects/descriptorSetLayout.h"
#include "../objects/descriptorSet.h"
#include "../helpers/stackArray.h"
#include "../misc/exception.h"

namespace magma
{
namespace aux
{
void ManagedDescriptorSet::ShaderStageBindings::bindImageView(uint32_t binding, std::shared_ptr<const ImageView> imageView, std::shared_ptr<const Sampler> sampler)
{
    VkDescriptorImageInfo imageInfo;
    imageInfo.sampler = *sampler;
    imageInfo.imageView = *imageView;
    imageInfo.imageLayout = imageView->getImage()->getLayout();
    imageInfos.push_back(imageInfo);
    VkWriteDescriptorSet writeDescriptor;
    writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptor.pNext = nullptr;
    writeDescriptor.dstSet = VK_NULL_HANDLE;
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptor.pImageInfo = &imageInfos.back();
    writeDescriptor.pBufferInfo = nullptr;
    writeDescriptor.pTexelBufferView = nullptr;
    descriptorWrites[binding] = writeDescriptor;
    updated = true;
}

void ManagedDescriptorSet::ShaderStageBindings::bindBuffer(uint32_t binding, std::shared_ptr<const Buffer> buffer,
    VkDeviceSize offset /* 0 */, VkDeviceSize range /* VK_WHOLE_SIZE */)
{
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = *buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;
    bufferInfos.push_back(bufferInfo);
    VkWriteDescriptorSet writeDescriptor;
    writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptor.pNext = nullptr;
    writeDescriptor.dstSet = VK_NULL_HANDLE;
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    bool isDynamic = false;
    auto bufferTrait = std::dynamic_pointer_cast<const BufferDynamicTrait>(buffer);
    if (bufferTrait)
        isDynamic = bufferTrait->isDynamic();
    switch (buffer->getUsage())
    {
    case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT:
        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        break;
    case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT:
        writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        break;
    case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT:
        writeDescriptor.descriptorType = isDynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                                   : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
    case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT:
        writeDescriptor.descriptorType = isDynamic ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
                                                   : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
    default:
        MAGMA_THROW("unknown buffer usage");
    }
    writeDescriptor.pImageInfo = nullptr;
    writeDescriptor.pBufferInfo = &bufferInfos.back();
    writeDescriptor.pTexelBufferView = nullptr;
    descriptorWrites[binding] = writeDescriptor;
    updated = true;
}

void ManagedDescriptorSet::ShaderStageBindings::bindTexelBufferView(uint32_t binding, std::shared_ptr<const BufferView> texelBufferView)
{
    bufferViews.push_back(*texelBufferView);
    VkWriteDescriptorSet writeDescriptor;
    writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptor.pNext = nullptr;
    writeDescriptor.dstSet = VK_NULL_HANDLE;
    writeDescriptor.dstBinding = binding;
    writeDescriptor.dstArrayElement = 0;
    writeDescriptor.descriptorCount = 1;
    writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    writeDescriptor.pImageInfo = nullptr;
    writeDescriptor.pBufferInfo = nullptr;
    writeDescriptor.pTexelBufferView = &bufferViews.back();
    descriptorWrites[binding] = writeDescriptor;
    updated = true;
}

ManagedDescriptorSet::ManagedDescriptorSet(std::shared_ptr<Device> device,
    std::shared_ptr<DescriptorPool> pool /* nullptr */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    device(device),
    pool(pool),
    allocator(allocator)
{}

void ManagedDescriptorSet::finalize()
{
    uint32_t numDescriptorWrites = 0;
    if (!pool)
    {
        std::unordered_map<VkDescriptorType, uint32_t> descriptorCounts;
        for (const auto& stage : stages)
        {   // Count number of descriptors of corresponding type
            for (const auto& write : stage.second.descriptorWrites)
            {
                const VkWriteDescriptorSet& writeDescriptor = write.second;
                descriptorCounts[writeDescriptor.descriptorType] += writeDescriptor.descriptorCount;
                ++numDescriptorWrites;
            }
        }
        // If pool not provided, create our own
        std::vector<magma::Descriptor> descriptors;
        for (const auto& counts : descriptorCounts)
        {
            const magma::Descriptor descriptor(counts.first /* type */, counts.second /* count */);
            descriptors.push_back(descriptor);
        }
        pool = std::make_shared<magma::DescriptorPool>(device, 1, descriptors, false, allocator);
    }
    // Prepare layout bindings
    std::vector<DescriptorSetLayout::Binding> bindings;
    std::unordered_set<uint32_t> uniqueBindings;
    bindings.reserve(numDescriptorWrites);
    for (const auto& stage : stages)
    {
        for (const auto& write : stage.second.descriptorWrites)
        {
            const VkWriteDescriptorSet& writeDescriptor = write.second;
            if (uniqueBindings.find(writeDescriptor.dstBinding) != uniqueBindings.end())
                MAGMA_THROW("descriptor binding should be unique across pipeline");
            uniqueBindings.insert(writeDescriptor.dstBinding);
            const Descriptor descriptor(writeDescriptor.descriptorType, writeDescriptor.descriptorCount);
            const VkShaderStageFlagBits stageFlags = stage.first; // TODO: combine flags from different stages?
            bindings.emplace_back(writeDescriptor.dstBinding, descriptor, stageFlags);
        }
    }
    // Create set layout
    layout = std::make_shared<DescriptorSetLayout>(device, bindings, 0, allocator);
    // Allocate descriptor set
    set = pool->allocateDescriptorSet(layout);
    // Create linear array of descriptor writes to pass to API
    MAGMA_STACK_ARRAY(VkWriteDescriptorSet, descriptorWrites, numDescriptorWrites);
    for (auto& stage : stages)
    {
        for (const auto& write : stage.second.descriptorWrites)
        {
            descriptorWrites.put(write.second);
            descriptorWrites.last().dstSet = MAGMA_HANDLE(set); // Assing newly created handle
        }
    }
    // Write all descriptors
    vkUpdateDescriptorSets(MAGMA_HANDLE(device), MAGMA_COUNT(descriptorWrites), descriptorWrites,
        0, nullptr);
    for (auto& stage : stages)
        stage.second.updated = false;
}

std::shared_ptr<DescriptorSetLayout> ManagedDescriptorSet::getLayout()
{
    return layout;
}

std::shared_ptr<DescriptorSet> ManagedDescriptorSet::getDescriptorSet()
{
    return set;
}
} // namespace aux
} // namespace magma
