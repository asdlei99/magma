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
#include "accelerationStructureKhr.h"
#include "accelerationStructureBuffer.h"
#include "device.h"
#include "commandBuffer.h"
#include "../allocator/allocator.h"
#include "../misc/extProcAddress.h"
#include "../exceptions/errorResult.h"

namespace magma
{
#ifdef VK_KHR_acceleration_structure
AccelerationStructure::AccelerationStructure(std::shared_ptr<Device> device,
    VkAccelerationStructureTypeKHR structureType, VkAccelerationStructureCreateFlagsKHR createFlags,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    const StructureChain& extendedInfo, std::shared_ptr<Allocator> allocator):
    NonDispatchableResource(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, device, Sharing(), allocator),
    structureType(structureType),
    createFlags(createFlags),
    buildType(buildType),
    buildFlags(buildFlags),
    accelerationStructureSize(0),
    updateScratchSize(0),
    buildScratchSize(0)
{
    VkAccelerationStructureCreateInfoKHR accelerationStructureInfo;
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizes;
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = nullptr;
    accelerationStructureBuildGeometryInfo.type = structureType;
    accelerationStructureBuildGeometryInfo.flags = buildFlags;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // Ignored
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE; // Ignored
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = VK_NULL_HANDLE; // Ignored
    accelerationStructureBuildGeometryInfo.geometryCount = MAGMA_COUNT(geometries);
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();
    accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
    accelerationStructureBuildGeometryInfo.scratchData.hostAddress = nullptr;
    accelerationStructureBuildSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    accelerationStructureBuildSizes.pNext = nullptr;
    accelerationStructureBuildSizes.accelerationStructureSize = 0;
    accelerationStructureBuildSizes.updateScratchSize = 0;
    accelerationStructureBuildSizes.buildScratchSize = 0;
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureInfo.pNext = extendedInfo.getChainedNodes();
    accelerationStructureInfo.createFlags = createFlags;
    accelerationStructureInfo.offset = 0;
    accelerationStructureInfo.type = structureType;
    accelerationStructureInfo.deviceAddress = 0;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetAccelerationStructureBuildSizesKHR, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    vkGetAccelerationStructureBuildSizesKHR(MAGMA_HANDLE(device), buildType, &accelerationStructureBuildGeometryInfo, maxPrimitiveCounts.data(), &accelerationStructureBuildSizes);
    buffer = std::make_shared<AccelerationStructureBuffer>(std::move(device), accelerationStructureBuildSizes.accelerationStructureSize, std::move(allocator));
    accelerationStructureInfo.buffer = *buffer;
    accelerationStructureInfo.size = accelerationStructureBuildSizes.accelerationStructureSize;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkCreateAccelerationStructureKHR, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    const VkResult result = vkCreateAccelerationStructureKHR(MAGMA_HANDLE(device), &accelerationStructureInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create acceleration structure");
    accelerationStructureSize = accelerationStructureBuildSizes.accelerationStructureSize;
    updateScratchSize = accelerationStructureBuildSizes.updateScratchSize;
    buildScratchSize = accelerationStructureBuildSizes.buildScratchSize;
}

AccelerationStructure::~AccelerationStructure()
{
    MAGMA_DEVICE_EXTENSION(vkDestroyAccelerationStructureKHR);
    vkDestroyAccelerationStructureKHR(MAGMA_HANDLE(device), handle, MAGMA_OPTIONAL_INSTANCE(hostAllocator));
}

VkDeviceAddress AccelerationStructure::getDeviceAddress() const noexcept
{
    VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo;
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    deviceAddressInfo.pNext = nullptr;
    deviceAddressInfo.accelerationStructure = handle;
    MAGMA_DEVICE_EXTENSION(vkGetAccelerationStructureDeviceAddressKHR);
    return vkGetAccelerationStructureDeviceAddressKHR(*device, &deviceAddressInfo);
}

TopLevelAccelerationStructure::TopLevelAccelerationStructure(std::shared_ptr<Device> device, VkAccelerationStructureCreateFlagsKHR createFlags,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    std::shared_ptr<Allocator> allocator /* nullptr */,
    const StructureChain& extendedInfo /* default */):
    AccelerationStructure(std::move(device), VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        createFlags, buildType, buildFlags, geometries, maxPrimitiveCounts,
        extendedInfo, std::move(allocator))
{}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(std::shared_ptr<Device> device, VkAccelerationStructureCreateFlagsKHR createFlags,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    std::shared_ptr<Allocator> allocator /* nullptr */,
    const StructureChain& extendedInfo /* default */):
    AccelerationStructure(std::move(device), VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        createFlags, buildType, buildFlags, geometries, maxPrimitiveCounts,
        extendedInfo, std::move(allocator))
{}

GenericAccelerationStructure::GenericAccelerationStructure(std::shared_ptr<Device> device, VkAccelerationStructureCreateFlagsKHR createFlags,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    std::shared_ptr<Allocator> allocator /* nullptr */,
    const StructureChain& extendedInfo /* default */):
    AccelerationStructure(std::move(device), VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR,
        createFlags, buildType, buildFlags, geometries, maxPrimitiveCounts,
        extendedInfo, std::move(allocator))
{}
#endif // VK_KHR_acceleration_structure
} // namespace magma
