#ifndef MOCK_DEVICE_HPP
#define MOCK_DEVICE_HPP

#include "vulkan/vulkan.hpp"
#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/vulkan_device.hpp"

namespace vira::vulkan {

class MockDevice : public DeviceInterface {
public:



    // Mock constructor and destructor
    MockDevice() = default;
    MOCK_METHOD(vk::ResultValue<uint32_t>, acquireNextImageKHR, (std::unique_ptr<SwapchainInterface>& swapchain, uint64_t timeout, vk::Semaphore semaphore, vk::Fence fence), (const, noexcept, override));
    MOCK_METHOD(std::vector<CommandBufferInterface*>, allocateCommandBuffers, (const vk::CommandBufferAllocateInfo& pAllocateInfo), (const, noexcept, override));
    MOCK_METHOD(std::unique_ptr<MemoryInterface>, allocateMemory, (const vk::MemoryAllocateInfo & allocateInfo,     vk::Optional<const vk::AllocationCallbacks> allocator), (const, override));
    MOCK_METHOD(std::vector<vk::DescriptorSet>, allocateDescriptorSets, (const vk::DescriptorSetAllocateInfo allocateInfo), (const, noexcept, override));
    MOCK_METHOD(void, bindBufferMemory, (vk::Buffer buffer, std::unique_ptr<MemoryInterface>& memory, vk::DeviceSize memoryOffset), (const, noexcept, override));
    MOCK_METHOD(void, bindImageMemory, (vk::Image image, std::unique_ptr<MemoryInterface>& memory, vk::DeviceSize memoryOffset), (const, noexcept, override));
    MOCK_METHOD(vk::UniqueBuffer, createBufferUnique, (const vk::BufferCreateInfo & createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, override));
    MOCK_METHOD(std::unique_ptr<CommandPoolInterface>, createCommandPool, (const vk::CommandPoolCreateInfo & createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, override));
    MOCK_METHOD(vk::DescriptorPool, createDescriptorPool, (const vk::DescriptorPoolCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::DescriptorSetLayout, createDescriptorSetLayout, (const vk::DescriptorSetLayoutCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::Fence, createFence, (const vk::FenceCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::Framebuffer, createFramebuffer, (const vk::FramebufferCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::ResultValue<vk::Pipeline>, createGraphicsPipeline, (vk::PipelineCache pipelineCache, const vk::GraphicsPipelineCreateInfo createInfos, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::Image, createImage, (const vk::ImageCreateInfo & createInfo, const vk::AllocationCallbacks* allocator), (const, override));
    MOCK_METHOD(vk::ImageView, createImageView, (const vk::ImageViewCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::Sampler, createSampler, (const vk::SamplerCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::BufferView, createBufferView, (const vk::BufferViewCreateInfo& createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::PipelineLayout, createPipelineLayout, (const vk::PipelineLayoutCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::RenderPass, createRenderPass, (const vk::RenderPassCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::Semaphore, createSemaphore, (const vk::SemaphoreCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::UniqueShaderModule, createShaderModule, (const vk::ShaderModuleCreateInfo createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(std::unique_ptr<SwapchainInterface>, createSwapchainKHR, (const vk::SwapchainCreateInfoKHR createInfo, vk::Optional<const vk::AllocationCallbacks> allocator), (const, override));
    MOCK_METHOD(void, destroyCommandPool, (std::unique_ptr<CommandPoolInterface> commandPool, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyDescriptorPool, (vk::DescriptorPool descriptorPool, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyDescriptorSetLayout, (vk::DescriptorSetLayout descriptorSetLayout, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyFence, (vk::Fence fence, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyFramebuffer, (vk::Framebuffer framebuffer, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyImage, (vk::Image image, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyImageView, (vk::ImageView imageView, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyBufferView, (vk::BufferView bufferView, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroySampler, (vk::Sampler imageView, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyPipeline, (vk::Pipeline pipeline, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyPipelineLayout, (vk::PipelineLayout pipelineLayout, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyRenderPass, (vk::RenderPass renderPass, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroySemaphore, (vk::Semaphore semaphore, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, destroyShaderModule, (vk::ShaderModule shaderModule, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(void, flushMappedMemoryRanges, (const std::vector<vk::MappedMemoryRange>& pMemoryRanges), (const, noexcept, override));
    MOCK_METHOD(void, freeMemory, (std::unique_ptr<MemoryInterface> memory, vk::Optional<const vk::AllocationCallbacks> allocator), (const, noexcept, override));
    MOCK_METHOD(vk::MemoryRequirements, getBufferMemoryRequirements, (vk::Buffer buffer), (const, noexcept, override));
    MOCK_METHOD(vk::MemoryRequirements, getImageMemoryRequirements, (vk::Image image), (const, override));
    MOCK_METHOD(QueueInterface*, getQueue, (uint32_t queueFamilyIndex, uint32_t queueIndex), (const, noexcept, override));
    MOCK_METHOD(std::vector<vk::Image>, getSwapchainImagesKHR, (std::unique_ptr<SwapchainInterface>& swapchain), (const, override));
    MOCK_METHOD(vk::Result, invalidateMappedMemoryRanges, (uint32_t memoryRangeCount, const vk::MappedMemoryRange * pMemoryRanges), (const, noexcept, override));
    MOCK_METHOD(void*, mapMemory, (MemoryInterface* memory, vk::DeviceSize offset, vk::DeviceSize size, vk::MemoryMapFlags flags
), (const, noexcept, override));
    MOCK_METHOD(void, resetDescriptorPool, (vk::DescriptorPool descriptorPool, vk::DescriptorPoolResetFlags flags), (const, noexcept, override));
    MOCK_METHOD(void, resetFences, (vk::ArrayProxy<const vk::Fence> const & fences), (const, noexcept, override));
    MOCK_METHOD(void, unmapMemory, (std::unique_ptr<MemoryInterface>& memory), (const, noexcept, override));
    MOCK_METHOD(void, updateDescriptorSets, (const std::vector<vk::WriteDescriptorSet> & descriptorWrites, const std::vector<vk::CopyDescriptorSet>& descriptorCopies), (const, noexcept, override));
    MOCK_METHOD(vk::Result, waitForFences, (vk::ArrayProxy<const vk::Fence> const & fences, vk::Bool32 waitAll, uint64_t timeout), (const, noexcept, override));
    MOCK_METHOD(void, waitIdle, (), (const, override));
    MOCK_METHOD(vk::AccelerationStructureBuildSizesInfoKHR , getAccelerationStructureBuildSizesKHR, (vk::AccelerationStructureBuildTypeKHR buildType, const vk::AccelerationStructureBuildGeometryInfoKHR & buildInfo,vk::ArrayProxy<const uint32_t> const & maxPrimitiveCounts), (const,noexcept, override));
    MOCK_METHOD(vk::AccelerationStructureKHR, createAccelerationStructureKHR, (vk::AccelerationStructureCreateInfoKHR createInfo), (const,noexcept, override)); 
    MOCK_METHOD(vk::DeviceAddress, getBufferAddress,(
        vk::BufferDeviceAddressInfo addressInfo
    ),(const, noexcept, override));
    MOCK_METHOD(const vk::Device&, getDevice, (), (const, override));
    MOCK_METHOD(vk::ResultValue<vk::Pipeline>, createRayTracingPipelineKHR, (vk::DeferredOperationKHR deferredOperation, 
    vk::PipelineCache pipelineCache, 
    const vk::RayTracingPipelineCreateInfoKHR & createInfo, 
    const vk::AllocationCallbacks* allocator
    ), 
    (const, noexcept, override)); 
    MOCK_METHOD(vk::Result, getRayTracingShaderGroupHandlesKHR, (vk::Pipeline pipeline, 
    uint32_t  firstGroup, 
    uint32_t groupCount, 
    size_t dataSize, 
    void * pData), (const, override));
    MOCK_METHOD(vk::DeviceAddress, 
                getAccelerationStructureAddressKHR, 
                (const vk::AccelerationStructureDeviceAddressInfoKHR& info), 
                (const, override));
    MOCK_METHOD(void, setDispatch, (vk::DispatchLoaderDynamic* dldi), (override));
    MOCK_METHOD(const vk::UniqueDevice&, getUniqueDevice, (), (const override));

};

} // namespace vira::vulkan

#endif // MOCK_DEVICE_HPP
