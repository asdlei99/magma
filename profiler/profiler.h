#pragma once
#include <chrono>
#include "descriptors.h"
#include "../core/noncopyable.h"

namespace magma
{
    namespace profile
    {
        /* Profiler interface. */

        class IProfiler : public core::NonCopyable
        {
        public:
            virtual void profileApiEntry(const char *entryName,
                uint32_t flags,
                const std::chrono::nanoseconds& duration) noexcept = 0;
            virtual void profileMethod(const char *methodName,
                VkObjectType objectType,
                const char *fileName,
                long line,
                const std::chrono::nanoseconds& duration) noexcept = 0;
            virtual void profileFunction(const char *functionName,
                const char *fileName,
                long line,
                const std::chrono::nanoseconds& duration) noexcept = 0;
        };

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
            ScopedSampler(const Type& desc) noexcept;
            ~ScopedSampler();

        private:
            const Type& desc;
            const std::chrono::high_resolution_clock::time_point start;
        };
    } // namespace profile
} // namespace magma

#include "scopedSampler.inl"

#ifdef MAGMA_ENABLE_PROFILING
#define MAGMA_PROFILE_ENTRY(apiEntry) profile::ScopedSampler<profile::ApiEntryDescription> apiEntry##Sampler_({MAGMA_STRINGIZE(apiEntry), profile::ScopeFlags::None})
#define MAGMA_PROFILE_QUEUE_ENTRY(apiQueueEntry) profile::ScopedSampler<profile::ApiEntryDescription> apiQueueEntry##Sampler_({MAGMA_STRINGIZE(apiQueueEntry), profile::ScopeFlags::Queue})
#define MAGMA_PROFILE_DEBUG_ENTRY(apiDebugEntry) profile::ScopedSampler<profile::ApiEntryDescription> apiDebugEntry##Sampler_({MAGMA_STRINGIZE(apiDebugEntry), profile::ScopeFlags::Debug})
#define MAGMA_PROFILE_METHOD profile::ScopedSampler<profile::MethodDescription> methodSampler_({this->getObjectType(), __FUNCTION__, __FILE__, __LINE__});
#define MAGMA_PROFILE_FUNCTION profile::ScopedSampler<profile::FunctionDescription> functionSampler_({__FUNCTION__, __FILE__, __LINE__});
#else
#define MAGMA_PROFILE_ENTRY(apiEntry)
#define MAGMA_PROFILE_DEBUG_ENTRY(apiEntry)
#define MAGMA_PROFILE_METHOD
#define MAGMA_PROFILE_FUNCTION
#endif // MAGMA_ENABLE_PROFILING
