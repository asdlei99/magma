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
#include "accelerationStructureInstanceBuffer.h"
#include "accelerationStructure.h"
#include "deviceMemory.h"
#include "srcTransferBuffer.h"
#include "commandBuffer.h"
#include "../exceptions/errorResult.h"

namespace magma
{
#ifdef VK_KHR_acceleration_structure
AccelerationStructureInstance::AccelerationStructureInstance() noexcept:
    VkAccelerationStructureInstanceKHR{
        {   // transform
            {   // matrix[3][4]
                {1.f, 0.f, 0.f, 0.f},
                {0.f, 1.f, 0.f, 0.f},
                {0.f, 0.f, 1.f, 0.f}
            }
        },
        0, // instanceCustomIndex
        0xFF, // mask
        0, // instanceShaderBindingTableRecordOffset
        0, // flags
        0 // accelerationStructureReference
    }
{}

void AccelerationStructureInstance::setAccelerationStructure(std::shared_ptr<const AccelerationStructure> accelerationStructure,
    bool hostOperations /* false */ )
{
    accelerationStructureReference = hostOperations ? accelerationStructure->getHandle() : accelerationStructure->getDeviceAddress();
}
#endif // VK_KHR_acceleration_structure
} // namespace magma
