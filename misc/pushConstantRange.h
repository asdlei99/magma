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
    /* Push constant range that is used by pipeline layout. */

    struct PushConstantRange : VkPushConstantRange
    {
        constexpr PushConstantRange(VkShaderStageFlags stageFlags,
            uint32_t offset,
            std::size_t size) noexcept;
        constexpr hash_t hash() const noexcept;
    };
} // namespace magma

#include "pushConstantRange.inl"

namespace magma
{
    namespace pushconstant
    {
        /* Defines stage flags describing the shader stages that will access a range of push constants.
           If a particular stage is not included in the range, then accessing members of that range of
           push constants from the corresponding shader stage will return undefined values. */

        template<typename Type>
        struct PushConstantRange : magma::PushConstantRange
        {
            constexpr PushConstantRange(const VkShaderStageFlags flags, const uint32_t offset = 0) noexcept:
                magma::PushConstantRange(flags, offset, sizeof(Type)) {}
        };

        template<typename Type>
        struct VertexConstantRange : PushConstantRange<Type>
        {
            constexpr VertexConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_VERTEX_BIT, offset) {}
        };

        template<typename Type>
        struct TesselationControlConstantRange : PushConstantRange<Type>
        {
            constexpr TesselationControlConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, offset) {}
        };

        template<typename Type>
        struct TesselationEvaluationConstantRange : PushConstantRange<Type>
        {
            constexpr TesselationEvaluationConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, offset) {}
        };

        template<typename Type>
        struct GeometryConstantRange : PushConstantRange<Type>
        {
            constexpr GeometryConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_GEOMETRY_BIT, offset) {}
        };

        template<typename Type>
        struct FragmentConstantRange : PushConstantRange<Type>
        {
            constexpr FragmentConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_FRAGMENT_BIT, offset) {}
        };

        template<typename Type>
        struct ComputeConstantRange : PushConstantRange<Type>
        {
            constexpr ComputeConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_COMPUTE_BIT, offset) {}
        };

        template<typename Type>
        struct GraphicsConstantRange : PushConstantRange<Type>
        {
            constexpr GraphicsConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_ALL_GRAPHICS, offset) {}
        };

        template<typename Type>
        struct AllConstantRange : PushConstantRange<Type>
        {
            constexpr AllConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_ALL, offset) {}
        };

        template<typename Type>
        struct VertexGeometryConstantRange : PushConstantRange<Type>
        {
            constexpr VertexGeometryConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT, offset) {}
        };

        template<typename Type>
        struct VertexFragmentConstantRange : PushConstantRange<Type>
        {
            constexpr VertexFragmentConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset) {}
        };

    #ifdef VK_EXT_mesh_shader
        template<typename Type>
        struct TaskConstantRange : PushConstantRange<Type>
        {
            constexpr TaskConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_TASK_BIT_EXT, offset) {}
        };

        template<typename Type>
        struct MeshConstantRange : PushConstantRange<Type>
        {
            constexpr MeshConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_MESH_BIT_EXT, offset) {}
        };

        template<typename Type>
        struct TaskMeshConstantRange : PushConstantRange<Type>
        {
            constexpr TaskMeshConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, offset) {}
        };

        template<typename Type>
        struct TaskMeshFragmentConstantRange : PushConstantRange<Type>
        {
            constexpr TaskMeshFragmentConstantRange(const uint32_t offset = 0) noexcept:
                PushConstantRange<Type>(VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT, offset) {}
        };
    #endif // VK_EXT_mesh_shader
    } // namespace pushconstant
} // namespace magma
