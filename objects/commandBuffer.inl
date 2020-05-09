namespace magma
{
inline void CommandBuffer::bindPipeline(const std::shared_ptr<Pipeline>& pipeline) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdBindPipeline);
    vkCmdBindPipeline(handle, pipeline->getBindPoint(), *pipeline);
}

inline void CommandBuffer::setViewport(float x, float y, float width, float height,
    float minDepth /* 0 */, float maxDepth /* 1 */) noexcept
{
    VkViewport viewport;
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    MAGMA_PROFILE_ENTRY(vkCmdSetViewport);
    vkCmdSetViewport(handle, 0, 1, &viewport);
}

inline void CommandBuffer::setViewport(uint32_t x, uint32_t y, uint32_t width, int32_t height,
    float minDepth /* 0 */, float maxDepth /* 1 */) noexcept
{
    VkViewport viewport;
    viewport.x = static_cast<float>(x);
    viewport.y = static_cast<float>(y);
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    MAGMA_PROFILE_ENTRY(vkCmdSetViewport);
    vkCmdSetViewport(handle, 0, 1, &viewport);
}

inline void CommandBuffer::setViewport(const Viewport& viewport) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdSetViewport);
    vkCmdSetViewport(handle, 0, 1, &viewport);
}

inline void CommandBuffer::setViewports(const std::initializer_list<Viewport>& viewports) noexcept
{
    MAGMA_ASSERT(viewports.size());
    MAGMA_PROFILE_ENTRY(vkCmdSetViewport);
    vkCmdSetViewport(handle, 0, MAGMA_COUNT(viewports), viewports.begin());
}

inline void CommandBuffer::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) noexcept
{
    VkRect2D scissor;
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
    MAGMA_PROFILE_ENTRY(vkCmdSetScissor);
    vkCmdSetScissor(handle, 0, 1, &scissor);
}

inline void CommandBuffer::setScissor(const Scissor& scissor) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdSetScissor);
    vkCmdSetScissor(handle, 0, 1, &scissor);
}

inline void CommandBuffer::setScissors(const std::initializer_list<Scissor>& scissors) noexcept
{
    MAGMA_ASSERT(scissors.size());
    MAGMA_PROFILE_ENTRY(vkCmdSetScissor);
    vkCmdSetScissor(handle, 0, MAGMA_COUNT(scissors), scissors.begin());
}

inline void CommandBuffer::setLineWidth(float lineWidth) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdSetLineWidth);
    vkCmdSetLineWidth(handle, lineWidth);
}

inline void CommandBuffer::setDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdSetDepthBias);
    vkCmdSetDepthBias(handle, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

inline void CommandBuffer::setBlendConstants(const float blendConstants[4]) noexcept
{
    MAGMA_ASSERT(blendConstants);
    MAGMA_PROFILE_ENTRY(vkCmdSetBlendConstants);
    vkCmdSetBlendConstants(handle, blendConstants);
}

inline void CommandBuffer::setDepthBounds(float minDepthBounds, float maxDepthBounds) noexcept
{
    MAGMA_ASSERT(minDepthBounds <= maxDepthBounds);
    MAGMA_PROFILE_ENTRY(vkCmdSetDepthBounds);
    vkCmdSetDepthBounds(handle, minDepthBounds, maxDepthBounds);
}

inline void CommandBuffer::setStencilCompareMask(bool frontFace, bool backFace, uint32_t compareMask) noexcept
{
    MAGMA_ASSERT(frontFace || backFace);
    MAGMA_PROFILE_ENTRY(vkCmdSetStencilCompareMask);
    vkCmdSetStencilCompareMask(handle, MAGMA_STENCIL_FACE_MASK(frontFace, backFace), compareMask);
}

inline void CommandBuffer::setStencilWriteMask(bool frontFace, bool backFace, uint32_t writeMask) noexcept
{
    MAGMA_ASSERT(frontFace || backFace);
    MAGMA_PROFILE_ENTRY(vkCmdSetStencilWriteMask);
    vkCmdSetStencilWriteMask(handle, MAGMA_STENCIL_FACE_MASK(frontFace, backFace), writeMask);
}

inline void CommandBuffer::setStencilReference(bool frontFace, bool backFace, uint32_t reference) noexcept
{
    MAGMA_ASSERT(frontFace || backFace);
    MAGMA_PROFILE_ENTRY(vkCmdSetStencilReference);
    vkCmdSetStencilReference(handle, MAGMA_STENCIL_FACE_MASK(frontFace, backFace), reference);
}

inline void CommandBuffer::bindDescriptorSet(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<DescriptorSet>& descriptorSet) noexcept
{
    const VkDescriptorSet dereferencedDescriptorSet[1] = {*descriptorSet};
    MAGMA_PROFILE_ENTRY(vkCmdBindDescriptorSets);
    vkCmdBindDescriptorSets(handle, pipeline->getBindPoint(), *pipeline->getLayout(), 0, 1, dereferencedDescriptorSet, 0, nullptr);
}

inline void CommandBuffer::bindDescriptorSet(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<DescriptorSet>& descriptorSet, uint32_t dynamicOffset) noexcept
{
    const VkDescriptorSet dereferencedDescriptorSet[1] = {*descriptorSet};
    MAGMA_PROFILE_ENTRY(vkCmdBindDescriptorSets);
    vkCmdBindDescriptorSets(handle, pipeline->getBindPoint(), *pipeline->getLayout(), 0, 1, dereferencedDescriptorSet, 1, &dynamicOffset);
}

inline void CommandBuffer::bindDescriptorSets(const std::shared_ptr<Pipeline>& pipeline, const std::initializer_list<std::shared_ptr<DescriptorSet>>& descriptorSets,
    const std::initializer_list<uint32_t>& dynamicOffsets /* {} */) noexcept
{
    MAGMA_STACK_ARRAY(VkDescriptorSet, dereferencedDescriptorSets, descriptorSets.size());
    for (const auto& descriptorSet : descriptorSets)
        dereferencedDescriptorSets.put(*descriptorSet);
    MAGMA_PROFILE_ENTRY(vkCmdBindDescriptorSets);
    vkCmdBindDescriptorSets(handle, pipeline->getBindPoint(), *pipeline->getLayout(), 0, dereferencedDescriptorSets.size(), dereferencedDescriptorSets, MAGMA_COUNT(dynamicOffsets), dynamicOffsets.begin());
}

template<uint32_t descriptorSetCount>
inline void CommandBuffer::bindDescriptorSets(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<magma::DescriptorSet>(&descriptorSets)[descriptorSetCount],
    const std::initializer_list<uint32_t>& dynamicOffsets /* {} */) noexcept
{
    MAGMA_STACK_ARRAY(VkDescriptorSet, dereferencedDescriptorSets, descriptorSetCount);
    for (uint32_t i = 0; i < descriptorSetCount; ++i)
        dereferencedDescriptorSets.put(*descriptorSets[i]);
    MAGMA_PROFILE_ENTRY(vkCmdBindDescriptorSets);
    vkCmdBindDescriptorSets(handle, pipeline->getBindPoint(), *pipeline->getLayout(), 0, descriptorSetCount, dereferencedDescriptorSets, MAGMA_COUNT(dynamicOffsets), dynamicOffsets.begin());
}

inline void CommandBuffer::bindIndexBuffer(const std::shared_ptr<BaseIndexBuffer>& indexBuffer, VkDeviceSize offset /* 0 */) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdBindIndexBuffer);
    vkCmdBindIndexBuffer(handle, *indexBuffer, offset, indexBuffer->getIndexType());
}

inline void CommandBuffer::bindVertexBuffer(uint32_t firstBinding, const std::shared_ptr<BaseVertexBuffer>& vertexBuffer,
    VkDeviceSize offset /* 0 */) noexcept
{
    const VkBuffer dereferencedBuffers[1] = {*vertexBuffer};
    MAGMA_PROFILE_ENTRY(vkCmdBindVertexBuffers);
    vkCmdBindVertexBuffers(handle, firstBinding, 1, dereferencedBuffers, &offset);
}

inline void CommandBuffer::bindVertexBuffers(uint32_t firstBinding, const std::vector<std::shared_ptr<BaseVertexBuffer>>& vertexBuffers,
    std::vector<VkDeviceSize> offsets /* {} */) noexcept
{
    MAGMA_ASSERT(vertexBuffers.size() > 0);
    MAGMA_STACK_ARRAY(VkBuffer, dereferencedBuffers, vertexBuffers.size());
    for (const auto& buffer : vertexBuffers)
        dereferencedBuffers.put(*buffer);
    if (offsets.empty())
        offsets.resize(vertexBuffers.size(), 0);
    else
        MAGMA_ASSERT(offsets.size() >= vertexBuffers.size());
    MAGMA_PROFILE_ENTRY(vkCmdBindVertexBuffers);
    vkCmdBindVertexBuffers(handle, firstBinding, dereferencedBuffers.size(), dereferencedBuffers, offsets.data());
}

inline void CommandBuffer::draw(uint32_t vertexCount,
    uint32_t firstVertex /* 0 */) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDraw);
    vkCmdDraw(handle, vertexCount, 1, firstVertex, 0);
}

inline void CommandBuffer::drawInstanced(uint32_t vertexCount, uint32_t instanceCount,
    uint32_t firstVertex /* 0 */, uint32_t firstInstance /* 0 */) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDraw);
    vkCmdDraw(handle, vertexCount, instanceCount, firstVertex, firstInstance);
}

inline void CommandBuffer::drawIndexed(uint32_t indexCount,
    uint32_t firstIndex /* 0 */, int32_t vertexOffset /* 0 */) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDrawIndexed);
    vkCmdDrawIndexed(handle, indexCount, 1, firstIndex, vertexOffset, 0);
}

inline void CommandBuffer::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDrawIndexed);
    vkCmdDrawIndexed(handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

inline void CommandBuffer::drawIndirect(const std::shared_ptr<Buffer>& buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDrawIndirect);
    vkCmdDrawIndirect(handle, *buffer, offset, drawCount, stride);
}

inline void CommandBuffer::drawIndexedIndirect(const std::shared_ptr<Buffer>& buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDrawIndexedIndirect);
    vkCmdDrawIndexedIndirect(handle, *buffer, offset, drawCount, stride);
}

inline void CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDispatch);
    vkCmdDispatch(handle, groupCountX, groupCountY, groupCountZ);
}

inline void CommandBuffer::dispatchIndirect(const std::shared_ptr<Buffer>& buffer, VkDeviceSize offset) const noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdDispatchIndirect);
    vkCmdDispatchIndirect(handle, *buffer, offset);
}

template<typename Type>
inline void CommandBuffer::updateBuffer(const std::shared_ptr<Buffer>& buffer, const std::vector<Type>& data,
    VkDeviceSize offset /* 0 */) const noexcept
{
    const VkDeviceSize dataSize = static_cast<VkDeviceSize>(sizeof(Type) * data.size());
    udpateBuffer(buffer, dataSize, data.data(), offset);
}

template<typename Type, uint32_t pushConstantCount>
inline void CommandBuffer::pushConstants(const std::shared_ptr<PipelineLayout>& layout, VkShaderStageFlags stageFlags, const Type(&values)[pushConstantCount],
    uint32_t offset /* 0 */) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdPushConstants);
    vkCmdPushConstants(handle, *layout, stageFlags, offset, static_cast<uint32_t>(sizeof(Type) * pushConstantCount), values);
}

template<typename Type>
inline void CommandBuffer::pushConstants(const std::shared_ptr<PipelineLayout>& layout, VkShaderStageFlags stageFlags, const std::vector<Type>& values,
    uint32_t offset /* 0 */) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdPushConstants);
    vkCmdPushConstants(handle, *layout, stageFlags, offset, static_cast<uint32_t>(sizeof(Type) * values.size()), values.data());
}

template<typename BlockType>
inline void CommandBuffer::pushConstantBlock(const std::shared_ptr<PipelineLayout>& layout, VkShaderStageFlags stageFlags, const BlockType& block,
    uint32_t offset /* 0 */) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdPushConstants);
    vkCmdPushConstants(handle, *layout, stageFlags, offset, static_cast<uint32_t>(sizeof(BlockType)), &block);
}

inline void CommandBuffer::nextSubpass(VkSubpassContents contents /* VK_SUBPASS_CONTENTS_INLINE */) noexcept
{
    MAGMA_PROFILE_ENTRY(vkCmdNextSubpass);
    vkCmdNextSubpass(handle, contents);
}

inline void CommandBuffer::endRenderPass() noexcept
{
#ifdef MAGMA_DEBUG_LABEL
    if (beginRenderPassMarked)
    {
        endDebugLabel();
        beginRenderPassMarked = VK_FALSE;
    }
#endif // MAGMA_DEBUG_LABEL
    MAGMA_PROFILE_ENTRY(vkCmdEndRenderPass);
    vkCmdEndRenderPass(handle);
}

inline void CommandBuffer::enableOcclusionQuery(bool enable, VkQueryControlFlags queryFlags) noexcept
{
    occlusionQueryEnable = MAGMA_BOOLEAN(enable);
    this->queryFlags = queryFlags;
}

inline void CommandBuffer::enableConditionalRendering(bool enable) noexcept
{
    conditionalRenderingEnable = MAGMA_BOOLEAN(enable);
}

inline void CommandBuffer::queryPipelineStatistics(VkQueryPipelineStatisticFlags pipelineStatistics) noexcept
{
    this->pipelineStatistics = pipelineStatistics;
}
} // namespace magma
