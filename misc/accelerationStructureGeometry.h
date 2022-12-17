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
#include "../misc/structureChain.h"

namespace magma
{
    class Buffer;
    class BaseIndexBuffer;

    /* Defines a triangle geometry in a bottom-level acceleration structure. */

#ifdef VK_KHR_acceleration_structure
    struct AccelerationStructureGeometryTriangles : VkAccelerationStructureGeometryTrianglesDataKHR
    {
        explicit AccelerationStructureGeometryTriangles(VkFormat vertexFormat,
            std::shared_ptr<const Buffer> vertexBuffer,
            VkDeviceSize vertexStride,
            uint32_t maxVertex,
            std::shared_ptr<const BaseIndexBuffer> indexBuffer,
            std::shared_ptr<const Buffer> transformBuffer,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureGeometryTriangles(VkFormat vertexFormat,
            const void *vertexBuffer,
            VkDeviceSize vertexStride,
            uint32_t maxVertex,
            const void *indexBuffer,
            VkIndexType indexType,
            const VkTransformMatrixKHR& transform,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
    };

    /* Defines an axis-aligned bounding box geometry in a bottom-level acceleration structure. */

    struct AccelerationStructureGeometryAabbs : VkAccelerationStructureGeometryAabbsDataKHR
    {
        explicit AccelerationStructureGeometryAabbs(std::shared_ptr<const Buffer> aabbPositions,
            VkDeviceSize stride = sizeof(VkAabbPositionsKHR),
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureGeometryAabbs(const std::vector<VkAabbPositionsKHR>& aabbPositions,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
    };

    /* Defines a geometry consisting of instances of other acceleration structures. */

    struct AccelerationStructureGeometryInstances : VkAccelerationStructureGeometryInstancesDataKHR
    {
        explicit AccelerationStructureGeometryInstances(std::shared_ptr<const Buffer> instances,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureGeometryInstances(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
    };

    /* Defines geometries to be built into an acceleration structure. */

    struct AccelerationStructureGeometry : VkAccelerationStructureGeometryKHR
    {
        explicit AccelerationStructureGeometry(const AccelerationStructureGeometryTriangles& triangles,
            VkGeometryFlagsKHR flags,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureGeometry(const AccelerationStructureGeometryAabbs& aabbs,
            VkGeometryFlagsKHR flags,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        explicit AccelerationStructureGeometry(const AccelerationStructureGeometryInstances& instances,
            VkGeometryFlagsKHR flags,
            const StructureChain& extendedInfo = StructureChain()) noexcept;
        bool triangles() const noexcept { return VK_GEOMETRY_TYPE_TRIANGLES_KHR == geometryType; }
        bool aabbs() const noexcept { return VK_GEOMETRY_TYPE_AABBS_KHR == geometryType; }
        bool instances() const noexcept { return VK_GEOMETRY_TYPE_INSTANCES_KHR == geometryType; }
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
