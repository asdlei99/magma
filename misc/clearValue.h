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
#include "../shared.h"

namespace magma
{
    // C++ forbids inheritance from union.
    struct ClearValue
    {
        operator const VkClearValue&() const { return value; }

    protected:
        VkClearValue value;
    };  

    struct ColorClear : public ClearValue 
    {
        ColorClear(float r, float g, float b, float a = 1.f)
        {
            value.color.float32[0] = r;
            value.color.float32[1] = g;
            value.color.float32[2] = b;
            value.color.float32[3] = a;
        }

        ColorClear(const float color[4])
        {
            for (int i = 0; i < 4; ++i)
                value.color.float32[i] = color[i];
        }

        ColorClear(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
        {
            value.color.uint32[0] = r;
            value.color.uint32[1] = g;
            value.color.uint32[2] = b;
            value.color.uint32[3] = a;
        }

        ColorClear(const uint32_t color[4])
        {
            for (int i = 0; i < 4; ++i)
                value.color.uint32[i] = color[i];
        }
    };

    struct DepthStencilClear : public ClearValue
    {
        DepthStencilClear(float depth, uint8_t stencil = 0)
        {
            value.depthStencil.depth = depth;
            value.depthStencil.stencil = stencil;
        }
    };
} // namespace magma
