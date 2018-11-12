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

        class ManagedDescriptorSet : public sys::NonCopyable
        {
            enum ShaderStage : uint32_t {
                Vertex = 0, TessControl, TessEvaluation,
                Geometry, Fragment, Compute, Count
            };

        public:
            class ShaderStageBindings;
            explicit ManagedDescriptorSet(std::shared_ptr<Device> device,
                std::shared_ptr<DescriptorPool> pool = nullptr,
                std::shared_ptr<IAllocator> allocator = nullptr);
            std::shared_ptr<ShaderStageBindings>& vertexStage() noexcept
                { return stages[ShaderStage::Vertex]; }
            std::shared_ptr<ShaderStageBindings>& tessControlStage() noexcept
                { return stages[ShaderStage::TessControl]; }
            std::shared_ptr<ShaderStageBindings>& tessEvaluationStage() noexcept
                { return stages[ShaderStage::TessEvaluation]; }
            std::shared_ptr<ShaderStageBindings>& geometryStage() noexcept
                { return stages[ShaderStage::Geometry]; }
            std::shared_ptr<ShaderStageBindings>& fragmentStage() noexcept
                { return stages[ShaderStage::Fragment]; }
            std::shared_ptr<ShaderStageBindings>& computeStage() noexcept
                { return stages[ShaderStage::Compute]; }
            void finalize();
            std::shared_ptr<DescriptorSetLayout> getLayout();
            std::shared_ptr<DescriptorSet> getDescriptorSet();

        private:
            std::shared_ptr<Device> device;
            std::shared_ptr<IAllocator> allocator;
            std::shared_ptr<DescriptorPool> pool;
            std::shared_ptr<DescriptorSetLayout> layout;
            std::shared_ptr<DescriptorSet> set;
            std::shared_ptr<ShaderStageBindings> stages[ShaderStage::Count];
        };

        /* */

        // TODO: support immutable samplers
        class ManagedDescriptorSet::ShaderStageBindings
        {
        public:
            explicit ShaderStageBindings(VkShaderStageFlagBits stage): stage(stage) {}
            void bindImageView(uint32_t binding,
                std::shared_ptr<const ImageView> imageView,
                std::shared_ptr<const Sampler> sampler);
            void bindBuffer(uint32_t binding,
                std::shared_ptr<const Buffer> buffer,
                VkDeviceSize offset = 0,
                VkDeviceSize range = VK_WHOLE_SIZE);
            void bindTexelBufferView(uint32_t binding,
                std::shared_ptr<const BufferView> texelBufferView);
            VkShaderStageFlagBits getStage() const noexcept { return stage; }

        private:
            VkShaderStageFlagBits stage;
            std::unordered_map<uint32_t, VkWriteDescriptorSet> descriptorWrites;
            std::list<VkDescriptorImageInfo> imageInfos;
            std::list<VkDescriptorBufferInfo> bufferInfos;
            std::list<VkBufferView> bufferViews;
            bool updated = false;
            friend ManagedDescriptorSet;
        };
    } // namespace aux
} // namespace magma
