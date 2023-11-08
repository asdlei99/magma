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
#pragma once

namespace magma
{
    /* Pool descriptor contains a descriptor type and number of
       descriptors of that type to be allocated in the pool. */

    namespace descriptor
    {
        struct DescriptorPool : VkDescriptorPoolSize
        {
            constexpr DescriptorPool(const VkDescriptorType type, const uint32_t descriptorCount) noexcept:
                VkDescriptorPoolSize{type, descriptorCount} {}
        };

        #define MAGMA_DEFINE_DESCRIPTOR_POOL(Pool, descriptorType)\
        struct Pool : DescriptorPool\
        {\
            constexpr Pool(const uint32_t count) noexcept:\
                DescriptorPool(descriptorType, count) {}\
        };

        MAGMA_DEFINE_DESCRIPTOR_POOL(SamplerPool, VK_DESCRIPTOR_TYPE_SAMPLER)
        MAGMA_DEFINE_DESCRIPTOR_POOL(CombinedImageSamplerPool, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        MAGMA_DEFINE_DESCRIPTOR_POOL(SampledImagePool, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        MAGMA_DEFINE_DESCRIPTOR_POOL(StorageImagePool, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
        MAGMA_DEFINE_DESCRIPTOR_POOL(UniformTexelBufferPool, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
        MAGMA_DEFINE_DESCRIPTOR_POOL(StorageTexelBufferPool, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
        MAGMA_DEFINE_DESCRIPTOR_POOL(UniformBufferPool, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        MAGMA_DEFINE_DESCRIPTOR_POOL(StorageBufferPool, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        MAGMA_DEFINE_DESCRIPTOR_POOL(DynamicUniformBufferPool, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        MAGMA_DEFINE_DESCRIPTOR_POOL(DynamicStorageBufferPool, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
        MAGMA_DEFINE_DESCRIPTOR_POOL(InputAttachmentPool, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
    #ifdef VK_KHR_acceleration_structure
        MAGMA_DEFINE_DESCRIPTOR_POOL(AccelerationStructurePool, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
    #endif
    #ifdef VK_VALVE_mutable_descriptor_type
        MAGMA_DEFINE_DESCRIPTOR_POOL(MutableDescriptorPool, VK_DESCRIPTOR_TYPE_MUTABLE_VALVE)
    #endif
    #ifdef VK_QCOM_image_processing
        MAGMA_DEFINE_DESCRIPTOR_POOL(SampleWeightImagePool, VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM)
        MAGMA_DEFINE_DESCRIPTOR_POOL(BlockMatchImagePool, VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM)
    #endif // VK_QCOM_image_processing

    #ifdef VK_EXT_inline_uniform_block
        template<class UniformBlockType>
        struct InlineUniformBlockPool : DescriptorPool
        {
            constexpr InlineUniformBlockPool() noexcept:
                DescriptorPool(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, sizeof(UniformBlockType)) {}
        };
    #endif // VK_EXT_inline_uniform_block
    } // namespace descriptor
} // namespace magma
