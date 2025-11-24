#include "vira/rendering/vulkan_private/vulkan_interface/queue/vulkan_queue.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/vulkan_command_pool.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/vulkan_command_buffer.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/vulkan_swapchain.hpp"

#undef VULKAN_HPP_DISABLE_ENHANCED_MODE

namespace vira::vulkan {

VulkanDevice::VulkanDevice() {}

VulkanDevice::VulkanDevice(vk::UniqueDevice device) : device_(std::move(device)) {}

VulkanDevice::~VulkanDevice() {} 

std::vector<CommandBufferInterface*> VulkanDevice::allocateCommandBuffers(
    const vk::CommandBufferAllocateInfo& allocateInfo
) const noexcept {

    std::vector<vk::UniqueCommandBuffer> vkCommandBuffers = device_->allocateCommandBuffersUnique(allocateInfo, *dldi);
    std::vector<CommandBufferInterface*> commandBuffers;
    
    // Loop over the unique command buffers and wrap each in a VulkanCommandBuffer
    for (auto& vkCommandBuffer : vkCommandBuffers) {
        auto* vcd = new VulkanCommandBuffer(std::move(vkCommandBuffer)); // Transfer ownership
        vcd->setDispatch(dldi);
        commandBuffers.push_back(vcd);
    }
    return commandBuffers;
}

// allocateDescriptorSets
std::vector<vk::DescriptorSet> VulkanDevice::allocateDescriptorSets(
    const vk::DescriptorSetAllocateInfo allocateInfo) const noexcept {
    return device_->allocateDescriptorSets(allocateInfo, *dldi);
}

// allocateMemory
std::unique_ptr<MemoryInterface> VulkanDevice::allocateMemory( 
    const vk::MemoryAllocateInfo & allocateInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const {
    vk::UniqueDeviceMemory umemory = device_->allocateMemoryUnique(allocateInfo, allocator, *dldi);
    return std::make_unique<VulkanMemory>(std::move(umemory));
}

// acquireNextImageKHR
vk::ResultValue<uint32_t> VulkanDevice::acquireNextImageKHR(
    std::unique_ptr<SwapchainInterface>& swapchain,
    uint64_t timeout,
    vk::Semaphore semaphore,
    vk::Fence fence
) const noexcept {
    return device_->acquireNextImageKHR(swapchain->getVkSwapchain(), timeout, semaphore, fence, *dldi);
}

// bindBufferMemory
void VulkanDevice::bindBufferMemory(
    vk::Buffer buffer,
    std::unique_ptr<MemoryInterface>& memory,
    vk::DeviceSize memoryOffset
) const noexcept {
    device_->bindBufferMemory(buffer, memory->getMemory(), memoryOffset, *dldi);
}

// bindImageMemory
void VulkanDevice::bindImageMemory(
    vk::Image image,
    std::unique_ptr<MemoryInterface>& memory,
    vk::DeviceSize memoryOffset
) const noexcept {
    device_->bindImageMemory(image, memory->getMemory(), memoryOffset, *dldi);
}

// createAccelerationStructureKHR
vk::AccelerationStructureKHR VulkanDevice::createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR createInfo) const noexcept {
    return device_->createAccelerationStructureKHR(createInfo, nullptr, *dldi);
};

// createBufferUnique
 vk::UniqueBuffer VulkanDevice::createBufferUnique(
    const vk::BufferCreateInfo & createInfo, vk::Optional<const vk::AllocationCallbacks> allocator) const {
        return device_->createBufferUnique(createInfo, allocator, *dldi);
    }
  
// createCommandPool
std::unique_ptr<CommandPoolInterface> VulkanDevice::createCommandPool( 
    const vk::CommandPoolCreateInfo &  createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator 
) const {
    (void)allocator; // TODO Consider removing
    vk::UniqueCommandPool commandPool = device_->createCommandPoolUnique(createInfo, nullptr, *dldi);

    return std::make_unique<VulkanCommandPool>(std::move(commandPool));
}

// createDescriptorPool
vk::DescriptorPool VulkanDevice::createDescriptorPool(
    const vk::DescriptorPoolCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createDescriptorPool(createInfo, allocator, *dldi);
}

// createDescriptorSetLayout
vk::DescriptorSetLayout VulkanDevice::createDescriptorSetLayout(
    const vk::DescriptorSetLayoutCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createDescriptorSetLayout(createInfo, allocator, *dldi);
}

// createFence
vk::Fence VulkanDevice::createFence(
    const vk::FenceCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createFence(createInfo, allocator, *dldi);
}

// createFramebuffer}
vk::Framebuffer VulkanDevice::createFramebuffer(
    const vk::FramebufferCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createFramebuffer(createInfo, allocator, *dldi);
    }

// createGraphicsPipeline
vk::ResultValue<vk::Pipeline> VulkanDevice::createGraphicsPipeline(
    vk::PipelineCache pipelineCache,
    const vk::GraphicsPipelineCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createGraphicsPipeline(pipelineCache, createInfo, allocator, *dldi);
}

// createImage
vk::Image VulkanDevice::createImage(
    const vk::ImageCreateInfo & createInfo, 
    const vk::AllocationCallbacks* allocator = nullptr
) const {
    return device_->createImage(createInfo, allocator, *dldi);
}

// createImageView
vk::ImageView VulkanDevice::createImageView(
    const vk::ImageViewCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createImageView(createInfo, allocator, *dldi);}

// createSampler
vk::Sampler VulkanDevice::createSampler(
    const vk::SamplerCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createSampler(createInfo, allocator, *dldi);}

// createBufferView
vk::BufferView VulkanDevice::createBufferView(
    const vk::BufferViewCreateInfo& createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createBufferView(createInfo, allocator, *dldi);}

// createPipelineLayout
vk::PipelineLayout VulkanDevice::createPipelineLayout(
    const vk::PipelineLayoutCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createPipelineLayout(createInfo, allocator, *dldi);
}

// createRayTracingPipelineKHR
vk::ResultValue<vk::Pipeline> VulkanDevice::createRayTracingPipelineKHR(
    vk::DeferredOperationKHR deferredOperation, 
    vk::PipelineCache pipelineCache, 
    const vk::RayTracingPipelineCreateInfoKHR & createInfo, 
    const vk::AllocationCallbacks* allocator) const noexcept {
    return device_->createRayTracingPipelineKHR(deferredOperation, pipelineCache, createInfo, allocator, *dldi);
};

// createRenderPass
vk::RenderPass VulkanDevice::createRenderPass(
    const vk::RenderPassCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createRenderPass(createInfo, allocator, *dldi);
}

// createSemaphore
vk::Semaphore VulkanDevice::createSemaphore(
    const vk::SemaphoreCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return device_->createSemaphore(createInfo, allocator, *dldi);
}

// createShaderModule
vk::UniqueShaderModule VulkanDevice::createShaderModule(
    const vk::ShaderModuleCreateInfo createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    return std::move(device_->createShaderModuleUnique(createInfo, allocator, *dldi));
}

// createSwapchainKHR
std::unique_ptr<SwapchainInterface> VulkanDevice::createSwapchainKHR(
    const vk::SwapchainCreateInfoKHR createInfo,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const {
    vk::UniqueSwapchainKHR swapchain = 
    device_->createSwapchainKHRUnique<vk::DispatchLoaderDynamic>(createInfo, allocator, *dldi);
    return std::make_unique<VulkanSwapchain>(std::move(swapchain));
}

// destroyCommandPool
void VulkanDevice::destroyCommandPool(
    std::unique_ptr<CommandPoolInterface> commandPool,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    (void)allocator; // TODO Consider removing

    commandPool.reset();
}

// destroyDescriptorPool
void VulkanDevice::destroyDescriptorPool(
    vk::DescriptorPool descriptorPool,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyDescriptorPool(descriptorPool, allocator, *dldi);
}

// destroyDescriptorSetLayout
void VulkanDevice::destroyDescriptorSetLayout(
    vk::DescriptorSetLayout descriptorSetLayout,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyDescriptorSetLayout(descriptorSetLayout, allocator, *dldi);
}

// destroyFence
void VulkanDevice::destroyFence(
    vk::Fence fence,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyFence(fence, allocator, *dldi);
}

// destroyFramebuffer
void VulkanDevice::destroyFramebuffer(
    vk::Framebuffer framebuffer,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyFramebuffer(framebuffer, allocator, *dldi);
}

// destroyImage
void VulkanDevice::destroyImage(
    vk::Image image,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyImage(image, allocator, *dldi);
}

// destroyImageView
void VulkanDevice::destroyImageView(
    vk::ImageView imageView,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyImageView(imageView, allocator, *dldi);
}

// destroyBufferView
void VulkanDevice::destroyBufferView(
    vk::BufferView bufferView,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyBufferView(bufferView, allocator, *dldi);
}

// destroySampler
void VulkanDevice::destroySampler(
    vk::Sampler sampler,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroySampler(sampler, allocator, *dldi);
}

// destroyPipeline
void VulkanDevice::destroyPipeline(
    vk::Pipeline pipeline,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyPipeline(pipeline, allocator, *dldi);
}

// destroyPipelineLayout
void VulkanDevice::destroyPipelineLayout(
    vk::PipelineLayout pipelineLayout,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyPipelineLayout(pipelineLayout, allocator, *dldi);
}

// destroyRenderPass
void VulkanDevice::destroyRenderPass(
    vk::RenderPass renderPass,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyRenderPass(renderPass, allocator, *dldi);
}

// destroySemaphore
void VulkanDevice::destroySemaphore(
    vk::Semaphore semaphore,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroySemaphore(semaphore, allocator, *dldi);
}

// destroyShaderModule
void VulkanDevice::destroyShaderModule(
    vk::ShaderModule shaderModule,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->destroyShaderModule(shaderModule, allocator, *dldi);
}

// flushMappedMemoryRanges
void VulkanDevice::flushMappedMemoryRanges( 
    const std::vector<vk::MappedMemoryRange>& pMemoryRanges) const noexcept {
    device_->flushMappedMemoryRanges(pMemoryRanges, *dldi);
}

// freeMemory
void VulkanDevice::freeMemory( 
    std::unique_ptr<MemoryInterface> memory,
    vk::Optional<const vk::AllocationCallbacks> allocator
) const noexcept {
    device_->freeMemory(memory->getMemory(), allocator, *dldi);
}
// getAccelerationStructureBuildSizesKHR
vk::AccelerationStructureBuildSizesInfoKHR VulkanDevice::getAccelerationStructureBuildSizesKHR(
    vk::AccelerationStructureBuildTypeKHR buildType, 
    const vk::AccelerationStructureBuildGeometryInfoKHR & buildInfo,
    vk::ArrayProxy<const uint32_t> const & maxPrimitiveCounts
    ) const noexcept {
        return device_->getAccelerationStructureBuildSizesKHR(buildType, buildInfo, maxPrimitiveCounts, *dldi);
    }

// getBufferAddress
vk::DeviceAddress VulkanDevice::getBufferAddress(
    vk::BufferDeviceAddressInfo addressInfo
) const noexcept {
    return device_->getBufferAddress(addressInfo, *dldi);
}

// getBufferMemoryRequirements
vk::MemoryRequirements VulkanDevice::getBufferMemoryRequirements( 
    vk::Buffer buffer
) const noexcept {
    return device_->getBufferMemoryRequirements(buffer, *dldi);
}

//getImageMemoryRequirements
vk::MemoryRequirements VulkanDevice::getImageMemoryRequirements(
    vk::Image image
) const {
    return device_->getImageMemoryRequirements(image, *dldi);
}

// getQueue
QueueInterface* VulkanDevice::getQueue( 
    uint32_t queueFamilyIndex, 
    uint32_t queueIndex
) const noexcept {

    vk::Queue queue = device_->getQueue(queueFamilyIndex, queueIndex, *dldi);
    return new VulkanQueue(queue, dldi);
}

// getSwapchainImagesKHR
std::vector<vk::Image> VulkanDevice::getSwapchainImagesKHR( 
    std::unique_ptr<SwapchainInterface>& swapchain
    ) const {
    return device_->getSwapchainImagesKHR(swapchain->getVkSwapchain(), *dldi);
}

// invalidateMappedMemoryRanges
vk::Result VulkanDevice::invalidateMappedMemoryRanges( 
    uint32_t memoryRangeCount, 
    const vk::MappedMemoryRange * pMemoryRanges
) const noexcept {
    return device_->invalidateMappedMemoryRanges(memoryRangeCount, pMemoryRanges, *dldi);
}

// mapMemory
void* VulkanDevice::mapMemory(
    MemoryInterface*   memory,
    vk::DeviceSize     offset,
    vk::DeviceSize     size,
    vk::MemoryMapFlags flags
) const noexcept {
    try {
        return device_->mapMemory(memory->getMemory(), offset, size, flags, *dldi);    
    }
    catch (const vk::SystemError& err) {
        std::cerr << "Failed to map device memory: " << err.what() << std::endl;
        return nullptr;        
    }
}

// resetDescriptorPool
void VulkanDevice::resetDescriptorPool(
    vk::DescriptorPool descriptorPool,
    vk::DescriptorPoolResetFlags flags
) const noexcept {
    return device_->resetDescriptorPool(descriptorPool, flags, *dldi);
}

// resetFences
void VulkanDevice::resetFences(
    vk::ArrayProxy<const vk::Fence> const & fences    
) const noexcept {
    device_->resetFences(fences, *dldi);
}

// unmapMemory
void VulkanDevice::unmapMemory(
    std::unique_ptr<MemoryInterface>& memory
) const noexcept {
    device_->unmapMemory(memory->getMemory(), *dldi);
}

// updateDescriptorSets
void VulkanDevice::updateDescriptorSets(
    const std::vector<vk::WriteDescriptorSet> & descriptorWrites,
    const std::vector<vk::CopyDescriptorSet>& descriptorCopies
) const noexcept {
    device_->updateDescriptorSets(descriptorWrites, descriptorCopies, *dldi);
}

// waitForFences
vk::Result VulkanDevice::waitForFences(
    vk::ArrayProxy<const vk::Fence> const & fences,    
    vk::Bool32 waitAll,
    uint64_t timeout
) const noexcept {
    return device_->waitForFences(fences, waitAll, timeout, *dldi);
}

// waitIdle
void VulkanDevice::waitIdle() const {
    device_->waitIdle(*dldi); 
}

// getDevice
const vk::Device& VulkanDevice::getDevice() const {
    return *device_;
}

// getRayTracingShaderGroupHandlesKHR
vk::Result VulkanDevice::getRayTracingShaderGroupHandlesKHR( 
    vk::Pipeline pipeline, 
    uint32_t  firstGroup, 
    uint32_t groupCount, 
    size_t dataSize, 
    void * pData) const {

        return device_->getRayTracingShaderGroupHandlesKHR(pipeline, firstGroup, groupCount, dataSize, pData, *dldi);
}

// getAccelerationStructureBuildSizesKHR
vk::DeviceAddress VulkanDevice::getAccelerationStructureAddressKHR(
    const vk::AccelerationStructureDeviceAddressInfoKHR& info) const {
        return device_->getAccelerationStructureAddressKHR(info, *dldi);
}  

const vk::UniqueDevice& VulkanDevice::getUniqueDevice() const {return device_;};


} 




