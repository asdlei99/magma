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
#include "rasterizationState.h"

#ifdef VK_EXT_transform_feedback
namespace magma
{
    /* The vertex stream used for rasterization is specified by this state.
       If the implementation is capable, a vertex stream other than zero can be rasterized.
       If this structure is not present, rasterizationStream is assumed to be zero. */

    struct StreamRasterizationState : RasterizationState
    {
        constexpr StreamRasterizationState(const RasterizationState& state,
            uint32_t rasterizationStream,
            VkPipelineRasterizationStateStreamCreateFlagsEXT flags = 0) noexcept;
        constexpr hash_t hash() const noexcept;
        constexpr bool operator==(const StreamRasterizationState&) const noexcept;

        const VkPipelineRasterizationStateStreamCreateInfoEXT stream;
    };
} // namespace magma

#include "streamRasterizationState.inl"

namespace magma
{
#define MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(polygonMode, Stream, rasterizationStream)\
    constexpr StreamRasterizationState polygonMode##CullNoneCCw##Stream(polygonMode##CullNoneCCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullFrontCCw##Stream(polygonMode##CullFrontCCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullBackCCw##Stream(polygonMode##CullBackCCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullFrontAndBackCCw##Stream(polygonMode##CullFrontAndBackCCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullNoneCw##Stream(polygonMode##CullNoneCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullFrontCw##Stream(polygonMode##CullFrontCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullBackCw##Stream(polygonMode##CullBackCw, rasterizationStream);\
    constexpr StreamRasterizationState polygonMode##CullFrontAndBackCw##Stream(polygonMode##CullFrontAndBackCw, rasterizationStream);

    namespace renderstate
    {
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fill, Zero, 0)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fill, One, 1)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fill, Two, 2)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fill, Three, 3)

        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(line, Zero, 0)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(line, One, 1)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(line, Two, 2)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(line, Three, 3)

        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(point, Zero, 0)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(point, One, 1)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(point, Two, 2)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(point, Three, 3)
    #ifdef VK_NV_fill_rectangle
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fillRectangle, Zero, 0)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fillRectangle, One, 1)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fillRectangle, Two, 2)
        MAGMA_STREAM_RASTERIZATION_STATE_PERMUTATIONS(fillRectangle, Three, 3)
    #endif
    } // namespace renderstate
} // namespace magma
#endif // VK_EXT_transform_feedback
