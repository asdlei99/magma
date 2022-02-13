#pragma once
#include <cstdint>
#include <cmath>

#ifdef MAGMA_SSE
#include <xmmintrin.h>
#include <smmintrin.h>
#else
#include <algorithm>
#endif

namespace magma
{
    namespace packed
    {
        /* Base packed vector type that provides underlying VkFormat. */

        template<VkFormat type>
        struct Vector
        {
            static constexpr VkFormat format = type;
        };

        /* Specifies a three-component, 16-bit packed unsigned normalized format that has
           a 5-bit Z component in bits 11..15, a 6-bit Y component in bits 5..10,
           and a 5-bit X component in bits 0..4.
           Corresponding format is VK_FORMAT_B5G6R5_UNORM_PACK16. */

        struct X5y6z5unorm : Vector<VK_FORMAT_B5G6R5_UNORM_PACK16>
        {
            union
            {
                struct layout
                {
                    uint16_t x: 5; // [0,31]
                    uint16_t y: 6; // [0,63]
                    uint16_t z: 5; // [0,31]
                };
                uint16_t v;
            };

            X5y6z5unorm() noexcept = default;
            explicit X5y6z5unorm(float x, float y, float z) noexcept;
            explicit X5y6z5unorm(const float v[3]) noexcept:
                X5y6z5unorm(v[0], v[1], v[2]) {}
            explicit X5y6z5unorm(uint16_t v) noexcept:
                v(v) {}
        };

        /* Specifies a four-component, 32-bit packed unsigned normalized format that has
           a 2-bit W component in bits 30..31, a 10-bit Z component in bits 20..29,
           a 10-bit Y component in bits 10..19, and a 10-bit X component in bits 0..9.
           Corresponding format is VK_FORMAT_A2B10G10R10_UNORM_PACK32. */

        struct X10y10z10w2unorm : Vector<VK_FORMAT_A2B10G10R10_UNORM_PACK32>
        {
            union
            {
                struct layout
                {
                    uint32_t x: 10; // [0,1023]
                    uint32_t y: 10; // [0,1023]
                    uint32_t z: 10; // [0,1023]
                    uint32_t w: 2;  // [0,3]
                };
                uint32_t v;
            };

            X10y10z10w2unorm() noexcept = default;
            explicit X10y10z10w2unorm(float x, float y, float z, int w = 0) noexcept;
            explicit X10y10z10w2unorm(const float v[3], int w = 0) noexcept:
                X10y10z10w2unorm(v[0], v[1], v[2], w) {}
            explicit X10y10z10w2unorm(uint32_t v) noexcept:
                v(v) {}
        };

        /* Specifies a four-component, 32-bit packed signed normalized format that has
           a 2-bit W component in bits 30..31, a 10-bit Z component in bits 20..29,
           a 10-bit Y component in bits 10..19, and a 10-bit X component in bits 0..9.
           Corresponding format is VK_FORMAT_A2B10G10R10_SNORM_PACK32. */

        struct X10y10z10w2snorm : Vector<VK_FORMAT_A2B10G10R10_SNORM_PACK32>
        {
            union
            {
                struct layout
                {
                    int32_t x: 10; // [-511,511]
                    int32_t y: 10; // [-511,511]
                    int32_t z: 10; // [-511,511]
                    uint32_t w: 2; // [0,3]
                };
                uint32_t v;
            };

            X10y10z10w2snorm() noexcept = default;
            explicit X10y10z10w2snorm(float x, float y, float z, int w = 0) noexcept;
            explicit X10y10z10w2snorm(const float v[3], int w = 0) noexcept:
                X10y10z10w2snorm(v[0], v[1], v[2], w) {}
            explicit X10y10z10w2snorm(uint32_t v) noexcept:
                v(v) {}
        };

        /* Specifies a three-component, 32-bit packed unsigned floating-point format that has
           a 10-bit Z component in bits 22..31, an 11-bit Y component in bits 11..21, an 11-bit X component in bits 0..10.
           An unsigned 11-bit floating-point number has no sign bit, a 5-bit exponent, and a 6-bit mantissa.
           An unsigned 10-bit floating-point number has no sign bit, a 5-bit exponent, and a 5-bit mantissa.
           Unsigned 10- and 11-bit floating point numbers are defined here:
           https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#11bitfp
           https://www.khronos.org/registry/DataFormat/specs/1.3/dataformat.1.3.html#10bitfp
           Corresponding format is VK_FORMAT_B10G11R11_UFLOAT_PACK32. */

        struct X11y11z10ufloat : Vector<VK_FORMAT_B10G11R11_UFLOAT_PACK32>
        {
            union
            {
                struct layout
                {
                    uint32_t xm : 6; // x-mantissa
                    uint32_t xe : 5; // x-exponent
                    uint32_t ym : 6; // y-mantissa
                    uint32_t ye : 5; // y-exponent
                    uint32_t zm : 5; // z-mantissa
                    uint32_t ze : 5; // z-exponent
                };
                uint32_t v;
            };

            X11y11z10ufloat() noexcept = default;
            explicit X11y11z10ufloat(float x, float y, float z) noexcept;
            explicit X11y11z10ufloat(const float v[3]) noexcept:
                X11y11z10ufloat(v[0], v[1], v[2]) {}
            explicit X11y11z10ufloat(uint32_t v) noexcept:
                v(v) {}
        };
    } // namespace packed
} // namespace magma

#include "x5y6z5unorm.inl"
#include "x10y10z10w2unorm.inl"
#include "x10y10z10w2snorm.inl"
#include "x11y11z10ufloat.inl"
