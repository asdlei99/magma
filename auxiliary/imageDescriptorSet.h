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
#include "base.h"
#include "../objects/descriptorSetLayout.h"

namespace magma
{
    class Device;
    class DescriptorPool;
    class DescriptorSet;
    class DescriptorSetLayout;
    class ShaderReflection;
    class ImageView;
    class Sampler;
    class IAllocator;

    namespace aux
    {
        /* Allocates descriptor set for the first image binding
           found in the shader reflection. */

        class ImageDescriptorSet : public Base
        {
        public:
            explicit ImageDescriptorSet(std::shared_ptr<Device> device,
                std::shared_ptr<const ShaderReflection> reflection,
                std::shared_ptr<IAllocator> allocator = nullptr);
            ~ImageDescriptorSet();
            const std::shared_ptr<DescriptorSetLayout>& getLayout() const noexcept { return descriptorSetLayout; }
            const std::shared_ptr<DescriptorSet>& getSet() const noexcept { return descriptorSet; }
            void writeDescriptor(std::shared_ptr<const ImageView> imageView,
                std::shared_ptr<Sampler> sampler);

        private:
            struct ImageTable;
            struct StorageImageTable;
            std::shared_ptr<DescriptorPool> descriptorPool;
            std::shared_ptr<DescriptorSetLayout> descriptorSetLayout;
            std::shared_ptr<DescriptorSet> descriptorSet;
            std::unique_ptr<ImageTable> imageTable;
            std::unique_ptr<StorageImageTable> storageImageTable;
            uint32_t binding;
        };
    } // namespace aux
} // namespace magma
