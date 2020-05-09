#include "pch.h"
#include "profiler.h"

namespace magma
{
namespace profile
{
#ifdef MAGMA_ENABLE_PROFILING
std::shared_ptr<IProfiler> Profiler::profiler;
#endif

void Profiler::setInstance(std::shared_ptr<IProfiler> instance) noexcept
{
#ifdef MAGMA_ENABLE_PROFILING
    profiler = std::move(instance);
#endif
}

std::shared_ptr<IProfiler> Profiler::getInstance() noexcept
{
#ifdef MAGMA_ENABLE_PROFILING
    return profiler;
#else
    return nullptr;
#endif
}
} // namespace profile
} // namespace magma
