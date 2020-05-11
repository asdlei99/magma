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
#include "pipelineLayout.h"
#include "device.h"
#include "descriptorSetLayout.h"
#include "../allocator/allocator.h"
#include "../helpers/stackArray.h"
#include "../misc/exception.h"

namespace magma
{
PipelineLayout::PipelineLayout(std::shared_ptr<Device> device, const PushConstantRange& pushConstantRange,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    PipelineLayout(std::move(device), {pushConstantRange}, std::move(allocator))
{}

PipelineLayout::PipelineLayout(std::shared_ptr<Device> device,
    const std::initializer_list<PushConstantRange>& pushConstantRanges /* {} */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_PIPELINE_LAYOUT, std::move(device), std::move(allocator))
{
    VkPipelineLayoutCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = MAGMA_COUNT(pushConstantRanges);
    info.pPushConstantRanges = pushConstantRanges.begin();
    const VkResult create = vkCreatePipelineLayout(MAGMA_HANDLE(device), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create pipeline layout");
    MAGMA_REGISTER_RESOURCE(PipelineLayout, this);
    hash = core::hashArgs(
        info.sType,
        info.flags,
        info.setLayoutCount,
        info.pushConstantRangeCount);
    for (const auto& pushConstantRange : pushConstantRanges)
        core::hashCombine(hash, pushConstantRange.hash());
}

PipelineLayout::PipelineLayout(std::shared_ptr<DescriptorSetLayout> setLayout, const PushConstantRange& pushConstantRange,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    PipelineLayout(std::move(setLayout), {pushConstantRange}, std::move(allocator))
{}

PipelineLayout::PipelineLayout(std::shared_ptr<DescriptorSetLayout> setLayout,
    const std::initializer_list<PushConstantRange>& pushConstantRanges /* {} */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    PipelineLayout(std::vector<std::shared_ptr<DescriptorSetLayout>>{setLayout}, std::move(pushConstantRanges), std::move(allocator))
{}

PipelineLayout::PipelineLayout(const std::vector<std::shared_ptr<DescriptorSetLayout>>& setLayouts,
    const std::initializer_list<PushConstantRange>& pushConstantRanges /* {} */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_PIPELINE_LAYOUT, std::move(setLayouts[0]->getDevice()), std::move(allocator)),
    setLayouts(setLayouts)
{
    VkPipelineLayoutCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    MAGMA_STACK_ARRAY(VkDescriptorSetLayout, dereferencedSetLayouts, setLayouts.size());
    for (const auto& layout : setLayouts)
        dereferencedSetLayouts.put(*layout);
    info.setLayoutCount = MAGMA_COUNT(dereferencedSetLayouts);
    info.pSetLayouts = dereferencedSetLayouts;
    info.pushConstantRangeCount = MAGMA_COUNT(pushConstantRanges);
    info.pPushConstantRanges = pushConstantRanges.begin();
    const VkResult create = vkCreatePipelineLayout(MAGMA_HANDLE(device), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create pipeline layout");
    MAGMA_REGISTER_RESOURCE(PipelineLayout, this);
    hash = core::hashArgs(
        info.sType,
        info.flags,
        info.setLayoutCount,
        info.pushConstantRangeCount);
    for (const auto& pushConstantRange : pushConstantRanges)
        core::hashCombine(hash, pushConstantRange.hash());
}

PipelineLayout::~PipelineLayout()
{
    MAGMA_UNREGISTER_RESOURCE(PipelineLayout, this);
    vkDestroyPipelineLayout(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(allocator));
}

std::size_t PipelineLayout::getHash() const noexcept
{   // Compute complex hash on demand
    std::size_t hash = this->hash;
    for (const std::shared_ptr<DescriptorSetLayout>& layout : setLayouts)
        core::hashCombine(hash, layout->getHash());
    return hash;
}
} // namespace magma
