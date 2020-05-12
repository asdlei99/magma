/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2020 Victor Coda.

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
#ifdef _MSC_VER
#include <malloc.h>
#else
#include <mm_malloc.h>
#endif
#include <memory>
#include <cassert>

#include "dereference.h"

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

#ifdef _DEBUG
#define MAGMA_DEBUG
#ifdef VK_EXT_debug_utils
#define MAGMA_DEBUG_LABEL
#endif
#endif

#ifdef MAGMA_DEBUG
#ifndef MAGMA_ASSERT
#define MAGMA_ASSERT(condition) assert(condition)
#endif
#else
#define MAGMA_ASSERT(condition)
#endif // !MAGMA_DEBUG

#ifdef _MSC_VER
#define MAGMA_CONSTEXPR constexpr
#else
#define MAGMA_CONSTEXPR
#endif

#define MAGMA_UNUSED(variable) variable
#define MAGMA_BOOLEAN(condition) (condition) ? VK_TRUE : VK_FALSE
#define MAGMA_COUNT(container) static_cast<uint32_t>(container.size())

#define MAGMA_ALIGNMENT 16
#define MAGMA_ALIGN(size) (((size) + 0xF) & ~(0xF))
#define MAGMA_ALIGNED(p) (((uintptr_t)(const void *)(p)) % (MAGMA_ALIGNMENT) == 0)

#if defined(_M_AMD64) || defined(__x86_64__)
#define MAGMA_XMM_REGISTERS 16
#else
#define MAGMA_XMM_REGISTERS 8
#endif
#define MAGMA_XMM_BLOCK_SIZE (sizeof(__m128i) * MAGMA_XMM_REGISTERS)

#define MAGMA_CONCURRENT_COPY_THREADS 4
#define MAGMA_COPY_PAGE_SIZE (MAGMA_XMM_BLOCK_SIZE * MAGMA_CONCURRENT_COPY_THREADS)

#ifdef _MSC_VER
#define MAGMA_MALLOC(size) _mm_malloc(size, MAGMA_ALIGNMENT)
#define MAGMA_FREE(p) _mm_free(p)
#define MAGMA_ALLOCA(size) _malloca(size)
#define MAGMA_FREEA(p) _freea(p)
#else
#define MAGMA_MALLOC(size) malloc(size)
#define MAGMA_FREE(p) free(p)
#define MAGMA_ALLOCA(size) alloca(size)
#define MAGMA_FREEA(p)
#endif // !_MSC_VER

#define MAGMA_MAX_STRING 4096

#define MAGMA_STRINGIZE(name) #name
#define MAGMA_STRINGIZE_FIELD(field) case field: return MAGMA_STRINGIZE(field); break
#define MAGMA_DEFAULT_UNKNOWN default: return "<unknown>"

#define MAGMA_HANDLE(obj) *(this->obj)
#define MAGMA_OPTIONAL_HANDLE(obj) core::dereference(obj)
#define MAGMA_OPTIONAL_INSTANCE(obj) this->obj ? this->obj.get() : nullptr

#define MAGMA_SUCCEEDED(result)\
    ((VK_SUCCESS == result) ||\
     (VK_INCOMPLETE == result))

#define MAGMA_PRESENT_SUCCEEDED(result)\
    ((VK_SUCCESS == result) ||\
     (VK_SUBOPTIMAL_KHR == result))

#define MAGMA_STENCIL_FACE_MASK(frontFace, backFace)\
    ((frontFace && backFace) ? VK_STENCIL_FRONT_AND_BACK :\
     (frontFace ? VK_STENCIL_FACE_FRONT_BIT : VK_STENCIL_FACE_BACK_BIT))

#define MAGMA_SPECIALIZE_VERTEX_ATTRIBUTE(type, normalized, format)\
    template<> struct VertexAttribute<type, normalized> : AttributeFormat<format> {}

#define MAGMA_REGISTER_RESOURCE(Type, self) self->getDevice()->getResourcePool()->getPool<Type>().registerResource(self)
#define MAGMA_UNREGISTER_RESOURCE(Type, self) self->getDevice()->getResourcePool()->getPool<Type>().unregisterResource(self)
