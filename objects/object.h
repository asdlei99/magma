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
#include "objectType.h"
#include "../allocator/objectAllocator.h"
#include "../profiler/profiler.h"
#include "../misc/deviceExtension.h"
#include "../helpers/castToDebugReport.h"

namespace magma
{
    class Device;
    class IAllocator;

    /* Base non-copyable object for dispatchable and non-dispatchable handles. */

    template<typename Type>
    class Object : public core::NonCopyable,
#ifdef MAGMA_X64
        // Use custom template specialization for ::getObjectType() method
        public ObjectType<Type>,
#endif
        public memory::Allocator
    {
    public:
        explicit Object(VkObjectType objectType,
            std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> allocator) noexcept;
        virtual ~Object() = default;
#if !defined(MAGMA_X64)
        VkObjectType getObjectType() const noexcept { return objectType; }
#endif
        std::shared_ptr<Device> getDevice() const noexcept { return device; }
        std::shared_ptr<IAllocator> getAllocator() const noexcept { return allocator; }
        virtual uint64_t getHandle() const noexcept = 0;
        void setObjectName(const char *name) noexcept;
        void setObjectTag(uint64_t tagName, std::size_t tagSize, const void *tag) noexcept;
        template<typename TagType>
        void setObjectTag(uint64_t tagName, const TagType& tag) noexcept;

    protected:
#if !defined(MAGMA_X64)
        // Additional storage is required under x86 target
        // as Vulkan non-dispatchable handles are defined as uint64_t
        // and thus cannot be used in custom template specialization.
        VkObjectType objectType;
#endif
        std::shared_ptr<Device> device;
        std::shared_ptr<IAllocator> allocator;
    };
} // namespace magma

#include "object.inl"
