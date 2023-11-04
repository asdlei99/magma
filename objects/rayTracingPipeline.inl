namespace magma
{
#ifdef VK_KHR_ray_tracing_pipeline
inline VkDeviceSize RayTracingPipeline::getGeneralShaderStackSize(uint32_t group) const noexcept
{
    return getShaderGroupStackSize(group, VK_SHADER_GROUP_SHADER_GENERAL_KHR);
}

inline VkDeviceSize RayTracingPipeline::getClosestHitShaderStackSize(uint32_t group) const noexcept
{
    return getShaderGroupStackSize(group, VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR);
}

inline VkDeviceSize RayTracingPipeline::getAnyHitShaderStackSize(uint32_t group) const noexcept
{
    return getShaderGroupStackSize(group, VK_SHADER_GROUP_SHADER_ANY_HIT_KHR);
}

inline VkDeviceSize RayTracingPipeline::getIntersectionShaderStackSize(uint32_t group) const noexcept
{
    return getShaderGroupStackSize(group, VK_SHADER_GROUP_SHADER_INTERSECTION_KHR);
}

inline std::vector<uint8_t> RayTracingPipeline::getShaderGroupHandles() const
{
    return getShaderGroupHandles(0, shaderGroupCount);
}

inline std::vector<uint8_t> RayTracingPipeline::getCaptureReplayShaderGroupHandles() const
{
    return getCaptureReplayShaderGroupHandles(0, shaderGroupCount);
}
#endif // VK_KHR_ray_tracing_pipeline
} // namespace magma
