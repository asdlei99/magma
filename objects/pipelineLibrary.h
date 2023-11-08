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
#include "../misc/structureChain.h"

namespace magma
{
    class Pipeline;

    /* A pipeline library is a special pipeline that cannot be bound,
       instead it defines a set of shaders and shader groups which
       can be linked into other pipelines. */

#ifdef VK_KHR_pipeline_library
    class PipelineLibrary : NonCopyable
    {
    public:
        explicit PipelineLibrary(std::vector<std::shared_ptr<Pipeline>>& libraries,
            const StructureChain& extendedInfo = StructureChain());
    #ifdef VK_KHR_ray_tracing_pipeline
        void setMaxPipelineRayPayloadSize(uint32_t size) noexcept;
        void setMaxPipelineRayHitAttributeSize(uint32_t size) noexcept;
        const VkRayTracingPipelineInterfaceCreateInfoKHR& getRayTracingLibraryInterface() const noexcept { return rayTracingPipelineInterfaceInfo; }
    #endif // VK_KHR_ray_tracing_pipeline
        const VkPipelineLibraryCreateInfoKHR& getLibraryInfo() const noexcept { return pipelineLibraryInfo; }

    private:
        VkPipelineLibraryCreateInfoKHR pipelineLibraryInfo;
    #ifdef VK_KHR_ray_tracing_pipeline
        VkRayTracingPipelineInterfaceCreateInfoKHR rayTracingPipelineInterfaceInfo;
    #endif
        std::vector<std::shared_ptr<Pipeline>> libraries;
        std::vector<VkPipeline> dereferencedLibraries;
    };
#endif // VK_KHR_pipeline_library
} // namespace magma
