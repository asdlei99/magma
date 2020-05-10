#pragma once
#include <chrono>
#include "../core/noncopyable.h"
#include "../objects/objectType.h"

namespace magma
{
    namespace profile
    {
        class IProfiler;

        class Profiler
        {
        public:
            typedef std::chrono::high_resolution_clock clock;

        public:
            static void setInstance(std::shared_ptr<IProfiler> instance) noexcept;
            static std::shared_ptr<IProfiler> getInstance() noexcept;

        protected:
#ifdef MAGMA_ENABLE_PROFILING
            static std::shared_ptr<IProfiler> profiler;
#endif
        };

#ifdef MAGMA_ENABLE_PROFILING
        class ScopedProfiler : public Profiler
        {
        protected:
            ScopedProfiler(const char *functionName) noexcept;
            clock::duration getDuration() const noexcept;

            const char *const functionName;
            const clock::time_point start;
        };

        class ScopedEntryProfiler : public ScopedProfiler
        {
        public:
            ScopedEntryProfiler(const char *entryName, bool debugEntry) noexcept;
            ~ScopedEntryProfiler();

        private:
            const bool debugEntry;
        };

        class ScopedMethodProfiler : public ScopedProfiler
        {
        public:
            ScopedMethodProfiler(VkObjectType objectType, const char *methodName,
                const char *fileName, long line) noexcept;
            ~ScopedMethodProfiler();

        private:
            VkObjectType objectType;
            const char *const fileName;
            const long line;
        };

        class ScopedFunctionProfiler : public ScopedProfiler
        {
        public:
            ScopedFunctionProfiler(const char *functionName, const char *fileName, long line) noexcept;
            ~ScopedFunctionProfiler();

        private:
            const char *const fileName;
            const long line;
        };
#endif // MAGMA_ENABLE_PROFILING
    } // namespace profile
} // namespace magma

#include "scopedProfiler.inl"

#ifdef MAGMA_ENABLE_PROFILING
#define MAGMA_PROFILE_ENTRY(apiEntry) magma::profile::ScopedEntryProfiler entryProfiler(MAGMA_STRINGIZE(apiEntry), false)
#define MAGMA_PROFILE_DEBUG_ENTRY(apiEntry) magma::profile::ScopedEntryProfiler debugEntryProfiler(MAGMA_STRINGIZE(apiEntry), true)
#define MAGMA_PROFILE_METHOD magma::profile::ScopedMethodProfiler methodProfiler(this->getObjectType(), __FUNCTION__, __FILE__, __LINE__);
#define MAGMA_PROFILE_FUNCTION magma::profile::ScopedFunctionProfiler functionProfiler(__FUNCTION__, __FILE__, __LINE__);
#else
#define MAGMA_PROFILE_ENTRY(apiEntry)
#define MAGMA_PROFILE_DEBUG_ENTRY(apiEntry)
#define MAGMA_PROFILE_METHOD
#define MAGMA_PROFILE_FUNCTION
#endif // MAGMA_ENABLE_PROFILING
