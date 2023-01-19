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
#include <iostream>
#include <sstream>
#include "descriptorSet.h"
#include "descriptorSetLayout.h"
#include "descriptorPool.h"
#include "device.h"
#include "../descriptors/descriptorSetLayoutReflection.h"
#include "../shaders/shaderReflection.h"
#include "../shaders/shaderReflectionFactory.h"
#include "../helpers/spirvReflectionTypeCast.h"
#include "../helpers/stringize.h"
#include "../helpers/stackArray.h"
#include "../exceptions/errorResult.h"

namespace magma
{
DescriptorSet::DescriptorSet(std::shared_ptr<DescriptorPool> descriptorPool,
    DescriptorSetLayoutReflection& layoutReflection, uint32_t stageFlags,
    std::shared_ptr<IAllocator> allocator /* nullptr */,
    std::shared_ptr<IShaderReflectionFactory> shaderReflectionFactory /* nullptr */,
    const std::string& shaderFileName /* default */,
    uint32_t setIndex /* 0 */,
    const StructureChain& extendedInfo /* default */):
    NonDispatchable(VK_OBJECT_TYPE_DESCRIPTOR_SET, descriptorPool->getDevice(), std::move(allocator)),
    layoutReflection(layoutReflection),
    descriptorPool(std::move(descriptorPool))
{   // Check that all descriptors have unique layout bindings
    const std::vector<descriptor::Descriptor*>& descriptors = layoutReflection.getBindingDescriptors();
    std::vector<uint32_t> locations;
    for (const auto descriptor: descriptors)
        locations.push_back(descriptor->getLayoutBinding().binding);
    if (std::unique(locations.begin(), locations.end()) != locations.end())
        MAGMA_THROW("elements of descriptor set layout should have unique binding locations");
    // Validate descriptors through shader reflection
    if (shaderReflectionFactory && !shaderFileName.empty())
        validateReflection(shaderReflectionFactory->getReflection(shaderFileName), setIndex);
    // Prepare list of native bindings
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    for (const auto descriptor: descriptors)
    {
        layoutBindings.push_back(descriptor->getLayoutBinding());
        // Set global stage flags if they have not been assigned for descriptor binding
        if (!layoutBindings.back().stageFlags)
            layoutBindings.back().stageFlags = stageFlags;
    }
    // Create descriptor set layout
    setLayout = std::make_shared<DescriptorSetLayout>(device, layoutBindings, hostAllocator, 0);
    // Allocate descriptor set
    const VkDescriptorSetLayout dereferencedSetLayouts[1] = {*setLayout};
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = extendedInfo.getChainedNodes();
    descriptorSetAllocateInfo.descriptorPool = MAGMA_HANDLE(descriptorPool);
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = dereferencedSetLayouts;
    const VkResult result = vkAllocateDescriptorSets(MAGMA_HANDLE(device), &descriptorSetAllocateInfo, &handle);
    MAGMA_THROW_FAILURE(result, "failed to allocate descriptor set");
}

DescriptorSet::~DescriptorSet()
{
    if (descriptorPool->canFreeDescriptorSet())
        vkFreeDescriptorSets(MAGMA_HANDLE(device), *descriptorPool, 1, &handle);
}

bool DescriptorSet::dirty() const
{
    return layoutReflection.dirty();
}

void DescriptorSet::update()
{
    MAGMA_ASSERT(dirty());
    uint32_t descriptorWriteCount = 0;
    const std::vector<descriptor::Descriptor*>& descriptors = layoutReflection.getBindingDescriptors();
    for (const auto descriptor: descriptors)
    {
        if (descriptor->dirty())
            ++descriptorWriteCount;
    }
    if (descriptorWriteCount > 0)
    {
        MAGMA_STACK_ARRAY(VkWriteDescriptorSet, descriptorWrites, descriptorWriteCount);
        for (const auto descriptor: descriptors)
        {
            if (descriptor->dirty())
                descriptorWrites.put(descriptor->getWriteDescriptorSet(handle));
        }
        device->updateDescriptorWrites(descriptorWrites, descriptorWriteCount);
    }
}

void DescriptorSet::gatherDirtyDescriptorWrites(std::vector<VkWriteDescriptorSet>& descriptorWrites) const
{
    MAGMA_ASSERT(dirty());
    const std::vector<descriptor::Descriptor*>& descriptors = layoutReflection.getBindingDescriptors();
    for (const auto descriptor: descriptors)
    {
        if (descriptor->dirty())
            descriptorWrites.push_back(descriptor->getWriteDescriptorSet(handle));
    }
}

void DescriptorSet::validateReflection(std::shared_ptr<const ShaderReflection> shaderReflection, uint32_t setIndex) const
{
    std::vector<const SpvReflectDescriptorSet *> descriptorSets = shaderReflection->enumerateDescriptorSets();
    if (setIndex >= descriptorSets.size())
        MAGMA_THROW("set index exceeds number of reflected descriptor sets");
    const SpvReflectDescriptorSet *descriptorSet = descriptorSets[setIndex];
    const std::vector<descriptor::Descriptor *>& descriptors = layoutReflection.getBindingDescriptors();
    for (const auto descriptor: descriptors)
    {
        const VkDescriptorSetLayoutBinding& binding = descriptor->getLayoutBinding();
        const SpvReflectDescriptorBinding *reflectedBinding = nullptr;
        for (uint32_t i = 0; i < descriptorSet->binding_count; ++i)
        {
            if (binding.binding == descriptorSet->bindings[i]->binding)
            {
                reflectedBinding = descriptorSet->bindings[i];
                break;
            }
        }
        if (!reflectedBinding)
        {
            std::cout << "warning: binding #" << binding.binding << " not found in the reflection" << std::endl;
            continue;
        }
        const VkDescriptorType reflectedDescriptorType = helpers::spirvToDescriptorType(reflectedBinding->descriptor_type);
        if (binding.descriptorType != reflectedDescriptorType)
        {
            if ((VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC == binding.descriptorType) &&
                (SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER != reflectedBinding->descriptor_type))
            {
                std::ostringstream oss;
                oss << "descriptor type mismatch:" << std::endl
                    << "binding #" << binding.binding << std::endl
                    << "expected: " << helpers::stringize(reflectedDescriptorType) << std::endl
                    << "defined: " << helpers::stringize(binding.descriptorType);
                MAGMA_THROW(oss.str());
            }
        }
        if (binding.descriptorCount != reflectedBinding->count)
        {
            std::ostringstream oss;
            oss << "descriptor count mismatch:" << std::endl
                << "binding #" << binding.binding << std::endl
                << "expected: " << reflectedBinding->count << ", defined: " << binding.descriptorCount;
            MAGMA_THROW(oss.str());
        }
    }
}
} // namespace magma
