#pragma once
#include <chrono>
#include "descriptors.h"
#include "../core/noncopyable.h"

namespace magma
{
    namespace profile
    {
        class IProfiler;

        /* Profiler object that holds pointer to profiler instance. */

        class Profiler : public core::NonCopyable
        {
        public:
            static void setInstance(std::shared_ptr<IProfiler> instance) noexcept;
            static std::shared_ptr<IProfiler> getInstance() noexcept;

        protected:
#ifdef MAGMA_ENABLE_PROFILING
            static std::shared_ptr<IProfiler> profiler;
#endif
        };

        /* Profiling sampler that is supposed to exists within a scope of block or function. */

        template<typename Type>
        class ScopedSampler
        {
        public:
            ScopedSampler(const Type& description) noexcept;
            ~ScopedSampler();

        private:
            const Type& description;
            const std::chrono::high_resolution_clock::time_point start;
        };
    } // namespace profile
} // namespace magma

#include "scopedSampler.inl"

#ifdef MAGMA_ENABLE_PROFILING
#define MAGMA_PROFILE_ENTRY(apiEntry) profile::ScopedSampler<profile::ApiEntryDescription> apiEntry##Sampler_({MAGMA_STRINGIZE(apiEntry), false})
#define MAGMA_PROFILE_DEBUG_ENTRY(apiEntry) profile::ScopedSampler<profile::ApiEntryDescription> apiEntry##Sampler_({MAGMA_STRINGIZE(apiEntry), true})
#define MAGMA_PROFILE_METHOD profile::ScopedSampler<profile::MethodDescription> methodSampler_({this->getObjectType(), __FUNCTION__, __FILE__, __LINE__});
#define MAGMA_PROFILE_FUNCTION profile::ScopedSampler<profile::FunctionDescription> functionSampler_({__FUNCTION__, __FILE__, __LINE__});
#else
#define MAGMA_PROFILE_ENTRY(apiEntry)
#define MAGMA_PROFILE_DEBUG_ENTRY(apiEntry)
#define MAGMA_PROFILE_METHOD
#define MAGMA_PROFILE_FUNCTION
#endif // MAGMA_ENABLE_PROFILING
