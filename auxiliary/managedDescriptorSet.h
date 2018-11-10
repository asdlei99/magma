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
#include "../vulkan.h"
#include "../nonCopyable.h"

namespace magma
{
    class Buffer;
    class BufferView;
    class ImageView;
    class Sampler;
    class DescriptorSet;
    class DescriptorSetLayout;
    class DescriptorPool;

    namespace aux
    {
        /* */

        // TODO: add immutable samplers
        class ManagedDescriptorSet : public sys::NonCopyable
        {
        public:
            ManagedDescriptorSet();
            void bindBuffer(uint32_t binding,
                std::shared_ptr<const Buffer> rbuffer,
                VkShaderStageFlagBits shaderStages);
            void bindImageView(uint32_t binding,
                std::shared_ptr<const ImageView> imageView,
                std::shared_ptr<const Sampler> sampler,
                VkShaderStageFlagBits shaderStages);
            void bindTexelBufferView(uint32_t binding,
                std::shared_ptr<const BufferView> texelBufferView,
                VkShaderStageFlagBits shaderStages);
            std::shared_ptr<DescriptorSet> getSet();
            std::shared_ptr<DescriptorSetLayout> getLayout();

        private:
            std::shared_ptr<DescriptorSet> set;
            std::shared_ptr<DescriptorSetLayout> layout;
            std::shared_ptr<DescriptorPool> pool;
            bool updated = false;
        };
    } // namespace aux
} // namespace magma
