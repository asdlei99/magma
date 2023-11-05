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
#include "structureChain.h"

namespace magma
{
    class Buffer;

    /* Geometries to be built into an acceleration structure. */

#ifdef VK_KHR_acceleration_structure
    struct AccelerationStructureGeometry : VkAccelerationStructureGeometryKHR
    {
    protected:
        AccelerationStructureGeometry(VkGeometryTypeKHR geometryType,
            VkGeometryFlagsKHR flags,
            const StructureChain& extendedInfo) noexcept;
    };

    /* Triangle geometry in a bottom-level acceleration structure. */

    struct AccelerationStructureTriangles : AccelerationStructureGeometry
    {
        explicit AccelerationStructureTriangles(VkFormat vertexFormat,
            const void *vertexData,
            VkDeviceSize vertexStride,
            uint32_t maxVertex,
            VkIndexType indexType,
            const void *indexData,
            const VkTransformMatrixKHR& transform,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureTriangles(VkFormat vertexFormat,
            std::shared_ptr<const Buffer> vertexData,
            VkDeviceSize vertexStride,
            uint32_t maxVertex,
            VkIndexType indexType,
            std::shared_ptr<const Buffer> indexData,
            const VkTransformMatrixKHR& transform,
            const StructureChain& extendedInfo = StructureChain()) noexcept;

        const VkTransformMatrixKHR transform;
    };

    /* Axis-aligned bounding box geometry in a bottom-level acceleration structure. */

    struct AccelerationStructureAabbs : AccelerationStructureGeometry
    {
        explicit AccelerationStructureAabbs(const std::vector<VkAabbPositionsKHR>& aabbPositions,
            VkGeometryFlagsKHR flags = 0,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureAabbs(std::shared_ptr<const Buffer> aabbPositions,
            VkDeviceSize stride = sizeof(VkAabbPositionsKHR),
            VkGeometryFlagsKHR flags = 0,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
    };

    /* Geometry consisting of instances of other acceleration structures. */

    struct AccelerationStructureInstances : AccelerationStructureGeometry
    {
        explicit AccelerationStructureInstances(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
            bool arrayOfPointers = false,
            VkGeometryFlagsKHR flags = 0,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureInstances(std::shared_ptr<const Buffer> instances,
            bool arrayOfPointers = false,
            VkGeometryFlagsKHR flags = 0,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
    };

    /* Defines build offsets and counts for acceleration structure builds. */

    struct AccelerationStructureBuildRange : VkAccelerationStructureBuildRangeInfoKHR
    {
        AccelerationStructureBuildRange() = default;
        explicit AccelerationStructureBuildRange(uint32_t primitiveCount,
            uint32_t primitiveOffset = 0,
            uint32_t firstVertex = 0,
            uint32_t transformOffset = 0);
    };
#endif // VK_KHR_acceleration_structure
} // namespace magma
