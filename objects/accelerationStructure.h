/*
Magma - Abstraction layer over Khronos Vulkan API.
Copyright (C) 2018-2023 Victor Coda.

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
#ifdef VK_KHR_acceleration_structure
#include "resource.h"
#include "../misc/accelerationStructureGeometry.h"

namespace magma
{
    class Buffer;
    class AccelerationStructureStorageBuffer;
#ifdef VK_KHR_deferred_host_operations
    class DeferredOperation;
#endif

    /* Acceleration structures are opaque data structures
       that are built by the implementation to more efficiently
       perform spatial queries on the provided geometric data.
       For this extension, an acceleration structure is either
       a top-level acceleration structure containing a set of
       bottom-level acceleration structures or a bottom-level
       acceleration structure containing either a set of axis-
       aligned bounding boxes for custom geometry or a set of
       triangles. */

    class AccelerationStructure : public NonDispatchableResource<AccelerationStructure, VkAccelerationStructureKHR>
    {
    public:
        ~AccelerationStructure();
        VkAccelerationStructureTypeKHR getType() const noexcept { return structureType; }
        VkAccelerationStructureCreateFlagsKHR getFlags() const noexcept { return flags; }
        VkAccelerationStructureBuildTypeKHR getBuildType() const noexcept { return buildType; }
        VkBuildAccelerationStructureFlagsKHR getBuildFlags() const noexcept { return buildFlags; }
        VkDeviceSize getStructureSize() const noexcept { return accelerationStructureSize; }
        VkDeviceSize getBuildScratchSize() const noexcept { return buildScratchSize; }
        VkDeviceSize getUpdateScratchSize() const noexcept { return updateScratchSize; }
        VkDeviceAddress getDeviceAddress() const noexcept;
        VkDeviceSize getProperty(VkQueryType queryType) const noexcept;
        bool topLevel() const noexcept;
        bool bottomLevel() const noexcept;
        bool hostBuild() const noexcept;
        bool deviceBuild() const noexcept;
        void build(const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<AccelerationStructureBuildRange>& buildRanges,
            void *scratchBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr);
        bool update(const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<AccelerationStructureBuildRange>& buildRanges,
            void *scratchBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr) noexcept;
        bool clone(std::shared_ptr<AccelerationStructure> dstAccelerationStructure,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr) const noexcept;
        bool compact(std::shared_ptr<AccelerationStructure> dstAccelerationStructure,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr) const noexcept;
        bool copyToBuffer(std::shared_ptr<Buffer> dstBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr,
            VkCopyAccelerationStructureModeKHR mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR) const noexcept;
        bool copyToMemory(void *dstBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr,
            VkCopyAccelerationStructureModeKHR mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR) const noexcept;
        bool copyFromBuffer(std::shared_ptr<const Buffer> srcBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr,
            VkCopyAccelerationStructureModeKHR mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR) noexcept;
        bool copyFromMemory(const void *srcBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation = nullptr,
            VkCopyAccelerationStructureModeKHR mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR) noexcept;
        bool serialize(void *data) const noexcept;
        bool deserialize(const void *data) noexcept;

    protected:
        AccelerationStructure(std::shared_ptr<Device> device,
            VkAccelerationStructureTypeKHR structureType,
            const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<uint32_t>& maxPrimitiveCounts,
            VkAccelerationStructureCreateFlagsKHR createFlags,
            VkAccelerationStructureBuildTypeKHR buildType,
            VkBuildAccelerationStructureFlagsKHR buildFlags,
            std::shared_ptr<Allocator> allocator,
            const StructureChain& extendedInfo);

        VkAccelerationStructureTypeKHR structureType;

    private:
        const VkAccelerationStructureCreateFlagsKHR flags;
        const VkAccelerationStructureBuildTypeKHR buildType;
        const VkBuildAccelerationStructureFlagsKHR buildFlags;
        VkDeviceSize accelerationStructureSize;
        VkDeviceSize buildScratchSize;
        VkDeviceSize updateScratchSize;
        std::unique_ptr<AccelerationStructureStorageBuffer> buffer;
    };

    /* Top-level acceleration structure containing instance
       data referring to bottom-level acceleration structures. */

    class TopLevelAccelerationStructure : public AccelerationStructure
    {
    public:
        explicit TopLevelAccelerationStructure(std::shared_ptr<Device> device,
            const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<uint32_t>& maxPrimitiveCounts,
            VkAccelerationStructureBuildTypeKHR buildType,
            VkBuildAccelerationStructureFlagsKHR buildFlags,
            std::shared_ptr<Allocator> allocator = nullptr,
            VkAccelerationStructureCreateFlagsKHR createFlags = 0,
            const StructureChain& extendedInfo = StructureChain());
    };

    /* Bottom-level acceleration structure containing the AABBs
       or geometry to be intersected. */

    class BottomLevelAccelerationStructure : public AccelerationStructure
    {
    public:
        explicit BottomLevelAccelerationStructure(std::shared_ptr<Device> device,
            const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<uint32_t>& maxPrimitiveCounts,
            VkAccelerationStructureBuildTypeKHR buildType,
            VkBuildAccelerationStructureFlagsKHR buildFlags,
            std::shared_ptr<Allocator> allocator = nullptr,
            VkAccelerationStructureCreateFlagsKHR createFlags = 0,
            const StructureChain& extendedInfo = StructureChain());
    };

    /* Generic acceleration structure is intended to be used
       by API translation layers. In these cases, the acceleration
       structure type is not known at creation time, but must be
       specified at build time as either top or bottom. */

    class GenericAccelerationStructure : public AccelerationStructure
    {
    public:
        explicit GenericAccelerationStructure(std::shared_ptr<Device> device,
            const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<uint32_t>& maxPrimitiveCounts,
            VkAccelerationStructureBuildTypeKHR buildType,
            VkBuildAccelerationStructureFlagsKHR buildFlags,
            std::shared_ptr<Allocator> allocator = nullptr,
            VkAccelerationStructureCreateFlagsKHR createFlags = 0,
            const StructureChain& extendedInfo = StructureChain());
        void build(VkAccelerationStructureTypeKHR structureType_,
            const std::vector<AccelerationStructureGeometry>& geometries,
            const std::vector<AccelerationStructureBuildRange>& buildRanges,
            void *scratchBuffer,
            std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */);
    };
} // namespace magma

#include "accelerationStructure.inl"
#endif // VK_KHR_acceleration_structure
