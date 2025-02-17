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
#ifdef _MSC_VER
    #pragma once
#endif

// C lib
#include <cassert>
#include <cstring>
#include <cstdarg>
#ifdef _MSC_VER
    #include <malloc.h>
#else
    #include <mm_malloc.h>
#endif

// Containers
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <array>
#include <string>

// Smart pointers
#include <memory>

// Threading
#include <thread>
#include <future>
#include <atomic>
#include <mutex>

// Misc
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <limits>

// Input/output
#include <iostream>
#include <sstream>

// Lean & mean windows.h
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #define NOMINMAX
#endif // _WIN32

#include <vulkan/vulkan.h>

// SIMD intrinsics
#ifdef MAGMA_SSE
    #include <xmmintrin.h>
    #include <smmintrin.h>
#else
    #include <cmath>
#endif

// Internal code
#include "core.h"
