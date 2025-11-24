#ifndef VULKAN_DEVICE_HPP
#define VULKAN_DEVICE_HPP
#undef VULKAN_HPP_DISABLE_ENHANCED_MODE

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"

namespace vira::vulkan {

class VulkanDevice : public DeviceInterface {

    public:
        VulkanDevice();

        VulkanDevice(vk::UniqueDevice device);

        ~VulkanDevice() override;  // Override the virtual destructor

        // allocateCommandBuffers
        std::vector<CommandBufferInterface*> allocateCommandBuffers(
            const vk::CommandBufferAllocateInfo& pAllocateInfo
        ) const noexcept override;

        // allocateDescriptorSets
        std::vector<vk::DescriptorSet> allocateDescriptorSets(
            const vk::DescriptorSetAllocateInfo allocateInfo
        ) const noexcept override;

        // allocateMemory
        std::unique_ptr<MemoryInterface> allocateMemory( 
            const vk::MemoryAllocateInfo & allocateInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
            ) const override;

        // acquireNextImageKHR
        vk::ResultValue<uint32_t> acquireNextImageKHR(
            std::unique_ptr<SwapchainInterface>& swapchain,
            uint64_t timeout,
            vk::Semaphore semaphore,
            vk::Fence fence
        ) const noexcept override;

        // bindBufferMemory
        void bindBufferMemory(
            vk::Buffer buffer,
            std::unique_ptr<MemoryInterface>& memory,
            vk::DeviceSize memoryOffset
        ) const noexcept override;

        // bindImageMemory
        void bindImageMemory(
            vk::Image image,
            std::unique_ptr<MemoryInterface>& memory,
            vk::DeviceSize memoryOffset
        ) const noexcept override;

        // createAccelerationStructureKHR
        vk::AccelerationStructureKHR createAccelerationStructureKHR(vk::AccelerationStructureCreateInfoKHR createInfo) const noexcept override;
        
        // createBufferUnique
        vk::UniqueBuffer createBufferUnique(
            const vk::BufferCreateInfo & createInfo, vk::Optional<const vk::AllocationCallbacks> allocator) const override;

        // createCommandPool
        std::unique_ptr<CommandPoolInterface> createCommandPool( 
            const vk::CommandPoolCreateInfo &       createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator 
        ) const override;

        // createDescriptorPool
        vk::DescriptorPool createDescriptorPool(
            const vk::DescriptorPoolCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createDescriptorSetLayout
        vk::DescriptorSetLayout createDescriptorSetLayout(
            const vk::DescriptorSetLayoutCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createFence
        vk::Fence createFence(
            const vk::FenceCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createFramebuffer
        vk::Framebuffer createFramebuffer(
            const vk::FramebufferCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createGraphicsPipelines
        virtual vk::ResultValue<vk::Pipeline> createGraphicsPipeline(
            vk::PipelineCache pipelineCache,
            const vk::GraphicsPipelineCreateInfo createInfos,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createImage
        vk::Image createImage(
            const vk::ImageCreateInfo & createInfo, 
            const vk::AllocationCallbacks* allocator
        ) const override;
                
        // createImageView
        vk::ImageView createImageView(
            const vk::ImageViewCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createSampler
        vk::Sampler createSampler(
            const vk::SamplerCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createBufferView
        vk::BufferView createBufferView(
            const vk::BufferViewCreateInfo& createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createPipelineLayout
        vk::PipelineLayout createPipelineLayout(
            const vk::PipelineLayoutCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createRayTracingPipelineKHR
        vk::ResultValue<vk::Pipeline> createRayTracingPipelineKHR(
            vk::DeferredOperationKHR deferredOperation, 
            vk::PipelineCache pipelineCache, 
            const vk::RayTracingPipelineCreateInfoKHR & createInfo, 
            const vk::AllocationCallbacks* allocator
        ) const noexcept override;

        // createRenderPass
        vk::RenderPass createRenderPass(
            const vk::RenderPassCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createSemaphore
        vk::Semaphore createSemaphore(
            const vk::SemaphoreCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createShaderModule
        vk::UniqueShaderModule createShaderModule(
            const vk::ShaderModuleCreateInfo createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // createSwapchainKHR
        std::unique_ptr<SwapchainInterface> createSwapchainKHR(
            const vk::SwapchainCreateInfoKHR createInfo,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const override;
        
        // destroyCommandPool
        void destroyCommandPool(
            std::unique_ptr<CommandPoolInterface> commandPool,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyDescriptorPool
        void destroyDescriptorPool(
            vk::DescriptorPool descriptorPool,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyDescriptorSetLayout
        void destroyDescriptorSetLayout(
            vk::DescriptorSetLayout descriptorSetLayout,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyFence
        void destroyFence(
            vk::Fence fence,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyFramebuffer
        void destroyFramebuffer(
            vk::Framebuffer framebuffer,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyImage
        void destroyImage(
            vk::Image image,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyImageView
        void destroyImageView(
            vk::ImageView imageView,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyBufferView
        void destroyBufferView(
            vk::BufferView bufferView,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroySampler
        void destroySampler(
            vk::Sampler sampler,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyPipeline
        void destroyPipeline(
            vk::Pipeline pipeline,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyPipelineLayout
        void destroyPipelineLayout(
            vk::PipelineLayout pipelineLayout,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyRenderPass
        void destroyRenderPass(
            vk::RenderPass renderPass,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroySemaphore
        void destroySemaphore(
            vk::Semaphore semaphore,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // destroyShaderModule
        void destroyShaderModule(
            vk::ShaderModule shaderModule,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;

        // // destroySwapchainKHR
        // void destroySwapchainKHR(
        //     std::unique_ptr<SwapchainInterface> swapchain,
        //     vk::Optional<const vk::AllocationCallbacks> allocator
        // ) const noexcept override;

        // flushMappedMemoryRanges
        void flushMappedMemoryRanges( 
            const std::vector<vk::MappedMemoryRange>& pMemoryRanges
        ) const noexcept override;

        // freeMemory
        void freeMemory( 
            std::unique_ptr<MemoryInterface> memory,
            vk::Optional<const vk::AllocationCallbacks> allocator
        ) const noexcept override;
    
        // getAccelerationStructureBuildSizesKHR
        vk::AccelerationStructureBuildSizesInfoKHR getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR buildType, 
            const vk::AccelerationStructureBuildGeometryInfoKHR & buildInfo,
            vk::ArrayProxy<const uint32_t> const & maxPrimitiveCounts
            ) const noexcept override;

        // getBufferAddress
        vk::DeviceAddress getBufferAddress(
            vk::BufferDeviceAddressInfo addressInfo
        ) const noexcept override;
        
        // getBufferMemoryRequirements
        vk::MemoryRequirements getBufferMemoryRequirements( 
            vk::Buffer buffer
        ) const noexcept override;
        
        //getImageMemoryRequirements
        vk::MemoryRequirements getImageMemoryRequirements(
            vk::Image image
        ) const override;

        // getQueue
        QueueInterface* getQueue( 
            uint32_t queueFamilyIndex, 
            uint32_t queueIndex 
        ) const noexcept override;       

        // getSwapchainImagesKHR
        std::vector<vk::Image> getSwapchainImagesKHR( 
            std::unique_ptr<SwapchainInterface>& swapchain 
        ) const override;

        // invalidateMappedMemoryRanges
        vk::Result invalidateMappedMemoryRanges( 
            uint32_t memoryRangeCount, 
            const vk::MappedMemoryRange * pMemoryRanges) const noexcept override;
            
        // mapMemory
        void* mapMemory(
            MemoryInterface*   memory,
            vk::DeviceSize     offset,
            vk::DeviceSize     size,
            vk::MemoryMapFlags flags
        ) const noexcept override;

        // resetDescriptorPool
        void resetDescriptorPool(
            vk::DescriptorPool descriptorPool,
            vk::DescriptorPoolResetFlags flags
        ) const noexcept override;

        // resetFences
        void resetFences(
            vk::ArrayProxy<const vk::Fence> const & fences    
        ) const noexcept override;

        // unmapMemory
        void unmapMemory(
            std::unique_ptr<MemoryInterface>& memory
        ) const noexcept override;

        // updateDescriptorSets
        void updateDescriptorSets(
            const std::vector<vk::WriteDescriptorSet> & descriptorWrites,
            const std::vector<vk::CopyDescriptorSet>& descriptorCopies
        ) const noexcept override;

        // waitForFences
        vk::Result waitForFences(
            vk::ArrayProxy<const vk::Fence> const & fences,    
            vk::Bool32 waitAll,
            uint64_t timeout
        ) const noexcept override;

        // waitIdle
        void waitIdle() const override;

        const vk::Device& getDevice() const override;

        // getRayTracingShaderGroupHandlesKHR
        virtual vk::Result getRayTracingShaderGroupHandlesKHR( 
            vk::Pipeline pipeline, 
            uint32_t  firstGroup, 
            uint32_t groupCount, 
            size_t dataSize, 
            void * pData ) const override;

        // getAccelerationStructureBuildSizesKHR
        virtual vk::DeviceAddress getAccelerationStructureAddressKHR(
            const vk::AccelerationStructureDeviceAddressInfoKHR& info
        ) const override;   
        
        void setDispatch(vk::DispatchLoaderDynamic* newDldi) override {this->dldi = newDldi;};

        virtual const vk::UniqueDevice& getUniqueDevice() const override;

    private:
        vk::UniqueDevice device_;
        vk::DispatchLoaderDynamic* dldi;
    };

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/device/vulkan_device.ipp"
#endif // VULKAN_DEVICE_HPP
