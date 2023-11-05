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
#include "pch.h"
#pragma hdrstop
#include "accelerationStructureGeometry.h"
#include "../objects/buffer.h"

namespace magma
{
#ifdef VK_KHR_acceleration_structure
AccelerationStructureGeometry::AccelerationStructureGeometry(VkGeometryTypeKHR geometryType, VkGeometryFlagsKHR flags,
    const StructureChain& extendedInfo) noexcept:
    VkAccelerationStructureGeometryKHR{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        extendedInfo.chainNodes(),
        geometryType,
        VkAccelerationStructureGeometryDataKHR{},
        flags
    }
{}

AccelerationStructureTriangles::AccelerationStructureTriangles(VkFormat vertexFormat, const void *vertexData, VkDeviceSize vertexStride,
    uint32_t maxVertex, VkIndexType indexType, const void *indexData, const VkTransformMatrixKHR& transform_,
    const StructureChain& extendedInfo /* default */) noexcept:
    AccelerationStructureGeometry(VK_GEOMETRY_TYPE_TRIANGLES_KHR, flags, extendedInfo),
    transform(transform_)
{
    geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.triangles.pNext = nullptr;
    geometry.triangles.vertexFormat = vertexFormat;
    geometry.triangles.vertexData.hostAddress = vertexData;
    geometry.triangles.vertexStride = vertexStride;
    geometry.triangles.maxVertex = maxVertex;
    geometry.triangles.indexType = indexType;
    geometry.triangles.indexData.hostAddress = indexData;
    geometry.triangles.transformData.hostAddress = &transform;
}

AccelerationStructureTriangles::AccelerationStructureTriangles(VkFormat vertexFormat, std::shared_ptr<const Buffer> vertexData, VkDeviceSize vertexStride,
    uint32_t maxVertex, VkIndexType indexType, std::shared_ptr<const Buffer> indexData, const VkTransformMatrixKHR& transform_,
    const StructureChain& extendedInfo /* default */) noexcept:
    AccelerationStructureGeometry(VK_GEOMETRY_TYPE_TRIANGLES_KHR, flags, extendedInfo),
    transform(transform_)
{
    geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.triangles.pNext = nullptr;
    geometry.triangles.vertexFormat = vertexFormat;
    geometry.triangles.vertexData.deviceAddress = vertexData->getDeviceAddress();
    geometry.triangles.vertexStride = vertexStride;
    geometry.triangles.maxVertex = maxVertex;
    geometry.triangles.indexType = indexType;
    geometry.triangles.indexData.deviceAddress = indexData->getDeviceAddress();
    geometry.triangles.transformData.hostAddress = &transform;
}

AccelerationStructureAabbs::AccelerationStructureAabbs(const std::vector<VkAabbPositionsKHR>& aabbPositions,
    VkGeometryFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */) noexcept:
    AccelerationStructureGeometry(VK_GEOMETRY_TYPE_AABBS_KHR, flags, extendedInfo)
{
    geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.aabbs.pNext = nullptr;
    geometry.aabbs.data.hostAddress = aabbPositions.data();
    geometry.aabbs.stride = sizeof(VkAabbPositionsKHR);
}

AccelerationStructureAabbs::AccelerationStructureAabbs(std::shared_ptr<const Buffer> aabbPositions,
    VkDeviceSize stride /* sizeof(VkAabbPositionsKHR) */,
    VkGeometryFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */) noexcept:
    AccelerationStructureGeometry(VK_GEOMETRY_TYPE_AABBS_KHR, flags, extendedInfo)
{
    geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.aabbs.pNext = nullptr;
    geometry.aabbs.data.deviceAddress = aabbPositions->getDeviceAddress();
    geometry.aabbs.stride = stride;
}

AccelerationStructureInstances::AccelerationStructureInstances(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
    bool arrayOfPointers /* false */,
    VkGeometryFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */) noexcept:
    AccelerationStructureGeometry(VK_GEOMETRY_TYPE_INSTANCES_KHR, flags, extendedInfo)
{
    geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.instances.pNext = nullptr;
    geometry.instances.arrayOfPointers = MAGMA_BOOLEAN(arrayOfPointers);
    geometry.instances.data.hostAddress = instances.data();
}

AccelerationStructureInstances::AccelerationStructureInstances(std::shared_ptr<const Buffer> instances,
    bool arrayOfPointers /* false */,
    VkGeometryFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */) noexcept:
    AccelerationStructureGeometry(VK_GEOMETRY_TYPE_INSTANCES_KHR, flags, extendedInfo)
{
    geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.instances.pNext = nullptr;
    geometry.instances.arrayOfPointers = MAGMA_BOOLEAN(arrayOfPointers);
    geometry.instances.data.deviceAddress = instances->getDeviceAddress();
}
#endif // VK_KHR_acceleration_structure
} // namespace magma
