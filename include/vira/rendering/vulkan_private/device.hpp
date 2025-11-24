#ifndef VIRA_RENDERING_VULKAN_DEVICE_HPP
#define VIRA_RENDERING_VULKAN_DEVICE_HPP

#include <string>
#include <vector>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/window.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance_factory.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"

namespace vira::vulkan {

    /**
     * @struct SwapchainSupportDetails
     * @brief Holds format, modes, and capabilities of a vulkan swapchain
     */	
    struct SwapchainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    /**
     * @struct QueueFamilyIndices
     * @brief Holds queue family indices for the graphicsFamily and presentFamily and
     * checks if queue families are complete.
     */	
    struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
    };
 
    /**
     * @class ViraDevice
     * @brief Manages vulkan instance, physical device, and logical device functionality
     *
     * @details The ViraDevice class manages the creation, usage, and interface with the Vulkan GPU physical device and logical device.
     * The ViraDevice is responsible for querying the host system to determine specific GPU/Vulkan capabilities and requirements, and 
     * uses those to create a compatible Vulkan Logical Device that manages and executes those interfaces and resources.
     *
     * A Vulkan Instance represents the connection between Vira and the Vulkan library. It initializes Vulkan and sets up the environment for all subsequent Vulkan operations.
     * A Vulkan Physical Device is the actual GPU devices available on the host machine.
     * A Vulkan Device represents a logical connection to a physical GPU. It manages resources and issues commands to the GPU, enabling rendering and compute operations.
     * 
     * @tparam TFloat The float type used by Vira, which obeys the "IsFloat" concept.
     * @tparam TSpectral The spectral type of light used in the render, such as the material albedos, the light irradiance, etc.
     * This parameter obeys the "IsSpectral" concept.
     * @tparam TMeshFloat The float type used in the meshes.
     * 
     * @param[in] instanceFactory (InstanceFactoryInterface) A factory class to create either a real or mock Vulkan instance.
     * @param[in] deviceFactory (DeviceFactoryInterface) A factory class to create either a real or mock Vulkan logical device.
     *
     * ### Member Variables:
     * 
     * - **Instance and Device Factories:**
     *   - `instanceFactory_`: Factory for creating Vulkan instances.
     *   - `deviceFactory_`: Factory for creating Vulkan devices.
     *
     * - **Vulkan Dispatch and Loaders:**
     *   - `dldi`: Pointer to Vulkan dispatch loader for dynamic function loading.
     *   - `dl`: Vulkan dynamic loader for fetching instance-level functions.
     *
     * - **Physical Device Properties:**
     *   - `physicalDeviceProperties`: General properties of the selected physical device.
     *   - `accelStructProperties`: Properties related to acceleration structures.
     *   - `rayTracingProperties`: Ray tracing pipeline-specific properties.
     *
     * - **Vulkan Instance and Debugging:**
     *   - `instance`: Unique pointer to the Vulkan instance.
     *   - `debugMessenger`: Debug callback for Vulkan validation layers.
     *
     * - **Vulkan Device and Related Resources:**
     *   - `device_`: Unique pointer to the logical Vulkan device.
     *   - `physicalDevice`: Unique pointer to the selected physical device.
     *   - `commandPool`: Command pool for managing command buffers.
     *
     * - **Window and Surface:**
     *   - `window`: Pointer to the application window.
     *   - `surface_`: Vulkan surface associated with the window.
     *
     * - **Queues for Command Submission:**
     *   - `graphicsQueue_`: Pointer to the graphics queue.
     *   - `presentQueue_`: Pointer to the presentation queue.
     *
     * - **Validation Layers and Extensions:**
     *   - `requiredValidationLayers`: List of validation layers to enable.
     *   - `requiredDeviceExtensions`: List of required Vulkan device extensions.
     *
     * @see InstanceFactoryInterface, DeviceFactoryInterface, 
     */    
    class ViraDevice {
    public:
        
        #ifdef NDEBUG
                const bool enableValidationLayers = false;
        #else
                const bool enableValidationLayers = true;
        #endif

        // Constructors & Destructor
        ViraDevice();
 
        ViraDevice(ViraWindow* window);
 
        ViraDevice( std::shared_ptr<InstanceFactoryInterface> instanceFactory,
                    std::shared_ptr<DeviceFactoryInterface> deviceFactory);                    

        ViraDevice( std::shared_ptr<InstanceFactoryInterface> instanceFactory,
                    std::shared_ptr<DeviceFactoryInterface> deviceFactory,
                    ViraWindow* window);

        ~ViraDevice();

        // Delete Copy Methods
        ViraDevice(const ViraDevice&) = delete;
        void operator=(const ViraDevice&) = delete;
        ViraDevice(ViraDevice&&) = delete;
        ViraDevice& operator=(ViraDevice&&) = delete;

        // Vulkan Dispatch
        void createDispatchLoaderDynamic();

        /**
         * @brief Create a debug messenger to return vulkan validation layer messages.
         */          
        virtual void CreateDebugUtilsMessengerEXT(
            const vk::DebugUtilsMessengerCreateInfoEXT& createInfo,
            const vk::Optional<const vk::AllocationCallbacks> allocator);

        // Retrieve methods
        ViraWindow* getWindow() { return window; }
        const CommandPoolInterface& getCommandPool() { return *commandPool; }
        vk::UniqueDebugUtilsMessengerEXT& getDebugMessenger() {return debugMessenger;};
        vk::SurfaceKHR surface() { return surface_; }
        QueueInterface* graphicsQueue() { return graphicsQueue_; }
        QueueInterface* presentQueue() { return presentQueue_; }
        vk::DeviceAddress getBufferAddress(vk::BufferDeviceAddressInfo addressInfo) {
            return device_->getBufferAddress(addressInfo);
        };
        InstanceInterface& getInstance() const {return *instance; }
        std::unique_ptr<DeviceInterface>& device() { return device_; }
        const vk::Device& getVkDevice();
        const vk::PhysicalDevice& getVkPhysicalDevice();
        const std::unique_ptr<PhysicalDeviceInterface>& getPhysicalDevice();
        vk::PhysicalDeviceProperties getPhysicalDeviceProperties() {return physicalDeviceProperties;};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR getAccelStructProperties() {return accelStructProperties;};
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR getRayTracingProperties() {return rayTracingProperties;};

        // Query vulkan support / types
        virtual uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);        
        virtual QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(*physicalDevice); }        
        virtual vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        virtual SwapchainSupportDetails getSwapchainSupport() { return querySwapchainSupport(*physicalDevice); }        

        // Buffer/Image Helper Functions

        /**
         * @brief Create a Vulkan Buffer given properties and usage
         */  
        std::unique_ptr<MemoryInterface>  createBuffer(
            vk::DeviceSize size,
            vk::BufferUsageFlags usage,
            vk::MemoryPropertyFlags properties,
            vk::UniqueBuffer& buffer);


        /**
         * @brief Create a vulkan image given image create info, properties, and image object and memory
         */              
        virtual void createImageWithInfo(
            const vk::ImageCreateInfo& imageInfo,
            vk::MemoryPropertyFlags properties,
            vk::Image& image,
            std::unique_ptr<MemoryInterface>& imageMemory);

        /**
         * @brief Transition a vulkan image from 'oldLayout' to 'newLayout'
         * 
         * These transitions are needed to put the image in a transferable/copyable state and back
         * to vulkan pipeline usable states.
         */              
        void transitionImageLayout( 
            vk::Image image,                  // Image to transition
            vk::ImageLayout oldLayout,        // Current layout of the image
            vk::ImageLayout newLayout,        // Target layout
            vk::PipelineStageFlags srcStageMask, // Source pipeline stage
            vk::PipelineStageFlags dstStageMask,  // Destination pipeline stage
            size_t layerCount = 1,
            vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor
        );

        /**
         * @brief Copy the contents of a vk::Buffer to another vk::Buffer
         */   
        void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

        /**
         * @brief Copy the contents of a vk::Buffer to a vk::Image
         */   
        void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, uint32_t layerCount = 1, uint32_t base_layer = 0);

        /**
         * @brief Copy the contents of a vk::Image to a vk::Buffer
         */   
        void copyImageToBuffer(vk::Image image, vk::Buffer buffer, uint32_t width, uint32_t height, uint32_t layerCount = 1, uint32_t base_layer = 0, vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor);

        /**
         * @brief Extract the vec4 data in a vk::Image used for color type data. 
         */   
        std::vector<vira::vec4<float>> extractSpectralSetData(vk::Image image, Resolution resolution);

        /**
         * @brief Extract the scalar float data in a vk::Image used for depth and other scalar data. 
         */   
        std::vector<float> extractScalarFloatImageData(vk::Image image, Resolution resolution, bool isDepth = FALSE); 

        /**
         * @brief Load a shader .spv file and construct a shader module
         */   
        vk::UniqueShaderModule createShaderModule(const std::vector<char>& code);

        /**
         * @brief Begin writing single time use commands to a command buffer, and return
         * the command buffer interface object.
         */   
        virtual CommandBufferInterface* beginSingleTimeCommands();

        /**
         * @brief Complete writing single time use commands to commandBuffer and submit them (to the particular
         * queue if specified)
         */   
        virtual void endSingleTimeCommands(CommandBufferInterface* commandBuffer,  QueueInterface* queue = nullptr);

        // Vulkan Initialization Methods
        /**
         * @brief Create a Vulkan Instance
         */   
        void createInstance();
        
        /**
         * @brief Make debug messenger create info and build debug messenger to return Vulkan validation layer messages.
         */   
        void setupDebugMessenger();

        /**
         * @brief Create a window surface for onscreen rendering (not yet implemented)
         */   
        void createSurface();

        /**
         * @brief Choose a vulkan gpu physical device to use to construct the vulkan logical device.
         */   
        void pickPhysicalDevice();

        /**
         * @brief Create a vulkan logical device using the selected physical device.
         */   
        void createLogicalDevice();

        /**
         * @brief Create a command pool to allocate command buffers used to issue Vulkan commands.
         */   
        void createCommandPool();

        // Public Member Variables
        vk::DispatchLoaderDynamic* dldi;
        std::shared_ptr<InstanceFactoryInterface> instanceFactory_;
        std::shared_ptr<DeviceFactoryInterface> deviceFactory_;

    protected:
        friend class TestableViraDevice;

        /**
         * @brief Populate a debug messenger create info with default required extensions, flags, and validation layers.
         */   
        virtual void populateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

        /**
         * @brief Query validation layer support on local physical device.
         */        
        bool checkValidationLayerSupport();
        /**
         * @brief Query instance extension support on local physical device.
         */        
        bool checkInstanceExtensionSupport(std::vector<const char*> requiredExtensions); 
        
        /**
         * @brief Query device extension support on local physical device.
         */        
        virtual bool checkDeviceExtensionSupport(PhysicalDeviceInterface& device);
        
        /**
         * @brief Query swapchain support on local physical device.
         */        
        virtual SwapchainSupportDetails querySwapchainSupport(PhysicalDeviceInterface& pdevice);

        /**
         * @brief Query required instance extensions for vulkan instance on local platform.
         */        
        std::vector<const char*> getRequiredInstanceExtensions();

        /**
         * @brief Query Physical Device properties.
         */        
        void queryPhysicalDeviceProperties2();

        /**
         * @brief Find available queue families.
         */        
        virtual QueueFamilyIndices findQueueFamilies(PhysicalDeviceInterface& device);

        /**
         * @brief Rate physical device suitability for Vira configuration needs.
         */        
        virtual int rateDeviceSuitability(PhysicalDeviceInterface& device);

        // Member Variables
        vk::DynamicLoader dl;
        std::unique_ptr<InstanceInterface> instance;
        std::unique_ptr<DeviceInterface> device_;
        std::unique_ptr<PhysicalDeviceInterface> physicalDevice = VK_NULL_HANDLE;
        ViraWindow* window;
        vk::UniqueDebugUtilsMessengerEXT debugMessenger;
        std::unique_ptr<CommandPoolInterface> commandPool;
        vk::SurfaceKHR surface_;
        QueueInterface* graphicsQueue_ = nullptr;
        QueueInterface* presentQueue_ = nullptr;

        // Device Properties
        vk::PhysicalDeviceProperties physicalDeviceProperties;
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR accelStructProperties;
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties;

        // Required Validation Layers and Device Extensions
        const std::vector<const char*> requiredValidationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"};
        const std::vector<const char*> requiredDeviceExtensions = { 
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, 
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_MAINTENANCE3_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, 
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME, 
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME           
        };

    };

}

#include "implementation/rendering/vulkan_private/device.ipp"

#endif