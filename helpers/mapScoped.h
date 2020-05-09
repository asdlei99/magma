/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2019 Victor Coda.

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
#include "uniformArray.h"
#include "alignedUniformArray.h"
#include "../objects/deviceMemory.h"
#include "../objects/buffer.h"
#include "../objects/uniformBuffer.h"
#include "../objects/image.h"
#include "../core/zeroMemory.h"

namespace magma
{
    namespace helpers
    {
        template<typename Type>
        inline void mapScoped(
            const std::shared_ptr<Buffer>& buffer,
            std::function<void(Type *data)> fn)
        {
            MAGMA_PROFILE_FUNCTION
            MAGMA_ASSERT(buffer);
            MAGMA_ASSERT(fn);
            std::shared_ptr<DeviceMemory> memory = buffer->getMemory();
            if (memory)
            {
                void *const data = memory->map();
                if (data)
                {
                    fn(static_cast<Type *>(data));
                    memory->unmap();
                }
            }
        }

        template<typename Block>
        inline void mapScoped(
            const std::shared_ptr<UniformBuffer<Block>>& buffer,
            bool clearMemory,
            std::function<void(Block *block)> fn)
        {
            MAGMA_PROFILE_FUNCTION
            MAGMA_ASSERT(buffer);
            MAGMA_ASSERT(fn);
            ZeroMemoryFunction zeroFn = clearMemory ? core::zeroMemory : nullptr;
            Block *const block = buffer->map(zeroFn);
            if (block)
            {
                fn(block);
                buffer->unmap();
            }
        }

        template<typename Type>
        inline void mapScoped(
            const std::shared_ptr<UniformBuffer<Type>>& buffer,
            bool clearMemory,
            std::function<void(UniformArray<Type>& array)> fn)
        {
            MAGMA_PROFILE_FUNCTION
            MAGMA_ASSERT(buffer);
            MAGMA_ASSERT(fn);
            ZeroMemoryFunction zeroFn = clearMemory ? core::zeroMemory : nullptr;
            Type *const data = buffer->map(zeroFn);
            if (data)
            {
                UniformArray<Type> array(data,
                    buffer->getArraySize());
                fn(array);
                buffer->unmap();
            }
        }

        template<typename Type>
        inline void mapScoped(
            const std::shared_ptr<DynamicUniformBuffer<Type>>& buffer,
            bool clearMemory,
            std::function<void(AlignedUniformArray<Type>& array)> fn)
        {
            MAGMA_PROFILE_FUNCTION
            MAGMA_ASSERT(buffer);
            MAGMA_ASSERT(fn);
            ZeroMemoryFunction zeroFn = clearMemory ? core::zeroMemory : nullptr;
            Type *const data = buffer->map(zeroFn);
            if (data)
            {
                AlignedUniformArray<Type> array(data,
                    buffer->getArraySize(),
                    buffer->getElementAlignment());
                fn(array);
                buffer->unmap();
            }
        }

        template<typename Type>
        inline void mapScoped(
            const std::shared_ptr<Image>& image,
            VkDeviceSize offset,
            std::function<void(Type *data)> fn)
        {
            MAGMA_PROFILE_FUNCTION
            MAGMA_ASSERT(image);
            MAGMA_ASSERT(fn);
            std::shared_ptr<DeviceMemory> memory = image->getMemory();
            if (memory)
            {
                void *const data = memory->map(offset);
                if (data)
                {
                    fn(static_cast<Type *>(data));
                    memory->unmap();
                }
            }
        }
    } // namespace helpers
} // namespace magma
