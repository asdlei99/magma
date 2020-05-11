namespace magma
{
template<typename Type>
inline void ResourcePool::ResourceSet<Type>::registerResource(const Type *resource) noexcept
{
    resources.insert(resource);
}

template<typename Type>
inline void ResourcePool::ResourceSet<Type>::unregisterResource(const Type *resource) noexcept
{
    auto it = resources.find(resource);
    if (it != resources.end())
        resources.erase(it);
}

template<typename Type>
inline uint32_t ResourcePool::ResourceSet<Type>::resourceCount() const noexcept
{
    return static_cast<uint32_t>(resources.size());
}

template<typename Type>
void ResourcePool::ResourceSet<Type>::forEach(const std::function<void(const Type *resource)>& fn) const noexcept
{
    for (const Type *resource : resources)
        fn(resource);
}

template<> inline ResourcePool::ResourceSet<Buffer>& ResourcePool::getResourceSet<Buffer>() noexcept { return buffers; }
template<> inline ResourcePool::ResourceSet<Image>& ResourcePool::getResourceSet<Image>() noexcept { return images; }
template<> inline ResourcePool::ResourceSet<DeviceMemory>& ResourcePool::getResourceSet<DeviceMemory>() noexcept { return deviceMemories; }
} // namespace magma
