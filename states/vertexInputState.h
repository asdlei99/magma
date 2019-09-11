/*
Magma - C++1x interface over Khronos Vulkan API.
Copyright (C) 2018 Victor Coda.

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
    struct VertexInputBinding : VkVertexInputBindingDescription
    {
        constexpr VertexInputBinding(uint32_t binding,
            uint32_t stride,
            VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
    };

    /* Structure specifying vertex input attribute description. */

    struct VertexInputAttribute : VkVertexInputAttributeDescription
    {
        // Workaround to deduce constructor's <bool unsignedNorm> template parameter
        template<bool normalized> struct normalized {};
        VertexInputAttribute() = default;
        constexpr VertexInputAttribute(uint32_t location,
            uint32_t binding,
            VkFormat format,
            uint32_t offset);
        template<typename Vertex, typename Type, bool unsignedNorm = false>
        VertexInputAttribute(uint32_t location,
            Type Vertex::*attrib,
            normalized<unsignedNorm> norm = normalized<unsignedNorm>()) noexcept;
    };

    /* Applications specify vertex input attribute and vertex input binding descriptions
       as part of graphics pipeline creation. */

    class VertexInputState : public VkPipelineVertexInputStateCreateInfo
    {
    public:
        VertexInputState();
        VertexInputState(const VertexInputBinding& binding,
            const std::initializer_list<VertexInputAttribute>& attributes);
        VertexInputState(const std::initializer_list<VertexInputBinding>& bindings,
            const std::initializer_list<VertexInputAttribute>& attributes);
        virtual ~VertexInputState();
        virtual uint32_t stride(uint32_t binding) const = 0;
        std::size_t hash() const;
        bool operator==(const VertexInputState&) const noexcept;
    };
} // namespace magma

#include "vertexInputState.inl"
