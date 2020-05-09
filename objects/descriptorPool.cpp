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
#include "descriptorPool.h"
#include "descriptorSet.h"
#include "descriptorSetLayout.h"
#include "device.h"
#include "../allocator/allocator.h"
#include "../helpers/stackArray.h"
#include "../misc/exception.h"

namespace magma
{
DescriptorPool::DescriptorPool(std::shared_ptr<Device> device, uint32_t maxSets, const Descriptor& descriptor,
    bool freeDescriptorSet /* false */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_DESCRIPTOR_POOL, std::move(device), std::move(allocator))
{
    VkDescriptorPoolCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    if (freeDescriptorSet)
        info.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = maxSets;
    info.poolSizeCount = 1;
    info.pPoolSizes = &descriptor;
    MAGMA_PROFILE_ENTRY(vkCreateDescriptorPool);
    const VkResult create = vkCreateDescriptorPool(MAGMA_HANDLE(device), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create descriptor pool");
}

DescriptorPool::DescriptorPool(std::shared_ptr<Device> device, uint32_t maxSets, const std::vector<Descriptor>& descriptors,
    bool freeDescriptorSet /* false */,
    std::shared_ptr<IAllocator> allocator /* nullptr */):
    NonDispatchable(VK_OBJECT_TYPE_DESCRIPTOR_POOL, std::move(device), std::move(allocator))
{
    VkDescriptorPoolCreateInfo info;
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = 0;
    if (freeDescriptorSet)
        info.flags |= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    info.maxSets = maxSets;
    info.poolSizeCount = MAGMA_COUNT(descriptors);
    info.pPoolSizes = descriptors.data();
    MAGMA_PROFILE_ENTRY(vkCreateDescriptorPool);
    const VkResult create = vkCreateDescriptorPool(MAGMA_HANDLE(device), &info, MAGMA_OPTIONAL_INSTANCE(allocator), &handle);
    MAGMA_THROW_FAILURE(create, "failed to create descriptor pool");
}

DescriptorPool::~DescriptorPool()
{
    MAGMA_PROFILE_ENTRY(vkDestroyDescriptorPool);
    vkDestroyDescriptorPool(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(allocator));
}

void DescriptorPool::reset()
{
    MAGMA_PROFILE_ENTRY(vkResetDescriptorPool);
    const VkResult reset = vkResetDescriptorPool(MAGMA_HANDLE(device), handle, 0);
    MAGMA_THROW_FAILURE(reset, "failed to reset descriptor pool");
}

std::shared_ptr<DescriptorSet> DescriptorPool::allocateDescriptorSet(std::shared_ptr<DescriptorSetLayout> setLayout)
{
    VkDescriptorSetAllocateInfo info;
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.descriptorPool = handle;
    info.descriptorSetCount = 1;
    const VkDescriptorSetLayout dereferencedLayouts[1] = {*setLayout};
    info.pSetLayouts = dereferencedLayouts;
    VkDescriptorSet descriptorSet;
    {
        MAGMA_PROFILE_ENTRY(vkAllocateDescriptorSets);
        const VkResult alloc = vkAllocateDescriptorSets(MAGMA_HANDLE(device), &info, &descriptorSet);
        MAGMA_THROW_FAILURE(alloc, "failed to allocate descriptor set");
    }
    return std::shared_ptr<DescriptorSet>(new DescriptorSet(descriptorSet,  device, shared_from_this(), setLayout));
}

void DescriptorPool::freeDescriptorSet(std::shared_ptr<DescriptorSet>& descriptorSet) noexcept
{
    const VkDescriptorSet dereferencedLayouts[1] = {*descriptorSet};
    MAGMA_PROFILE_ENTRY(vkFreeDescriptorSets);
    vkFreeDescriptorSets(MAGMA_HANDLE(device), handle, 1, dereferencedLayouts);
    descriptorSet.reset();
}

std::vector<std::shared_ptr<DescriptorSet>> DescriptorPool::allocateDescriptorSets(const std::vector<std::shared_ptr<DescriptorSetLayout>>& setLayouts)
{
    VkDescriptorSetAllocateInfo info;
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.descriptorPool = handle;
    info.descriptorSetCount = MAGMA_COUNT(setLayouts);
    MAGMA_STACK_ARRAY(VkDescriptorSetLayout, dereferencedLayouts, setLayouts.size());
    for (const auto& layout : setLayouts)
        dereferencedLayouts.put(*layout);
    info.pSetLayouts = dereferencedLayouts;
    MAGMA_STACK_ARRAY(VkDescriptorSet, nativeDescriptorSets, info.descriptorSetCount);
    {
        MAGMA_PROFILE_ENTRY(vkAllocateDescriptorSets);
        const VkResult alloc = vkAllocateDescriptorSets(MAGMA_HANDLE(device), &info, nativeDescriptorSets);
        MAGMA_THROW_FAILURE(alloc, "failed to allocate descriptor sets");
    }
    std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
    int i = 0;
    for (const VkDescriptorSet descriptorSet : nativeDescriptorSets)
        descriptorSets.emplace_back(new DescriptorSet(descriptorSet, device, shared_from_this(), setLayouts[i++]));
    return descriptorSets;
}

void DescriptorPool::freeDescriptorSets(std::vector<std::shared_ptr<DescriptorSet>>& descriptorSets) noexcept
{
    MAGMA_STACK_ARRAY(VkDescriptorSet, dereferencedDescriptorSets, descriptorSets.size());
    for (const auto& descriptorSet : descriptorSets)
        dereferencedDescriptorSets.put(*descriptorSet);
    MAGMA_PROFILE_ENTRY(vkFreeDescriptorSets);
    vkFreeDescriptorSets(MAGMA_HANDLE(device), handle, dereferencedDescriptorSets.size(), dereferencedDescriptorSets);
    descriptorSets.clear();
}
} // namespace magma
