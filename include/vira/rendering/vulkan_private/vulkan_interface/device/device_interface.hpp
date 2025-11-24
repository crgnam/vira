#ifndef DEVICE_INTERFACE_HPP
#define DEVICE_INTERFACE_HPP

#include <vector>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"

namespace vira::vulkan {

class DeviceInterface {
public:

    virtual ~DeviceInterface() = default;
    
    // allocateCommandBuffers
    virtual std::vector<CommandBufferInterface*> allocateCommandBuffers(
        const vk::CommandBufferAllocateInfo& pAllocateInfo
    ) const noexcept = 0;

    // allocateDescriptorSets
    virtual std::vector<vk::DescriptorSet> allocateDescriptorSets(
        const vk::DescriptorSetAllocateInfo allocateInfo
    ) const noexcept = 0;

    // allocateMemory
    virtual std::unique_ptr<MemoryInterface> allocateMemory( 
        const vk::MemoryAllocateInfo & allocateInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const = 0;
    
    // acquireNextImageKHR
    virtual vk::ResultValue<uint32_t> acquireNextImageKHR(
        std::unique_ptr<SwapchainInterface>& swapchain,
        uint64_t timeout,
        vk::Semaphore semaphore,
        vk::Fence fence
    ) const noexcept = 0;
    
    // bindBufferMemory
    virtual void bindBufferMemory(
        vk::Buffer buffer,
        std::unique_ptr<MemoryInterface>& memory,
        vk::DeviceSize memoryOffset
    ) const noexcept = 0;

    // bindImageMemory
    virtual void bindImageMemory(
        vk::Image image,
        std::unique_ptr<MemoryInterface>& memory,
        vk::DeviceSize memoryOffset
    ) const noexcept = 0;

    // createBufferUnique
    virtual vk::UniqueBuffer createBufferUnique(
        const vk::BufferCreateInfo & createInfo, vk::Optional<const vk::AllocationCallbacks> allocator
    ) const = 0;

    // createCommandPool
    virtual std::unique_ptr<CommandPoolInterface> createCommandPool( 
        const vk::CommandPoolCreateInfo &       createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator 
    ) const = 0;

    // createDescriptorPool
    virtual vk::DescriptorPool createDescriptorPool(
        const vk::DescriptorPoolCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createDescriptorSetLayout
    virtual vk::DescriptorSetLayout createDescriptorSetLayout(
            const vk::DescriptorSetLayoutCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createFence
    virtual vk::Fence createFence(
        const vk::FenceCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createFramebuffer
    virtual vk::Framebuffer createFramebuffer(
        const vk::FramebufferCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createGraphicsPipelines
    virtual vk::ResultValue<vk::Pipeline> createGraphicsPipeline(
        vk::PipelineCache pipelineCache,
        const vk::GraphicsPipelineCreateInfo createInfos,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createImage
    virtual vk::Image createImage(
        const vk::ImageCreateInfo & createInfo, 
        const vk::AllocationCallbacks* allocator
    ) const = 0;
        
    virtual vk::ImageView createImageView(
        const vk::ImageViewCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createSampler
    virtual vk::Sampler createSampler(
        const vk::SamplerCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createBufferView
    
    virtual vk::BufferView createBufferView(
        const vk::BufferViewCreateInfo& createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createPipelineLayout
    virtual vk::PipelineLayout createPipelineLayout(
        const vk::PipelineLayoutCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createRayTracingPipelineKHR
    virtual vk::ResultValue<vk::Pipeline> createRayTracingPipelineKHR(
        vk::DeferredOperationKHR deferredOperation, 
        vk::PipelineCache pipelineCache, 
        const vk::RayTracingPipelineCreateInfoKHR & createInfo, 
        const vk::AllocationCallbacks* allocator
    ) const noexcept = 0;

    // createRenderPass
    virtual vk::RenderPass createRenderPass(
        const vk::RenderPassCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createSemaphore
    virtual vk::Semaphore createSemaphore(
        const vk::SemaphoreCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createShaderModule
    virtual vk::UniqueShaderModule createShaderModule(
        const vk::ShaderModuleCreateInfo createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // createSwapchainKHR
    virtual std::unique_ptr<SwapchainInterface> createSwapchainKHR(
        const vk::SwapchainCreateInfoKHR createInfo,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const = 0;

    // getSwapchainImagesKHR
    virtual std::vector<vk::Image> getSwapchainImagesKHR( 
        std::unique_ptr<SwapchainInterface>& swapchain 
    ) const = 0;

    // destroyCommandPool
    virtual void destroyCommandPool(
        std::unique_ptr<CommandPoolInterface> commandPool,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyDescriptorPool
    virtual void destroyDescriptorPool(
        vk::DescriptorPool descriptorPool,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyDescriptorSetLayout
    virtual void destroyDescriptorSetLayout(
        vk::DescriptorSetLayout descriptorSetLayout,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyFence
    virtual void destroyFence(
        vk::Fence fence,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyFramebuffer
    virtual void destroyFramebuffer(
        vk::Framebuffer framebuffer,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyImage
    virtual void destroyImage(
        vk::Image image,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyImageView
    virtual void destroyImageView(
        vk::ImageView imageView,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyBufferView
    virtual void destroyBufferView(
        vk::BufferView bufferView,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroySampler
    virtual void destroySampler(
        vk::Sampler sampler,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyPipeline
    virtual void destroyPipeline(
        vk::Pipeline pipeline,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyPipelineLayout
    virtual void destroyPipelineLayout(
        vk::PipelineLayout pipelineLayout,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyRenderPass
    virtual void destroyRenderPass(
        vk::RenderPass renderPass,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroySemaphore
    virtual void destroySemaphore(
        vk::Semaphore semaphore,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // destroyShaderModule
    virtual void destroyShaderModule(
        vk::ShaderModule shaderModule,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // flushMappedMemoryRanges
    virtual void flushMappedMemoryRanges( 
        const std::vector<vk::MappedMemoryRange>& pMemoryRanges
    ) const noexcept = 0;

    // freeMemory
    virtual void freeMemory( 
        std::unique_ptr<MemoryInterface> memory,
        vk::Optional<const vk::AllocationCallbacks> allocator
    ) const noexcept = 0;

    // getAccelerationStructureBuildSizesKHR
    virtual vk::AccelerationStructureBuildSizesInfoKHR getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR buildType, 
        const vk::AccelerationStructureBuildGeometryInfoKHR & buildInfo,
        vk::ArrayProxy<const uint32_t> const & maxPrimitiveCounts
    ) const noexcept = 0;

    // getAccelerationStructureBuildSizesKHR
    virtual vk::DeviceAddress getAccelerationStructureAddressKHR(
        const vk::AccelerationStructureDeviceAddressInfoKHR& info
    ) const = 0;    

    // createAccelerationStructureKHR
    virtual vk::AccelerationStructureKHR createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR createInfo) const noexcept = 0;

    // getBufferAddress
    virtual vk::DeviceAddress getBufferAddress(
        vk::BufferDeviceAddressInfo addressInfo
    ) const noexcept = 0;
    
    // getBufferMemoryRequirements
    virtual vk::MemoryRequirements getBufferMemoryRequirements( 
        vk::Buffer buffer
    ) const noexcept = 0;

    //getImageMemoryRequirements
    virtual vk::MemoryRequirements getImageMemoryRequirements(
        vk::Image image
    ) const = 0;

    // getQueue
    virtual QueueInterface* getQueue( 
        uint32_t queueFamilyIndex, 
        uint32_t queueIndex 
    ) const noexcept = 0;

    // invalidateMappedMemoryRanges
    virtual vk::Result invalidateMappedMemoryRanges( 
        uint32_t memoryRangeCount, 
        const vk::MappedMemoryRange * pMemoryRanges
    ) const noexcept = 0;

    // mapMemory
    virtual void* mapMemory(
        MemoryInterface*   memory,
        vk::DeviceSize     offset,
        vk::DeviceSize     size,
        vk::MemoryMapFlags flags
    ) const noexcept = 0;

    // resetDescriptorPool
    virtual void resetDescriptorPool(
        vk::DescriptorPool descriptorPool,
        vk::DescriptorPoolResetFlags flags
    ) const noexcept = 0;

    // resetFences
    virtual void resetFences(
        vk::ArrayProxy<const vk::Fence> const & fences    
    ) const noexcept = 0;

    // unmapMemory
    virtual void unmapMemory(
        std::unique_ptr<MemoryInterface>& memory
    ) const noexcept = 0;

    // updateDescriptorSets
    virtual void updateDescriptorSets(
        const std::vector<vk::WriteDescriptorSet> & descriptorWrites,
        const std::vector<vk::CopyDescriptorSet>& descriptorCopies
    ) const noexcept = 0;

    // waitForFences
    virtual vk::Result waitForFences(
        vk::ArrayProxy<const vk::Fence> const & fences,    
        vk::Bool32 waitAll,
        uint64_t timeout
    ) const noexcept = 0;

    // waitIdle
    virtual void waitIdle() const = 0;

    virtual void setDispatch(vk::DispatchLoaderDynamic* dldi) = 0;
    virtual const vk::Device& getDevice() const = 0;

    // getRayTracingShaderGroupHandlesKHR
    virtual vk::Result getRayTracingShaderGroupHandlesKHR( 
        vk::Pipeline pipeline, 
        uint32_t  firstGroup, 
        uint32_t groupCount, 
        size_t dataSize, 
        void * pData
    ) const = 0;

    virtual const vk::UniqueDevice& getUniqueDevice() const = 0;

}; 

} // namespace vira::vulkan

#endif // DEVICE_INTERFACE_HPP
