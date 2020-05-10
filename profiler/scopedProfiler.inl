namespace magma
{
namespace profile
{
template<typename Type>
inline ScopedProfiler<Type>::ScopedProfiler(const Type& description) noexcept:
    description(description),
    start(std::chrono::high_resolution_clock::now())
{}

template<>
inline ScopedProfiler<ApiEntryDescription>::~ScopedProfiler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (profiler)
    {
    }
}

template<>
inline ScopedProfiler<MethodDescription>::~ScopedProfiler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (profiler)
    {
    }
}

template<>
inline ScopedProfiler<FunctionDescription>::~ScopedProfiler()
{
    const auto end = std::chrono::high_resolution_clock::now();
    if (profiler)
    {
    }
}
} // namespace profile
} // namespace magma
