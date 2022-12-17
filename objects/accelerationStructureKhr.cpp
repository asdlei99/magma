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
#include "deferredOperation.h"
#include "../allocator/allocator.h"
#include "../misc/extProcAddress.h"
#include "../exceptions/errorResult.h"

namespace magma
{
#ifdef VK_KHR_acceleration_structure
AccelerationStructure::AccelerationStructure(std::shared_ptr<Device> device, VkAccelerationStructureTypeKHR structureType,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    VkAccelerationStructureCreateFlagsKHR flags, VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    const StructureChain& extendedInfo, std::shared_ptr<Allocator> allocator):
    NonDispatchableResource(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV, device, Sharing(), allocator),
    structureType(structureType),
    flags(flags),
    buildType(buildType),
    buildFlags(buildFlags),
    accelerationStructureSize(0),
    updateScratchSize(0),
    buildScratchSize(0)
{
    VkAccelerationStructureCreateInfoKHR accelerationStructureInfo;
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo;
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureInfo.pNext = extendedInfo.getChainedNodes();
    accelerationStructureInfo.createFlags = flags;
    accelerationStructureInfo.offset = 0;
    accelerationStructureInfo.type = structureType;
    accelerationStructureInfo.deviceAddress = 0;
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    accelerationStructureBuildSizesInfo.pNext = nullptr;
    accelerationStructureBuildSizesInfo.accelerationStructureSize = 0;
    accelerationStructureBuildSizesInfo.updateScratchSize = 0;
    accelerationStructureBuildSizesInfo.buildScratchSize = 0;
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
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkGetAccelerationStructureBuildSizesKHR, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    vkGetAccelerationStructureBuildSizesKHR(MAGMA_HANDLE(device), buildType, &accelerationStructureBuildGeometryInfo, maxPrimitiveCounts.data(), &accelerationStructureBuildSizesInfo);
    buffer = std::make_shared<AccelerationStructureBuffer>(std::move(device), accelerationStructureBuildSizesInfo.accelerationStructureSize, std::move(allocator));
    accelerationStructureInfo.buffer = *buffer;
    accelerationStructureInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    MAGMA_REQUIRED_DEVICE_EXTENSION(vkCreateAccelerationStructureKHR, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    const VkResult result = vkCreateAccelerationStructureKHR(MAGMA_HANDLE(device), &accelerationStructureInfo, MAGMA_OPTIONAL_INSTANCE(hostAllocator), &handle);
    MAGMA_THROW_FAILURE(result, "failed to create acceleration structure");
    accelerationStructureSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    updateScratchSize = accelerationStructureBuildSizesInfo.updateScratchSize;
    buildScratchSize = accelerationStructureBuildSizesInfo.buildScratchSize;
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

bool AccelerationStructure::hostBuild() const noexcept
{
    return (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR == buildType) ||
        (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR == buildType);
}

bool AccelerationStructure::deviceBuild() const noexcept
{
    return (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR == buildType) ||
        (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR == buildType);
}

bool AccelerationStructure::build(const std::vector<AccelerationStructureGeometry>& geometries,
    const std::vector<AccelerationStructureBuildRange>& buildRanges,
    std::shared_ptr<Buffer> scratchBuffer,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) noexcept
{
    if (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR == buildType)
        return false;
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = nullptr;
    accelerationStructureBuildGeometryInfo.type = structureType;
    accelerationStructureBuildGeometryInfo.flags = buildFlags;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = handle;
    accelerationStructureBuildGeometryInfo.geometryCount = MAGMA_COUNT(geometries);
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();
    accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();
    const VkAccelerationStructureBuildRangeInfoKHR *buildRangeInfos[1] = {buildRanges.data()};
    MAGMA_DEVICE_EXTENSION(vkBuildAccelerationStructuresKHR);
    const VkResult result = vkBuildAccelerationStructuresKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation),
        1, &accelerationStructureBuildGeometryInfo, buildRangeInfos);
    return MAGMA_SUCCEEDED(result);
}

bool AccelerationStructure::update(const std::vector<AccelerationStructureGeometry>& geometries,
    const std::vector<AccelerationStructureBuildRange>& buildRanges,
    std::shared_ptr<Buffer> scratchBuffer,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) noexcept
{
    if (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR == buildType)
        return false;
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo;
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.pNext = nullptr;
    accelerationStructureBuildGeometryInfo.type = structureType;
    accelerationStructureBuildGeometryInfo.flags = buildFlags;
    accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    accelerationStructureBuildGeometryInfo.srcAccelerationStructure = handle; // Update
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = handle; // in-place
    accelerationStructureBuildGeometryInfo.geometryCount = MAGMA_COUNT(geometries);
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();
    accelerationStructureBuildGeometryInfo.ppGeometries = nullptr;
    accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer->getDeviceAddress();
    const VkAccelerationStructureBuildRangeInfoKHR *buildRangeInfos[1] = {buildRanges.data()};
    MAGMA_DEVICE_EXTENSION(vkBuildAccelerationStructuresKHR);
    const VkResult result = vkBuildAccelerationStructuresKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation),
        1, &accelerationStructureBuildGeometryInfo, buildRangeInfos);
    return MAGMA_SUCCEEDED(result);
}

bool AccelerationStructure::copy(std::shared_ptr<AccelerationStructure> accelerationStructure,
    VkCopyAccelerationStructureModeKHR mode,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) const noexcept
{
    VkCopyAccelerationStructureInfoKHR copyAccelerationStructureInfo;
    copyAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
    copyAccelerationStructureInfo.pNext = nullptr;
    copyAccelerationStructureInfo.src = handle;
    copyAccelerationStructureInfo.dst = accelerationStructure->handle;
    copyAccelerationStructureInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCopyAccelerationStructureKHR);
    const VkResult result = vkCopyAccelerationStructureKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation), &copyAccelerationStructureInfo);
    return MAGMA_SUCCEEDED(result);
}

bool AccelerationStructure::copyToMemory(std::shared_ptr<Buffer> buffer, VkCopyAccelerationStructureModeKHR mode,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) const noexcept
{
    VkCopyAccelerationStructureToMemoryInfoKHR copyAccelerationStructureToMemoryInfo;
    copyAccelerationStructureToMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
    copyAccelerationStructureToMemoryInfo.pNext = nullptr;
    copyAccelerationStructureToMemoryInfo.src = handle;
    copyAccelerationStructureToMemoryInfo.dst.deviceAddress = buffer->getDeviceAddress();
    copyAccelerationStructureToMemoryInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCopyAccelerationStructureToMemoryKHR);
    const VkResult result = vkCopyAccelerationStructureToMemoryKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation), &copyAccelerationStructureToMemoryInfo);
    return MAGMA_SUCCEEDED(result);
}

bool AccelerationStructure::copyToMemory(void *buffer, VkCopyAccelerationStructureModeKHR mode,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) const noexcept
{
    VkCopyAccelerationStructureToMemoryInfoKHR copyAccelerationStructureToMemoryInfo;
    copyAccelerationStructureToMemoryInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR;
    copyAccelerationStructureToMemoryInfo.pNext = nullptr;
    copyAccelerationStructureToMemoryInfo.src = handle;
    copyAccelerationStructureToMemoryInfo.dst.hostAddress = buffer;
    copyAccelerationStructureToMemoryInfo.mode = mode;
    MAGMA_DEVICE_EXTENSION(vkCopyAccelerationStructureToMemoryKHR);
    const VkResult result = vkCopyAccelerationStructureToMemoryKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation), &copyAccelerationStructureToMemoryInfo);
    return MAGMA_SUCCEEDED(result);
}

bool AccelerationStructure::copyFromMemory(std::shared_ptr<const Buffer> buffer, VkCopyAccelerationStructureModeKHR mode,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) noexcept
{
    VkCopyMemoryToAccelerationStructureInfoKHR copyMemoryToAccelerationStructureInfo;
    copyMemoryToAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
    copyMemoryToAccelerationStructureInfo.pNext = nullptr;
    copyMemoryToAccelerationStructureInfo.src.deviceAddress = buffer->getDeviceAddress();
    copyMemoryToAccelerationStructureInfo.dst = handle;
    copyMemoryToAccelerationStructureInfo.mode = mode;
    const VkResult result = vkCopyMemoryToAccelerationStructureKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation), &copyMemoryToAccelerationStructureInfo);
    return MAGMA_SUCCEEDED(result);
}

bool AccelerationStructure::copyFromMemory(const void *buffer, VkCopyAccelerationStructureModeKHR mode,
    std::shared_ptr<DeferredOperation> deferredOperation /* nullptr */) noexcept
{
    VkCopyMemoryToAccelerationStructureInfoKHR copyMemoryToAccelerationStructureInfo;
    copyMemoryToAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR;
    copyMemoryToAccelerationStructureInfo.pNext = nullptr;
    copyMemoryToAccelerationStructureInfo.src.hostAddress = buffer;
    copyMemoryToAccelerationStructureInfo.dst = handle;
    copyMemoryToAccelerationStructureInfo.mode = mode;
    const VkResult result = vkCopyMemoryToAccelerationStructureKHR(MAGMA_HANDLE(device), MAGMA_OPTIONAL_HANDLE(deferredOperation), &copyMemoryToAccelerationStructureInfo);
    return MAGMA_SUCCEEDED(result);
}

TopLevelAccelerationStructure::TopLevelAccelerationStructure(std::shared_ptr<Device> device,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    std::shared_ptr<Allocator> allocator /* nullptr */,
    VkAccelerationStructureCreateFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */):
    AccelerationStructure(std::move(device), VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        geometries, maxPrimitiveCounts, flags, buildType, buildFlags, extendedInfo, std::move(allocator))
{}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(std::shared_ptr<Device> device,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    std::shared_ptr<Allocator> allocator /* nullptr */,
    VkAccelerationStructureCreateFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */):
    AccelerationStructure(std::move(device), VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        geometries, maxPrimitiveCounts, flags, buildType, buildFlags, extendedInfo, std::move(allocator))
{}

GenericAccelerationStructure::GenericAccelerationStructure(std::shared_ptr<Device> device,
    const std::vector<AccelerationStructureGeometry>& geometries, const std::vector<uint32_t>& maxPrimitiveCounts,
    VkAccelerationStructureBuildTypeKHR buildType, VkBuildAccelerationStructureFlagsKHR buildFlags,
    std::shared_ptr<Allocator> allocator /* nullptr */,
    VkAccelerationStructureCreateFlagsKHR flags /* 0 */,
    const StructureChain& extendedInfo /* default */):
    AccelerationStructure(std::move(device), VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR,
        geometries, maxPrimitiveCounts, flags, buildType, buildFlags, extendedInfo, std::move(allocator))
{}
#endif // VK_KHR_acceleration_structure
} // namespace magma
