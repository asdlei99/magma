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
#include "handle.h"

namespace magma
{
    /* Semaphores are a synchronization primitive that can be used
       to insert a dependency between batches submitted to queues.
       Semaphores have two states - signaled and unsignaled.
       The state of a semaphore can be signaled after execution of
       a batch of commands is completed. A batch can wait for a semaphore
       to become signaled before it begins execution, and the semaphore is
       also unsignaled before the batch begins execution. */

    class Semaphore : public NonDispatchable<VkSemaphore>
    {
    public:
        Semaphore(std::shared_ptr<Device> device,
            std::shared_ptr<IAllocator> allocator = nullptr);
        ~Semaphore();
    };
} // namespace magma
