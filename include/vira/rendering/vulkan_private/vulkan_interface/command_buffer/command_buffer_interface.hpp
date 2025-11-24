#ifndef COMMAND_BUFFER_INTERFACE_HPP
#define COMMAND_BUFFER_INTERFACE_HPP

#include <vector>

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

class CommandBufferInterface {
public:

    virtual ~CommandBufferInterface();
    virtual void end() const = 0;
    virtual void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, const vk::ArrayProxy<const vk::BufferCopy>& regions) const = 0;
    virtual void copyBufferToImage(vk::Buffer srcBuffer, vk::Image dstImage, vk::ImageLayout dstImageLayout, const vk::ArrayProxy<const vk::BufferImageCopy>& regions) const = 0;
    virtual void copyImageToBuffer(vk::Image dstImage, vk::ImageLayout dstImageLayout,vk::Buffer srcBuffer,  const vk::ArrayProxy<const vk::BufferImageCopy>& regions) const = 0;
    virtual void bindPipeline(vk::PipelineBindPoint pipelineBindPoint, vk::Pipeline pipeline) const = 0;
    virtual void bindDescriptorSets(vk::PipelineBindPoint pipelineBindPoint, vk::PipelineLayout layout, uint32_t firstSet, vk::ArrayProxy<const vk::DescriptorSet> descriptorSets, vk::ArrayProxy<const uint32_t> dynamicOffsets) const = 0;
    virtual void bindVertexBuffers(uint32_t firstBinding, vk::ArrayProxy<const vk::Buffer> buffers, vk::ArrayProxy<const vk::DeviceSize> offsets) const = 0;
    virtual void bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType) const = 0;
    // virtual void pushConstants(vk::PipelineLayout layout, vk::ShaderStageFlags stageFlags, uint32_t offset, vk::ArrayProxy<const ValuesType> const &  values) const = 0;
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const = 0;
    virtual void begin(const vk::CommandBufferBeginInfo& beginInfo) const = 0;
    virtual void beginRenderPass(const vk::RenderPassBeginInfo& renderPassBegin, vk::SubpassContents contents) const = 0;
    virtual void setViewport(uint32_t firstViewport, vk::ArrayProxy<const vk::Viewport> viewports) const = 0;
    virtual void setScissor(uint32_t firstScissor, vk::ArrayProxy<const vk::Rect2D> scissors) const = 0;
    virtual void endRenderPass() const = 0;
    virtual void pipelineBarrier(
        vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::MemoryBarrier> memoryBarriers,
        vk::ArrayProxy<const vk::BufferMemoryBarrier> bufferMemoryBarriers,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> imageMemoryBarriers
    ) const = 0;
    virtual vk::CommandBuffer get() const = 0;
    virtual void  buildAccelerationStructuresKHR( vk::ArrayProxy<const vk::AccelerationStructureBuildGeometryInfoKHR> const & infos,
                                                  vk::ArrayProxy<const vk::AccelerationStructureBuildRangeInfoKHR * const> const & pBuildRangeInfos
                                                  ) const = 0;
    virtual void setDispatch(vk::DispatchLoaderDynamic* dldi) = 0;

};

inline CommandBufferInterface::~CommandBufferInterface() {}

} // namespace vira::vulkan

#endif // COMMAND_BUFFER_INTERFACE_HPP
