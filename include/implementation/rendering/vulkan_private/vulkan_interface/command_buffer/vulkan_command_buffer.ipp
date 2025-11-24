namespace vira::vulkan {

// Constructor
VulkanCommandBuffer::VulkanCommandBuffer(vk::UniqueCommandBuffer commandBuffer)
    : commandBuffer_(std::move(commandBuffer)) {

}
VulkanCommandBuffer::VulkanCommandBuffer(vk::UniqueCommandBuffer commandBuffer, vk::DispatchLoaderDynamic* dldi)
    : commandBuffer_(std::move(commandBuffer)) {
        setDispatch(dldi);
}

void VulkanCommandBuffer::end() const {
    commandBuffer_->end(*dldi);
}

void VulkanCommandBuffer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, const vk::ArrayProxy<const vk::BufferCopy>& regions) const {
    commandBuffer_->copyBuffer(srcBuffer, dstBuffer, regions, *dldi);
}

void VulkanCommandBuffer::copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, vk::ImageLayout dstImageLayout, const vk::ArrayProxy<const vk::BufferImageCopy>& regions) const {
    commandBuffer_->copyBufferToImage(srcBuffer, dstImage, dstImageLayout, regions, *dldi);
}

void VulkanCommandBuffer::copyImageToBuffer(vk::Image srcImage, vk::ImageLayout srcImageLayout,vk::Buffer dstBuffer,  const vk::ArrayProxy<const vk::BufferImageCopy>& regions) const {
    commandBuffer_->copyImageToBuffer(srcImage, srcImageLayout, dstBuffer, regions, *dldi);
}

void VulkanCommandBuffer::bindPipeline(vk::PipelineBindPoint pipelineBindPoint, vk::Pipeline pipeline) const {
    commandBuffer_->bindPipeline(pipelineBindPoint, pipeline, *dldi);
}

void VulkanCommandBuffer::bindDescriptorSets(vk::PipelineBindPoint pipelineBindPoint, vk::PipelineLayout layout, uint32_t firstSet, vk::ArrayProxy<const vk::DescriptorSet> descriptorSets, vk::ArrayProxy<const uint32_t> dynamicOffsets) const {
    commandBuffer_->bindDescriptorSets(pipelineBindPoint, layout, firstSet, descriptorSets, dynamicOffsets, *dldi);
}

void VulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, vk::ArrayProxy<const vk::Buffer> buffers, vk::ArrayProxy<const vk::DeviceSize> offsets) const {
    commandBuffer_->bindVertexBuffers(firstBinding, buffers, offsets, *dldi);
}

void VulkanCommandBuffer::bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const {
    commandBuffer_->bindIndexBuffer( buffer,  offset,  indexType, *dldi);
}

void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
    commandBuffer_->drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, *dldi);
}

void VulkanCommandBuffer::begin(const vk::CommandBufferBeginInfo& beginInfo) const {
    try {
        commandBuffer_->begin(beginInfo, *dldi);
    }
    catch (const std::exception& err) {
        std::cerr << "Vulkan error: " << err.what() << std::endl;
    }
}

void VulkanCommandBuffer::beginRenderPass(const vk::RenderPassBeginInfo& renderPassBegin, vk::SubpassContents contents) const {
    commandBuffer_->beginRenderPass(renderPassBegin, contents, *dldi);
}

void VulkanCommandBuffer::setViewport(uint32_t firstViewport, vk::ArrayProxy<const vk::Viewport> viewports) const {
    commandBuffer_->setViewport(firstViewport, viewports, *dldi);
}

void VulkanCommandBuffer::setScissor(uint32_t firstScissor, vk::ArrayProxy<const vk::Rect2D> scissors) const {
    commandBuffer_->setScissor(firstScissor, scissors, *dldi); 
}

void VulkanCommandBuffer::endRenderPass() const {
    commandBuffer_->endRenderPass(*dldi); 
}

void VulkanCommandBuffer::pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::MemoryBarrier> memoryBarriers,
        vk::ArrayProxy<const vk::BufferMemoryBarrier> bufferMemoryBarriers,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> imageMemoryBarriers) const {
    commandBuffer_->pipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarriers, bufferMemoryBarriers, imageMemoryBarriers, *dldi);
};    

vk::CommandBuffer VulkanCommandBuffer::get() const {
    return *commandBuffer_;
}

void VulkanCommandBuffer::buildAccelerationStructuresKHR(vk::ArrayProxy<const vk::AccelerationStructureBuildGeometryInfoKHR> const & infos,
                                        vk::ArrayProxy<const vk::AccelerationStructureBuildRangeInfoKHR * const> const & pBuildRangeInfos
                                        ) const {
                                            commandBuffer_->buildAccelerationStructuresKHR(infos, pBuildRangeInfos, *dldi);
                                        }

}
