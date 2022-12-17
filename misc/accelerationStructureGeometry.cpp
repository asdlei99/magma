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
#include "pch.h"
#pragma hdrstop
#include "accelerationStructureGeometry.h"
#include "../objects/indexBuffer.h"

namespace magma
{
#ifdef VK_KHR_acceleration_structure
AccelerationStructureGeometryTriangles::AccelerationStructureGeometryTriangles(VkFormat vertexFormat_, std::shared_ptr<const Buffer> vertexBuffer, VkDeviceSize vertexStride_,
    uint32_t maxVertex_, std::shared_ptr<const BaseIndexBuffer> indexBuffer, std::shared_ptr<const Buffer> transformBuffer,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    pNext = extendedInfo.getChainedNodes();
    vertexFormat = vertexFormat_;
    vertexData.deviceAddress = vertexBuffer->getDeviceAddress();
    vertexStride = vertexStride_;
    maxVertex = maxVertex_;
    indexType = indexBuffer->getIndexType();
    indexData.deviceAddress = indexBuffer->getDeviceAddress();
    transformData.deviceAddress = transformBuffer->getDeviceAddress();
}

AccelerationStructureGeometryTriangles::AccelerationStructureGeometryTriangles(VkFormat vertexFormat_, const void *vertexBuffer, VkDeviceSize vertexStride_,
    uint32_t maxVertex_, const void *indexBuffer, VkIndexType indexType_, const VkTransformMatrixKHR& transform,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    pNext = extendedInfo.getChainedNodes();
    vertexFormat = vertexFormat_;
    vertexData.hostAddress = vertexBuffer;
    vertexStride = vertexStride_;
    maxVertex = maxVertex_;
    indexType = indexType_;
    indexData.hostAddress = indexBuffer;
    transformData.hostAddress = &transform;
}

AccelerationStructureGeometryAabbs::AccelerationStructureGeometryAabbs(std::shared_ptr<const Buffer> aabbPositions,
    VkDeviceSize stride_ /* sizeof(VkAabbPositionsKHR) */,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    pNext = extendedInfo.getChainedNodes();
    data.deviceAddress = aabbPositions->getDeviceAddress();
    stride = stride_;
}

AccelerationStructureGeometryAabbs::AccelerationStructureGeometryAabbs(const std::vector<VkAabbPositionsKHR>& aabbPositions,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    pNext = extendedInfo.getChainedNodes();
    data.hostAddress = aabbPositions.data();
    stride = sizeof(VkAabbPositionsKHR);
}

AccelerationStructureGeometryInstances::AccelerationStructureGeometryInstances(std::shared_ptr<const Buffer> instances,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    pNext = extendedInfo.getChainedNodes();
    arrayOfPointers = VK_FALSE;
    data.deviceAddress = instances->getDeviceAddress();
}

AccelerationStructureGeometryInstances::AccelerationStructureGeometryInstances(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    pNext = extendedInfo.getChainedNodes();
    arrayOfPointers = VK_FALSE;
    data.hostAddress = instances.data();
}

AccelerationStructureGeometry::AccelerationStructureGeometry(const AccelerationStructureGeometryTriangles& triangles, VkGeometryFlagsKHR flags_,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    pNext = extendedInfo.getChainedNodes();
    geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.triangles = triangles;
    geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.aabbs.pNext = nullptr;
    geometry.aabbs.data.hostAddress = nullptr;
    geometry.aabbs.stride = 0;
    geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.instances.pNext = nullptr;
    geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.instances.data.hostAddress = nullptr;
    flags = flags_;
}

AccelerationStructureGeometry::AccelerationStructureGeometry(const AccelerationStructureGeometryAabbs& aabbs, VkGeometryFlagsKHR flags_,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    pNext = extendedInfo.getChainedNodes();
    geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    geometry.aabbs = aabbs;
    geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.triangles.pNext = nullptr;
    geometry.triangles.vertexFormat = VK_FORMAT_UNDEFINED;
    geometry.triangles.vertexData.hostAddress = nullptr;
    geometry.triangles.vertexStride = 0;
    geometry.triangles.maxVertex = 0;
    geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
    geometry.triangles.indexData.hostAddress = nullptr;
    geometry.triangles.transformData.hostAddress = nullptr;
    geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometry.instances.pNext = nullptr;
    geometry.instances.arrayOfPointers = VK_FALSE;
    geometry.instances.data.hostAddress = nullptr;
    flags = flags_;
}

AccelerationStructureGeometry::AccelerationStructureGeometry(const AccelerationStructureGeometryInstances& instances, VkGeometryFlagsKHR flags_,
    const StructureChain& extendedInfo /* default */) noexcept
{
    sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    pNext = extendedInfo.getChainedNodes();
    geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.instances = instances;
    geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geometry.triangles.pNext = nullptr;
    geometry.triangles.vertexFormat = VK_FORMAT_UNDEFINED;
    geometry.triangles.vertexData.hostAddress = nullptr;
    geometry.triangles.vertexStride = 0;
    geometry.triangles.maxVertex = 0;
    geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
    geometry.triangles.indexData.hostAddress = nullptr;
    geometry.triangles.transformData.hostAddress = nullptr;
    geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    geometry.aabbs.pNext = nullptr;
    geometry.aabbs.data.hostAddress = nullptr;
    geometry.aabbs.stride = 0;
    flags = flags_;
}
#endif // VK_KHR_acceleration_structure
} // namespace magma
