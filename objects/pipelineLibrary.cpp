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
#include "pch.h"
#pragma hdrstop
#include "pipelineLibrary.h"
#include "pipeline.h"

namespace magma
{
#ifdef VK_KHR_pipeline_library
PipelineLibrary::PipelineLibrary(std::vector<std::shared_ptr<Pipeline>>& libraries,
    const StructureChain& extendedInfo /* default */):
    libraries(libraries)
{
    dereferencedLibraries.reserve(libraries.size());
    for (auto const& pipeline: libraries)
        dereferencedLibraries.push_back(*pipeline);
    pipelineLibraryInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
    pipelineLibraryInfo.pNext = extendedInfo.chainNodes();
    pipelineLibraryInfo.libraryCount = MAGMA_COUNT(libraries);
    pipelineLibraryInfo.pLibraries = dereferencedLibraries.data();
#ifdef VK_KHR_ray_tracing_pipeline
    rayTracingPipelineInterfaceInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
    rayTracingPipelineInterfaceInfo.pNext = nullptr;
    rayTracingPipelineInterfaceInfo.maxPipelineRayPayloadSize = 0;
    rayTracingPipelineInterfaceInfo.maxPipelineRayHitAttributeSize = 0;
#endif // VK_KHR_ray_tracing_pipeline
}

#ifdef VK_KHR_ray_tracing_pipeline
void PipelineLibrary::setMaxPipelineRayPayloadSize(uint32_t size) noexcept
{
    rayTracingPipelineInterfaceInfo.maxPipelineRayPayloadSize = size;
}

void PipelineLibrary::setMaxPipelineRayHitAttributeSize(uint32_t size) noexcept
{
    rayTracingPipelineInterfaceInfo.maxPipelineRayHitAttributeSize = size;
}
#endif // VK_KHR_ray_tracing_pipeline
#endif // VK_KHR_pipeline_library
} // namespace magma
