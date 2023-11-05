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

namespace magma
{
    class AccelerationStructure;
    class CommandBuffer;
    class SrcTransferBuffer;

    /* Single acceleration structure instance for building
       into an acceleration structure geometry. */

#ifdef VK_KHR_acceleration_structure
    class AccelerationStructureInstance : public VkAccelerationStructureInstanceKHR
    {
    public:
        AccelerationStructureInstance() noexcept;
        void setTransform(const VkTransformMatrixKHR& transform_) noexcept { transform = transform_; }
        void setInstanceCustomIndex(uint32_t customIndex) noexcept { instanceCustomIndex = customIndex; }
        void setVisibilityMask(uint8_t mask_) noexcept { mask = mask_; }
        void setInstanceShaderBindingTableRecordOffset(uint32_t offset) noexcept { instanceShaderBindingTableRecordOffset = offset; }
        void disableFaceCulling(bool disable) noexcept { setFlag(VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR, disable); }
        bool faceCullingDisabled() const noexcept { return flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; }
        void flipTriangleFacing(bool flip) noexcept { setFlag(VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR, flip); }
        bool triangleFacingFlipped() const noexcept { return flags & VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR; }
        void setForceOpaque(bool opaque) noexcept { setFlag(VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR, opaque); }
        bool getForceOpaque() const noexcept { return flags & VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR; }
        void setForceNoOpaque(bool noOpaque) noexcept { setFlag(VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR, noOpaque); }
        bool getForceNoOpaque() const noexcept { return flags & VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR; }
        void setAccelerationStructure(std::shared_ptr<const AccelerationStructure> accelerationStructure,
            bool hostOperations = false);

    private:
        void setFlag(VkGeometryInstanceFlagBitsKHR bit, bool set) noexcept { if (set) flags |= bit; else flags &= ~bit; }
    };
#endif // VK_NV_ray_tracing
} // namespace magma
