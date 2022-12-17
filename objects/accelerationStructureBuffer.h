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
#include "buffer.h"

namespace magma
{
    /* Buffer that is suitable as storage space for an acceleration structure. */

#ifdef VK_KHR_acceleration_structure
    class AccelerationStructureBuffer : public Buffer
    {
    public:
        explicit AccelerationStructureBuffer(std::shared_ptr<Device> device,
            VkDeviceSize size,
            std::shared_ptr<Allocator> allocator = nullptr,
            const Descriptor& optional = Descriptor(),
            const Sharing& sharing = Sharing());
    };
#endif // VK_KHR_acceleration_structure
} // namespace magma
