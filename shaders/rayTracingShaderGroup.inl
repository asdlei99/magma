namespace magma
{
#ifdef VK_KHR_ray_tracing_pipeline
inline RayTracingShaderGroup::RayTracingShaderGroup(VkRayTracingShaderGroupTypeKHR type_,
    uint32_t generalShader_, uint32_t closestHitShader_,
    uint32_t anyHitShader_, uint32_t intersectionShader_,
	const void *captureReplayHandle) noexcept
{
    sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    pNext = nullptr;
    type = type_;
    generalShader = generalShader_;
    closestHitShader = closestHitShader_;
    anyHitShader = anyHitShader_;
    intersectionShader = intersectionShader_;
	pShaderGroupCaptureReplayHandle = captureReplayHandle;
}

inline hash_t RayTracingShaderGroup::hash() const noexcept
{
    return core::hashArgs(
        sType,
        type,
        generalShader,
        closestHitShader,
        anyHitShader,
        intersectionShader);
}

inline GeneralRayTracingShaderGroup::GeneralRayTracingShaderGroup(uint32_t generalShader,
    const void *captureReplayHandle /* nullptr */) noexcept:
    RayTracingShaderGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        generalShader, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR, VK_SHADER_UNUSED_KHR,
        captureReplayHandle)
{}

inline TrianglesHitRayTracingShaderGroup::TrianglesHitRayTracingShaderGroup(uint32_t closestHitShader,
    uint32_t anyHitShader /* VK_SHADER_UNUSED_KHR */,
    const void *captureReplayHandle /* nullptr */) noexcept:
    RayTracingShaderGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        VK_SHADER_UNUSED_KHR, closestHitShader, anyHitShader, VK_SHADER_UNUSED_KHR,
        captureReplayHandle)
{}

inline ProceduralHitRayTracingShaderGroup::ProceduralHitRayTracingShaderGroup(uint32_t intersectionShader,
    uint32_t closestHitShader /* VK_SHADER_UNUSED_KHR */,
    uint32_t anyHitShader /* VK_SHADER_UNUSED_KHR */,
    const void *captureReplayHandle /* nullptr */) noexcept:
    RayTracingShaderGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
        VK_SHADER_UNUSED_KHR, closestHitShader, anyHitShader, intersectionShader,
        captureReplayHandle)
{}
#endif // VK_KHR_ray_tracing_pipeline
} // namespace magma
