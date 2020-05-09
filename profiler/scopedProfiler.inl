namespace magma
{
namespace profile
{
inline ScopedProfiler::ScopedProfiler(const char *functionName) noexcept:
    functionName(functionName), start(clock::now())
{}

inline ScopedProfiler::clock::duration ScopedProfiler::getDuration() const noexcept
{
    return clock::now() - start;
}

inline ScopedEntryProfiler::ScopedEntryProfiler(const char *entryName, bool debugEntry) noexcept:
    ScopedProfiler(entryName), debugEntry(debugEntry)
{}

inline ScopedEntryProfiler::~ScopedEntryProfiler()
{}

inline ScopedMethodProfiler::ScopedMethodProfiler(VkObjectType objectType, const char *methodName,
    const char *fileName, int line) noexcept:
    ScopedProfiler(methodName), objectType(objectType), fileName(fileName), line(line)
{}

inline ScopedMethodProfiler::~ScopedMethodProfiler()
{}

inline ScopedFunctionProfiler::ScopedFunctionProfiler(const char *functionName,
    const char *fileName, int line) noexcept:
    ScopedProfiler(functionName), fileName(fileName), line(line)
{}

inline ScopedFunctionProfiler::~ScopedFunctionProfiler()
{}
} // namespace profile
} // namespace magma
