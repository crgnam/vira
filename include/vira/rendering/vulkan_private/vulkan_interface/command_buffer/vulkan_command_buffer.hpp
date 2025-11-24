#ifndef VULKAN_COMMAND_BUFFER_HPP
#define VULKAN_COMMAND_BUFFER_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"

namespace vira::vulkan {

class VulkanCommandBuffer : public CommandBufferInterface {
public:

    // Constructor
    VulkanCommandBuffer(vk::UniqueCommandBuffer commandBuffer);
    VulkanCommandBuffer(vk::UniqueCommandBuffer commandBuffer, vk::DispatchLoaderDynamic* dldi);

    void end() const override;
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, const vk::ArrayProxy<const vk::BufferCopy>& regions) const override;
    void copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, vk::ImageLayout dstImageLayout, const vk::ArrayProxy<const vk::BufferImageCopy>& regions) const override;
    void copyImageToBuffer(vk::Image dstImage, vk::ImageLayout dstImageLayout,vk::Buffer srcBuffer,  const vk::ArrayProxy<const vk::BufferImageCopy>& regions) const override;
    void bindPipeline(vk::PipelineBindPoint pipelineBindPoint, vk::Pipeline pipeline) const override;
    void bindDescriptorSets(vk::PipelineBindPoint pipelineBindPoint, vk::PipelineLayout layout, uint32_t firstSet, vk::ArrayProxy<const vk::DescriptorSet> descriptorSets, vk::ArrayProxy<const uint32_t> dynamicOffsets) const override;
    void bindVertexBuffers(uint32_t firstBinding, vk::ArrayProxy<const vk::Buffer> buffers, vk::ArrayProxy<const vk::DeviceSize> offsets) const override;
    void bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const override;
    // void pushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stageFlags, uint32_t offset, vk::ArrayProxy<const void> values) const override;
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const override;
    void begin(const vk::CommandBufferBeginInfo& beginInfo) const override;
    void beginRenderPass(const vk::RenderPassBeginInfo& renderPassBegin, vk::SubpassContents contents) const override;
    void setViewport(uint32_t firstViewport, vk::ArrayProxy<const vk::Viewport> viewports) const override;
    void setScissor(uint32_t firstScissor, vk::ArrayProxy<const vk::Rect2D> scissors) const override;
    void endRenderPass() const override;
    void buildAccelerationStructuresKHR(vk::ArrayProxy<const vk::AccelerationStructureBuildGeometryInfoKHR> const & infos,
                                        vk::ArrayProxy<const vk::AccelerationStructureBuildRangeInfoKHR * const> const & pBuildRangeInfos
                                        ) const;
    void pipelineBarrier(
            vk::PipelineStageFlags srcStageMask,
            vk::PipelineStageFlags dstStageMask,
            vk::DependencyFlags dependencyFlags,
            vk::ArrayProxy<const vk::MemoryBarrier> memoryBarriers,
            vk::ArrayProxy<const vk::BufferMemoryBarrier> bufferMemoryBarriers,
            vk::ArrayProxy<const vk::ImageMemoryBarrier> imageMemoryBarriers
        ) const override;                                                
    vk::CommandBuffer get() const override;
    void setDispatch(vk::DispatchLoaderDynamic* newDldi) override {this->dldi = newDldi;};

private:
    vk::UniqueCommandBuffer commandBuffer_;
    vk::DispatchLoaderDynamic* dldi;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/command_buffer/vulkan_command_buffer.ipp"

#endif // VULKAN_COMMAND_BUFFER_HPP
