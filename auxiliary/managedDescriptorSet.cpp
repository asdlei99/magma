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
#include "managedDescriptorSet.h"

namespace magma
{
namespace aux
{
ManagedDescriptorSet::ManagedDescriptorSet()
{
    // TODO:
}

void ManagedDescriptorSet::bindBuffer(uint32_t binding, std::shared_ptr<const Buffer> buffer, VkShaderStageFlagBits shaderStages)
{
    updated = true;
}

void ManagedDescriptorSet::bindImageView(uint32_t binding, std::shared_ptr<const ImageView> imageView, std::shared_ptr<const Sampler> sampler,
    VkShaderStageFlagBits shaderStages)
{
    updated = true;
}

void ManagedDescriptorSet::bindTexelBufferView(uint32_t binding, std::shared_ptr<const BufferView> texelBufferView, VkShaderStageFlagBits shaderStages)
{
    updated = true;
}

std::shared_ptr<DescriptorSet> ManagedDescriptorSet::getSet()
{
    if (updated)
    {
        // TODO: rebuild
    }
    return set;
}

std::shared_ptr<DescriptorSetLayout> ManagedDescriptorSet::getLayout()
{
    if (updated)
    {
        // TODO: rebuild
    }
    return layout;
}
} // namespace aux
} // namespace magma
