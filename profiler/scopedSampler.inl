namespace magma
{
namespace profile
{
template<typename Type>
inline ScopedSampler<Type>::ScopedSampler(const Type& desc) noexcept:
    desc(desc), start(std::chrono::high_resolution_clock::now())
{}

template<>
inline ScopedSampler<ApiEntryDescription>::~ScopedSampler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (std::shared_ptr<IProfiler> profiler = Profiler::getInstance())
    {
        const std::chrono::nanoseconds duration = end - start;
        profiler->profileApiEntry(desc.entryName, desc.flags, duration);
    }
}

template<>
inline ScopedSampler<MethodDescription>::~ScopedSampler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (std::shared_ptr<IProfiler> profiler = Profiler::getInstance())
    {
        const std::chrono::nanoseconds duration = end - start;
        profiler->profileMethod(desc.methodName, desc.objectType,
            desc.fileName, desc.line, duration);
    }
}

template<>
inline ScopedSampler<FunctionDescription>::~ScopedSampler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (std::shared_ptr<IProfiler> profiler = Profiler::getInstance())
    {
        const std::chrono::nanoseconds duration = end - start;
        profiler->profileFunction(desc.functionName, desc.fileName, desc.line, duration);
    }
}
} // namespace profile
} // namespace magma
