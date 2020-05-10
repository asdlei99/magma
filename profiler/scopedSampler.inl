namespace magma
{
namespace profile
{
template<typename Type>
inline ScopedSampler<Type>::ScopedSampler(const Type& desc) noexcept:
    desc(desc),
    start(std::chrono::high_resolution_clock::now())
{}

template<>
inline ScopedSampler<ApiEntryDescription>::~ScopedSampler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (auto profiler = Profiler::getInstance())
    {
    }
}

template<>
inline ScopedSampler<MethodDescription>::~ScopedSampler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (auto profiler = Profiler::getInstance())
    {
    }
}

template<>
inline ScopedSampler<FunctionDescription>::~ScopedSampler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (auto profiler = Profiler::getInstance())
    {
    }
}
} // namespace profile
} // namespace magma
