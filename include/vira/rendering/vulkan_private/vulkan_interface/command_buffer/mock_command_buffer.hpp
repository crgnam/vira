#ifndef MOCK_VULKAN_COMMAND_BUFFER_HPP
#define MOCK_VULKAN_COMMAND_BUFFER_HPP

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"

namespace vira::vulkan {

class MockCommandBuffer : public CommandBufferInterface {
public:

    MOCK_METHOD(void, end, (), (const, override));
    MOCK_METHOD(void, copyBuffer, (vk::Buffer srcBuffer, vk::Buffer dstBuffer, const vk::ArrayProxy<const vk::BufferCopy>& regions), (const, override));
    MOCK_METHOD(void, copyBufferToImage, 
                (vk::Buffer srcBuffer, 
                vk::Image dstImage, 
                vk::ImageLayout dstImageLayout, 
                const vk::ArrayProxy<const vk::BufferImageCopy>& regions
                ), (const, override));
    MOCK_METHOD(void, copyImageToBuffer, (vk::Image dstImage, vk::ImageLayout dstImageLayout,vk::Buffer srcBuffer,  const vk::ArrayProxy<const vk::BufferImageCopy>& regions), (const, override));

    MOCK_METHOD(void, bindPipeline, (vk::PipelineBindPoint pipelineBindPoint, vk::Pipeline pipeline), (const, override));
    MOCK_METHOD(void, bindDescriptorSets, (vk::PipelineBindPoint pipelineBindPoint, vk::PipelineLayout layout, uint32_t firstSet, vk::ArrayProxy<const vk::DescriptorSet> descriptorSets, vk::ArrayProxy<const uint32_t> dynamicOffsets), (const, override));
    MOCK_METHOD(void, bindVertexBuffers, (uint32_t firstBinding, vk::ArrayProxy<const vk::Buffer> buffers, vk::ArrayProxy<const vk::DeviceSize> offsets), (const, override));
    MOCK_METHOD(void, bindIndexBuffer, (vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType indexType), (const, override));
    MOCK_METHOD(void, drawIndexed, (uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance), (const, override));
    MOCK_METHOD(void, begin, (const vk::CommandBufferBeginInfo& beginInfo), (const, override));
    MOCK_METHOD(void, beginRenderPass, (const vk::RenderPassBeginInfo& renderPassBegin, vk::SubpassContents contents), (const, override));
    MOCK_METHOD(void, setViewport, (uint32_t firstViewport, vk::ArrayProxy<const vk::Viewport> viewports), (const, override));
    MOCK_METHOD(void, setScissor, (uint32_t firstScissor, vk::ArrayProxy<const vk::Rect2D> scissors), (const, override));
    MOCK_METHOD(void, endRenderPass, (), (const, override));
    MOCK_METHOD(void, pipelineBarrier, (vk::PipelineStageFlags srcStageMask,
        vk::PipelineStageFlags dstStageMask,
        vk::DependencyFlags dependencyFlags,
        vk::ArrayProxy<const vk::MemoryBarrier> memoryBarriers,
        vk::ArrayProxy<const vk::BufferMemoryBarrier> bufferMemoryBarriers,
        vk::ArrayProxy<const vk::ImageMemoryBarrier> imageMemoryBarriers), (const, override));
    MOCK_METHOD(vk::CommandBuffer, get, (), (const, override));
    MOCK_METHOD(void, buildAccelerationStructuresKHR, ( vk::ArrayProxy<const vk::AccelerationStructureBuildGeometryInfoKHR> const & infos,
                                                  vk::ArrayProxy<const vk::AccelerationStructureBuildRangeInfoKHR * const> const & pBuildRangeInfos), (const));
    MOCK_METHOD(void, setDispatch, (vk::DispatchLoaderDynamic* dldi), (override));

};

} // namespace vira::vulkan

#endif // MOCK_VULKAN_COMMAND_BUFFER_HPP
