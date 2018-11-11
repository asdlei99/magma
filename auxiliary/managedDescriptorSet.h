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
#include <memory>
#include <list>
#include <unordered_map>
#include "../vulkan.h"
#include "../nonCopyable.h"

namespace magma
{
    class Device;
    class Buffer;
    class BufferView;
    class ImageView;
    class Sampler;
    class DescriptorSet;
    class DescriptorSetLayout;
    class DescriptorPool;
    class IAllocator;

    namespace aux
    {
        /* */

        // TODO: support immutable samplers
        class ManagedDescriptorSet : public sys::NonCopyable
        {
        public:
            class ShaderStageBindings
            {
            public:
                ShaderStageBindings() {}
                void bindImageView(uint32_t binding,
                    std::shared_ptr<const ImageView> imageView,
                    std::shared_ptr<const Sampler> sampler);
                void bindBuffer(uint32_t binding,
                    std::shared_ptr<const Buffer> buffer,
                    VkDeviceSize offset = 0,
                    VkDeviceSize range = VK_WHOLE_SIZE);
                void bindTexelBufferView(uint32_t binding,
                    std::shared_ptr<const BufferView> texelBufferView);

            private:
                std::unordered_map<uint32_t, VkWriteDescriptorSet> descriptorWrites; // Should be unique descriptor for each binding!
                std::list<VkDescriptorImageInfo> imageInfos;
                std::list<VkDescriptorBufferInfo> bufferInfos;
                std::list<VkBufferView> bufferViews;
                bool updated = false;
                friend ManagedDescriptorSet;
            };

        public:
            ManagedDescriptorSet(std::shared_ptr<Device> device,
                std::shared_ptr<DescriptorPool> pool = nullptr,
                std::shared_ptr<IAllocator> allocator = nullptr);
            ShaderStageBindings& vertexStage() { return stages[VK_SHADER_STAGE_VERTEX_BIT]; }
            ShaderStageBindings& tessControlStage() { return stages[VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT]; }
            ShaderStageBindings& tessEvaluationStage() { return stages[VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT]; }
            ShaderStageBindings& geometryStage() { return stages[VK_SHADER_STAGE_GEOMETRY_BIT]; }
            ShaderStageBindings& fragmentStage() { return stages[VK_SHADER_STAGE_FRAGMENT_BIT]; }
            ShaderStageBindings& computeStage() { return stages[VK_SHADER_STAGE_COMPUTE_BIT]; }
            void finalize();
            std::shared_ptr<DescriptorSetLayout> getLayout();
            std::shared_ptr<DescriptorSet> getDescriptorSet();

        private:
            std::shared_ptr<Device> device;
            std::shared_ptr<IAllocator> allocator;
            std::shared_ptr<DescriptorPool> pool;
            std::shared_ptr<DescriptorSetLayout> layout;
            std::shared_ptr<DescriptorSet> set;
            std::unordered_map<VkShaderStageFlagBits, ShaderStageBindings> stages;
        };
    } // namespace aux
} // namespace magma
