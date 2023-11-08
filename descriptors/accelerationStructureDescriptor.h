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
#include "descriptor.h"

namespace magma
{
#ifdef VK_KHR_acceleration_structure
    class AccelerationStructure;
#endif

    namespace descriptor
    {
        /* An acceleration structure is a descriptor type that is
           used to retrieve scene geometry from within shaders that
           are used for ray traversal. Shaders have read-only access
           to the memory. */

    #ifdef VK_KHR_acceleration_structure
        class AccelerationStructure : public Descriptor<VkWriteDescriptorSetAccelerationStructureKHR>
        {
        public:
            AccelerationStructure(uint32_t binding) noexcept;
            bool associatedWithResource() const noexcept override;
            void write(VkDescriptorSet dstSet,
                VkWriteDescriptorSet& writeDescriptorSet) const noexcept override;
            AccelerationStructure& operator=(std::shared_ptr<const magma::AccelerationStructure>) noexcept;

        private:
            VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        };
    #endif // VK_KHR_acceleration_structure
    } // namespace descriptor
} // namespace magma
