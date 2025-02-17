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
#include "../allocator/cxxAllocator.h"

namespace magma
{
    class Device;
    class IAllocator;

    /* Base non-copyable class for dispatchable and non-
       dispatchable objects. Allows to give a user-friendly
       name and to attach arbitrary data to an object. */

    class Object : public CxxAllocator,
        public IDestructible,
        /* private */ NonCopyable
    {
    public:
        explicit Object(std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> hostAllocator) noexcept;
        virtual VkObjectType getObjectType() const noexcept = 0;
        virtual uint64_t getHandle() const noexcept = 0;
        const std::shared_ptr<Device>& getDevice() const noexcept { return device; }
        const std::shared_ptr<IAllocator>& getHostAllocator() const noexcept { return hostAllocator; }
        void setPrivateData(uint64_t data);
        uint64_t getPrivateData() const;
    #ifdef MAGMA_DEBUG
        void setDebugName(const std::string& name);
        const std::string& getDebugName() const noexcept { return name; }
        void setDebugTag(uint64_t tagName, std::size_t tagSize, const void *tag);
        template<class Type>
        void setDebugTag(uint64_t tagName, const Type& tag);
        uint64_t getDebugTag() const noexcept { return tagName; }
    #else // No-op in release build
        void setDebugName(const std::string&) const noexcept {}
        std::string getDebugName() const noexcept { return std::string(); }
        void setDebugTag(uint64_t, std::size_t, const void *) const noexcept {}
        template<class Type>
        void setDebugTag(uint64_t, const Type&) const noexcept {}
        uint64_t getDebugTag() const noexcept { return 0; }
    #endif // MAGMA_DEBUG

    protected:
        std::shared_ptr<Device> device;
        std::shared_ptr<IAllocator> hostAllocator;
        static std::mutex mtx;
    #ifdef MAGMA_DEBUG
        std::string name;
        uint64_t tagName = 0;
    #endif
    };

    /* Template object that provides getObjectType() getter. */

    template<class Type>
    class TObject :
    #ifdef MAGMA_X64
        public ObjectType<Type>, // Use custom template specialization
    #endif
        public Object
    {
    public:
        typedef Type NativeHandle;
        explicit TObject(VkObjectType objectType,
            std::shared_ptr<IAllocator> hostAllocator) noexcept;
        explicit TObject(VkObjectType objectType,
            std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> hostAllocator);
        explicit TObject(VkObjectType objectType,
            Type handle,
            std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> hostAllocator);
        VkObjectType getObjectType() const noexcept override;
        const NativeHandle *getHandleAddress() const noexcept { return &handle; }
        operator NativeHandle() const noexcept { return handle; }

    protected:
        // Additional storage is required under x86 target
        // as Vulkan non-dispatchable handles are defined as uint64_t
        // and thus cannot be used in custom template specialization.
    #if !defined(MAGMA_X64)
        const VkObjectType objectType;
    #endif
        NativeHandle handle;
    };
} // namespace magma

#include "object.inl"
