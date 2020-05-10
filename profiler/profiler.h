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

        /* Base profiler object that is supposed to exists
           within a scope of block or function. */

#ifdef MAGMA_ENABLE_PROFILING
        class ScopedProfiler : public Profiler
        {
        protected:
            ScopedProfiler(const char *functionName) noexcept;
            clock::duration getDuration() const noexcept;

            const char *const functionName;
            const clock::time_point start;
        };

        /* 1. Vulkan function profiler.
              If function belongs to debugging functionality (e.g. vkSubmitDebugUtilsMessageEXT),
              then <debugEntry> should be true. This allows to exclude debugging functions
              from profiling if this isn't neccessary. */

        class ScopedEntryProfiler : public ScopedProfiler
        {
        public:
            ScopedEntryProfiler(const char *entryName, bool debugEntry) noexcept;
            ~ScopedEntryProfiler();

        private:
            const bool debugEntry;
        };

        /* 2. Method profiler.
              Supposed to be used within methods of objects that are inherited
              from base Object class and has associated Vulkan object type. */

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

        /* 3. Function profiler.
              Supposed to be used within non-Object types and standalon functions. */

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
