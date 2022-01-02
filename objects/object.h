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
#include "objectType.h"
#include "../core/noncopyable.h"
#include "../allocator/cxxAllocator.h"

namespace magma
{
    class Device;
    class IAllocator;

    /* Base non-copyable class for dispatchable and non-dispatchable objects.
       Allows to give a user-friendly name and to attach arbitrary data to an object. */

    class Object : public CxxAllocator,
        public core::NonCopyable
    {
    public:
        explicit Object(std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> hostAllocator) noexcept;
        virtual ~Object() = default;
        virtual VkObjectType getObjectType() const noexcept = 0;
        virtual uint64_t getHandle() const noexcept = 0;
        std::shared_ptr<Device> getDevice() const noexcept { return device; }
        std::shared_ptr<IAllocator> getHostAllocator() const noexcept { return hostAllocator; }
        void setDebugName(const char *name);
        void setDebugTag(uint64_t tagName,
            std::size_t tagSize,
            const void *tag);
        template<typename Type>
        void setDebugTag(uint64_t tagName,
            const Type& tag);

    protected:
        std::shared_ptr<Device> device;
        std::shared_ptr<IAllocator> hostAllocator;
    };

    /* Template object that provides getObjectType() getter. */

    template<typename Type>
    class ObjectT : public Object
#ifdef MAGMA_X64
        ,public ObjectType<Type> // Use custom template specialization
#endif
    {
    public:
        explicit ObjectT(VkObjectType objectType,
            std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> hostAllocator) noexcept;
        VkObjectType getObjectType() const noexcept override;

    protected:
#if !defined(MAGMA_X64)
        // Additional storage is required under x86 target
        // as Vulkan non-dispatchable handles are defined as uint64_t
        // and thus cannot be used in custom template specialization.
        const VkObjectType objectType;
#endif
    };
} // namespace magma

#include "object.inl"
