namespace magma
{
constexpr AccelerationStructureBuildRange::AccelerationStructureBuildRange(uint32_t primitiveCount,
    uint32_t primitiveOffset /* 0 */,
    uint32_t firstVertex /* 0 */,
    uint32_t transformOffset /* 0 */):
    VkAccelerationStructureBuildRangeInfoKHR{
        primitiveCount,
        primitiveOffset,
        firstVertex,
        transformOffset
    }
{}

inline bool AccelerationStructure::topLevel() const noexcept
{
    return (VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR == structureType);
}

inline bool AccelerationStructure::bottomLevel() const noexcept
{
    return (VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR == structureType);
}

inline bool AccelerationStructure::hostBuild() const noexcept
{
    return (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR == buildType) ||
        (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR == buildType);
}

inline bool AccelerationStructure::deviceBuild() const noexcept
{
    return (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR == buildType) ||
        (VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR == buildType);
}

inline bool AccelerationStructure::serialize(void *data) const noexcept
{
    return copyToMemory(data, nullptr, VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR);
}

inline bool AccelerationStructure::deserialize(const void *data) noexcept
{
    return copyFromMemory(data, nullptr, VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR);
}
} // namespace magma
