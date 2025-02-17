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

#if defined(__LP64__) ||\
    defined(_WIN64) ||\
    (defined(__x86_64__) && !defined(__ILP32__) ) ||\
    defined(_M_X64) ||\
    defined(__ia64) ||\
    defined (_M_IA64) ||\
    defined(__aarch64__) ||\
    defined(__powerpc64__)
  #define MAGMA_X64
#endif

#if defined(_MSC_VER)
  #if _HAS_CXX17
    #define MAGMA_CXX17
  #endif
#elif defined(__MINGW32__) || defined(__GNUC__)
  #if __cplusplus >= 201703L
    #define MAGMA_CXX17
  #endif
#endif

#if defined(_DEBUG) || defined(DEBUG)
    #define MAGMA_DEBUG
#endif

#ifdef _DEBUG
  #ifdef VK_EXT_debug_utils
    #define MAGMA_DEBUG_LABEL
  #endif
#endif

#define MAGMA_ALIGNMENT 16

// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryPriorityAllocateInfoEXT.html
#define MAGMA_DEFAULT_MEMORY_PRIORITY 0.5f

#define MAGMA_MAX_STRING 4096

#define MAGMA_BAD_CODE 0xBAADC0DE

#define MAGMA_EXTENSION_PREFIX "VK_"
#define MAGMA_LAYER_PREFIX "VK_LAYER_"

#if defined(_M_AMD64) || defined(__x86_64__)
  #define MAGMA_XMM_REGISTERS 16
#else
  #define MAGMA_XMM_REGISTERS 8
#endif
#define MAGMA_XMM_BLOCK_SIZE (sizeof(__m128i) * MAGMA_XMM_REGISTERS)
#define MAGMA_CONCURRENT_COPY_THREADS 4
#define MAGMA_COPY_PAGE_SIZE (MAGMA_XMM_BLOCK_SIZE * MAGMA_CONCURRENT_COPY_THREADS)

#ifdef MAGMA_SSE
  #define mm_permute_ps(v, c) _mm_shuffle_ps((v), (v), c)
#endif

namespace magma
{
#if (VK_USE_64_BIT_PTR_DEFINES == 1)
typedef void *NonDispatchableHandle;
#else
typedef uint64_t NonDispatchableHandle;
#endif

/* Base class with virtual destructor. */

class IDestructible
{
public:
    virtual ~IDestructible() = default;
};

/* Prevents objects of a class from being copy-
   constructed or assigned to each other. */

class NonCopyable
{
    NonCopyable(const NonCopyable&) = delete;
    const NonCopyable& operator=(const NonCopyable&) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

namespace core
{
template<class Type>
inline typename Type::NativeHandle dereference(const std::shared_ptr<Type>& ptr) noexcept
{
    if (ptr)
        return *ptr;
#ifdef VK_NULL_HANDLE
    return VK_NULL_HANDLE;
#else
    return 0ull;
#endif
}

template<class VkObject>
inline VkObject reinterpret(NonDispatchableHandle handle) noexcept
{
    if (handle)
    {
    #if (VK_USE_64_BIT_PTR_DEFINES == 1)
        return reinterpret_cast<VkObject>(handle);
    #else
        return static_cast<VkObject>(handle);
    #endif
    }
#ifdef VK_NULL_HANDLE
    return VK_NULL_HANDLE;
#else
    return 0ull;
#endif
}

template<class Int>
inline Int roundUp(Int value, Int multiple) noexcept
{
    if (Int(0) == multiple)
        return Int(0);
    if (Int(1) == multiple)
        return value;
    return ((value + multiple - Int(1)) / multiple) * multiple;
}
} // namespace core
} // namespace magma

#include "macros.h"
#include "hash.h"
#include "memory.h"
