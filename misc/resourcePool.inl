namespace magma
{
template<typename Type>
inline void ResourcePool::Pool<Type>::registerResource(const Type *resource) noexcept
{
    try {
        resources.insert(resource);
    } catch (...) {}
}

template<typename Type>
inline void ResourcePool::Pool<Type>::unregisterResource(const Type *resource) noexcept
{
    try {
        auto it = resources.find(resource);
        MAGMA_ASSERT(it != resources.end());
        if (it != resources.end())
            resources.erase(it);
    } catch (...) {}
}

template<typename Type>
inline uint32_t ResourcePool::Pool<Type>::resourceCount() const noexcept
{
    return static_cast<uint32_t>(resources.size());
}

template<typename Type>
void ResourcePool::Pool<Type>::forEach(const std::function<void(const Type *resource)>& fn) const noexcept
{
    for (const Type *resource : resources)
        fn(resource);
}

#define MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Type, pool)\
template<>\
inline ResourcePool::Pool<Type>& ResourcePool::getPool<Type>() noexcept\
{\
    return pool;\
}

MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(DeviceMemory, deviceMemories)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Buffer, buffers)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Image, images)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Framebuffer, framebuffers)
#ifdef VK_NV_ray_tracing
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(AccelerationStructure, accelerationStructures)
#endif
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Pipeline, pipelines)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(PipelineLayout, pipelineLayouts)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(DescriptorSet, descriptorSets)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(DescriptorSetLayout, descriptorSetLayouts)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(CommandBuffer, commandBuffers)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Fence, fences)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Event, events)
MAGMA_RESOURCE_POOL_IMPLEMENT_ACCESSOR(Semaphore, semaphores)
} // namespace magma
