#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>
#include <map>

#include "vulkan/vulkan.hpp"
#include "GLFW/glfw3.h"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"

namespace vira::vulkan {

    // Define Debug Messenger callback function
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        (void)pUserData; // TODO Consider removing

        if (!(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
            std::cerr << "\nValidation Layer: ";
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) std::cerr << "[WARNING] ";
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) std::cerr << "[ERROR] ";

            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) std::cerr << "[GENERAL] ";
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) std::cerr << "[VALIDATION] ";
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) std::cerr << "[PERFORMANCE] ";

            std::cerr << pCallbackData->pMessage << std::endl;

            // Print objects (if any) associated with the message
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
                std::cerr << "Object[" << i << "] Handle: " 
                        << pCallbackData->pObjects[i].objectHandle
                        << " Type: " << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
                        << std::endl;
            }

            // Breakpoint on severe issues
            if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                std::abort(); // Use debugger to trace the error location
            }        
        }

        return VK_FALSE;
    }

    ViraDevice::ViraDevice() {};
    ViraDevice::ViraDevice(ViraWindow* window): window{window} {};
    ViraDevice::ViraDevice( std::shared_ptr<InstanceFactoryInterface> instanceFactory,
                            std::shared_ptr<DeviceFactoryInterface> deviceFactory,
                            ViraWindow* window) :   instanceFactory_{instanceFactory}, 
                                                    deviceFactory_{deviceFactory}, 
                                                    window{window} {
        createDispatchLoaderDynamic();
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPool();
    }

    ViraDevice::ViraDevice( std::shared_ptr<InstanceFactoryInterface> instanceFactory,
                            std::shared_ptr<DeviceFactoryInterface> deviceFactory) :   instanceFactory_{instanceFactory}, 
                                                    deviceFactory_{deviceFactory} {
        createDispatchLoaderDynamic();
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
        createCommandPool();

    }

    ViraDevice::~ViraDevice() {

        // Destroy the window surface
        if (surface_) {
            instance->destroySurfaceKHR(surface_);
            surface_ = nullptr;
        }

        // Destroy QueueInterface* properties
        if (graphicsQueue_) {
            delete graphicsQueue_;
            graphicsQueue_ = nullptr;
        }
        if (presentQueue_) {
            delete presentQueue_;
            presentQueue_ = nullptr;
        }

        commandPool.reset();
        debugMessenger.reset();  

        device_.reset();
        instance.reset();
        physicalDevice.reset();
        
    }

    void ViraDevice::createDispatchLoaderDynamic() {

        // Create dynamic loader
        std::string vulkan_library_path = 
        std::string( std::getenv("VULKAN_SDK") ) + "/lib/libvulkan.so";
        vk::DynamicLoader dl(vulkan_library_path.c_str()); 

        // Load vkGetInstanceProcAddr from library using dynamic loader
        auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        if (!vkGetInstanceProcAddr) {
            throw std::runtime_error("Failed to retrieve vkGetInstanceProcAddr!");
        }        

        // Create distpatcher
        dldi = new vk::DispatchLoaderDynamic(vkGetInstanceProcAddr);

        std::cout << "\nCreated Dynamic Dispatch Loader" << std::endl;

        return;
    }

    const vk::Device& ViraDevice::getVkDevice() {
        const vk::Device& rawdevice = device_->getDevice();
        return rawdevice;
    };
    
    const vk::PhysicalDevice& ViraDevice::getVkPhysicalDevice() {
        const vk::PhysicalDevice& rawdevice = physicalDevice->getDevice();
        return rawdevice;
    };
    
    const std::unique_ptr<PhysicalDeviceInterface>& ViraDevice::getPhysicalDevice() {
        return physicalDevice;
    };

    void ViraDevice::createInstance() {
        
        std::cout << "Creating Vulkan Instance"  << std::endl;

        // Create application info and populate
        vk::ApplicationInfo appInfo = {};
        appInfo.pApplicationName = "LittleVulkanEngine App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        // Create instance create info and populate
        vk::InstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.pApplicationInfo = &appInfo;

        // Retrieve required instance extensions and set in create info
        auto requiredInstanceExtensions = getRequiredInstanceExtensions();
        instanceCreateInfo.flags = vk::InstanceCreateFlags{};
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

        // Set create info for debug messenger 
        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        
        // Set validation layers
        if (enableValidationLayers) {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = requiredValidationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            instanceCreateInfo.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            instanceCreateInfo.enabledLayerCount = 0;
            instanceCreateInfo.pNext = nullptr;
        }
        
        // Create Vulkan Instance
        try {
            instance = instanceFactory_->createInstance(instanceCreateInfo, dldi);
            std::cout << "Extensions Found" << std::endl;
            std::cout << "Successfully created Vulkan instance" << std::endl;

        }
        catch (const std::exception& err) {
            std::cerr << "Vulkan error: " << err.what() << std::endl;
        }

        // Initialize Dynamic Dispatcher with created Instance
        dldi->init(instance->get()); 

    }

    void ViraDevice::pickPhysicalDevice()
    {

        std::cout << "\nFinding Suitable GPU Device..." << std::endl;

        // Get list of physical devices on machine
        std::vector<PhysicalDeviceInterface*> pdevices = instance->enumeratePhysicalDevices();
        std::multimap<int, PhysicalDeviceInterface*> candidates;
        std::cout << "\nAvailable GPU devices:" << std::endl;

        // Loop through physical devices and score their suitablility
        for ( auto& pdevice : pdevices) {

            vk::PhysicalDeviceProperties deviceProperties = pdevice->getProperties();
            std::cout << " === " << deviceProperties.deviceName << " === " << std::endl;
            
            std::cout << "Max vertex input attributes: " << deviceProperties.limits.maxVertexInputAttributes << std::endl;
            std::cout << "Max vertex input attribute offset: " << deviceProperties.limits.maxVertexInputAttributeOffset << std::endl;
            std::cout << "Max vertex input bindings: " << deviceProperties.limits.maxVertexInputBindings << std::endl;

            std::cout << "Device Vulkan API Version: " 
                      << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
                      << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
                      << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;

            PhysicalDeviceInterface& rdevice = *pdevice;
            int score = rateDeviceSuitability(rdevice);

            candidates.insert(std::pair<int, PhysicalDeviceInterface*>(score, pdevice));            
            
            std::cout <<  "Device Score: " << score << std::endl;
        }

        // Choose most suitable physical device
        if (candidates.rbegin()->first > 0) {
            physicalDevice.reset(candidates.rbegin()->second);
            pdevices.clear();
            candidates.clear();
        }
        else {
            pdevices.clear(); 
            candidates.clear();
            throw std::runtime_error("failed to find a suitable GPU!");
        }

    };

    int ViraDevice::rateDeviceSuitability(PhysicalDeviceInterface& pdevice)
    {

        // Get Physical Device properties and features
        vk::PhysicalDeviceProperties deviceProperties = pdevice.getProperties();
        vk::PhysicalDeviceFeatures deviceFeatures = pdevice.getFeatures();

        int score = 1;

        // Check for discrete GPU and weight it more:
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }

        // Check if required device extensions are supported:
        bool extensionsSupported = checkDeviceExtensionSupport(pdevice);
        if (extensionsSupported){
            std::cout << "\tDevice Extensions ARE Supported" << std::endl;
 
        } else {
            std::cout << "\tDevice Extensions NOT Supported" << std::endl;
            return -1;
        };

        // Check if swap chain is adequate:
        bool swapChainAdequate = false;
        if(surface_) {
            if (extensionsSupported) {
                
                SwapchainSupportDetails swapChainSupport = querySwapchainSupport(pdevice);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
                if(!swapChainAdequate){
                    std::cout << "\tSwapChain NOT adequate" << std::endl;
                } else {
                    std::cout << "\tSwapChain IS adequate" << std::endl;

                }
            }             

        } else {
            swapChainAdequate = true;
        }
        
        // Check if sampler anisotropy supported:
        if (!deviceFeatures.samplerAnisotropy) {
            std::cout << "\tSamplerAnisotropy NOT supported" << std::endl;            
            return -1;
        } else {
            std::cout << "\tSamplerAnisotropy IS supported" << std::endl;            
        }

        // Check for Queue Family Completeness
        QueueFamilyIndices indices = findQueueFamilies(pdevice);

        if (indices.graphicsFamilyHasValue) {
            std::cout << "\tGraphics Queue Family FOUND" << std::endl;
        } else {
            std::cout << "\tGraphics Queue Family MISSING" << std::endl;
        }
        if (indices.presentFamilyHasValue) {
            std::cout << "\tPresent Queue Family FOUND" << std::endl;
        } else {
            std::cout << "\tPresent Queue Family MISSING" << std::endl;
        }
        
        bool queueFamiliesHaveValues;
        if(surface_) {
            queueFamiliesHaveValues = indices.isComplete();
        } else {
            queueFamiliesHaveValues = indices.graphicsFamilyHasValue;
        }
        if (!queueFamiliesHaveValues || !extensionsSupported || !swapChainAdequate) {
            // If no supportable queue family, return negative score
            std::cout << "\tQueue Families  NOT supported" << std::endl;            
            return -1;
        } else {
            std::cout << "\tQueue Families  ARE supported" << std::endl;            
        }

        return score;
    };
    
    void ViraDevice::createLogicalDevice() {

        queryPhysicalDeviceProperties2();

        std::cout << "\nCreating Vulkan Logical Device Using:\t " << physicalDeviceProperties.deviceName << std::endl;

        // Determine Queue families
        QueueFamilyIndices indices = findQueueFamilies(*physicalDevice);
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

        // Set Graphics queue
        float queuePriority = 1.0f;        
        vk::DeviceQueueCreateInfo graphicsQueueCreateInfo = {};
        graphicsQueueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
        graphicsQueueCreateInfo.queueCount = 1;
        graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(graphicsQueueCreateInfo);

        // Retrieve Physical Device Features
        auto featureChain = physicalDevice->getDevice().getFeatures2<
            vk::PhysicalDeviceFeatures2, 
            vk::PhysicalDeviceRayTracingPipelineFeaturesKHR, 
            vk::PhysicalDeviceRayQueryFeaturesKHR, 
            vk::PhysicalDeviceBufferDeviceAddressFeatures,
            vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
            vk::PhysicalDeviceDescriptorIndexingFeaturesEXT>(*dldi);

        auto& deviceFeatures2     = const_cast<vk::PhysicalDeviceFeatures2&>(featureChain.get<vk::PhysicalDeviceFeatures2>());
        auto& rayTracingFeatures  = const_cast<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR&>(featureChain.get<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>());
        auto& rayQueryFeatures    = const_cast<vk::PhysicalDeviceRayQueryFeaturesKHR&>(featureChain.get<vk::PhysicalDeviceRayQueryFeaturesKHR>());
        auto& bufferDeviceAddressFeatures    = const_cast<vk::PhysicalDeviceBufferDeviceAddressFeatures&>(featureChain.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>());
        auto& accelerationStructureFeatures    = const_cast<vk::PhysicalDeviceAccelerationStructureFeaturesKHR&>(featureChain.get<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>());
        auto& descriptorIndexingFeatures = const_cast<vk::PhysicalDeviceDescriptorIndexingFeaturesEXT&>(featureChain.get<vk::PhysicalDeviceDescriptorIndexingFeaturesEXT>());
        
        // Enable required features
        deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
        rayTracingFeatures.rayTracingPipeline = VK_TRUE;
        rayQueryFeatures.rayQuery = VK_TRUE;        
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
        accelerationStructureFeatures.accelerationStructure = VK_TRUE;
        descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
        
        if (!rayTracingFeatures.rayTracingPipeline) {
            throw std::runtime_error("Ray tracing pipeline feature is not supported on this device.");
        }

        if (!rayQueryFeatures.rayQuery) {
            std::cerr << "Ray query feature is not supported, continuing with only ray tracing pipeline." << std::endl;
        }


        // Construct device create info
        vk::DeviceCreateInfo createInfo = {};

        // Pass the feature chain via pNext
        createInfo.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>();

        // Set Device Extensions
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();

        // Set Queues
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        // Set Enabled Layers
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
            createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        // Create Device
        try {
            device_ = deviceFactory_->createDevice(*physicalDevice, createInfo);
        }
        catch (const std::exception& err) {
            std::cerr << "Vulkan error: " << err.what() << std::endl;
        }

        // Load/Retrieve vkGetInstanceProcAddr and vkGetDeviceProcAddr
        auto vkGetInstanceProcAddr = dldi->vkGetInstanceProcAddr;
        auto vkGetDeviceProcAddr = dldi->vkGetDeviceProcAddr;    
        
        // Re-Initialize Dynamic Dispatcher with Device (and Instance)
        dldi->init(instance->get(), vkGetInstanceProcAddr, getVkDevice(), vkGetDeviceProcAddr);   

        // Set Device Dispatch
        device_->setDispatch(dldi);

        // Set Device Graphics Queue
        if (device_) {
            graphicsQueue_ = device_->getQueue(indices.graphicsFamily, 0);
        } else {
            throw std::runtime_error("Logical device is invalid.");
        }

        std::cout << "Vulkan Logical Device Created" << std::endl;

    }

    void ViraDevice::createCommandPool() {

        // Get Queue Families
        QueueFamilyIndices queueFamilyIndices = findPhysicalQueueFamilies();

        // Set Command Pool Create Info
        vk::CommandPoolCreateInfo poolInfo = {};
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

        // Create Command Pool
        try {
            commandPool = device_->createCommandPool(poolInfo, nullptr);            
        }
        catch (const std::exception& err) {
            std::cerr << "Vulkan error: " << err.what() << std::endl;
        }

    }

    void ViraDevice::createSurface() { 
        vk::Instance vinstance = instance->get();
        window->createWindowSurface(vinstance, surface_); 
    }

	vk::UniqueShaderModule ViraDevice::createShaderModule(const std::vector<char>& code)
	{
		vk::ShaderModuleCreateInfo createInfo{};
		createInfo.setCodeSize(code.size());
		createInfo.setPCode(reinterpret_cast<const uint32_t*>(code.data()));

		try {
			vk::UniqueShaderModule shaderModule = device_->createShaderModule(createInfo, nullptr);
			return shaderModule;
		} catch (vk::SystemError& err) {
    		std::cerr << "Failed to create shader module: " << err.what() << std::endl;
		}

	};

    void ViraDevice::populateDebugMessengerCreateInfo(
        vk::DebugUtilsMessengerCreateInfoEXT& debugCreateInfo) {
        debugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT{};
        debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | 
                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;  // Optional
    }

    void ViraDevice::setupDebugMessenger() {

        if (!enableValidationLayers) return;

        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        populateDebugMessengerCreateInfo(debugCreateInfo);
        CreateDebugUtilsMessengerEXT(debugCreateInfo, nullptr);

        std::cout << "\nCreated Debug Messenger " << std::endl;

    }

    bool ViraDevice::checkValidationLayerSupport() {

        // Check for loaded enumerate function
        if (!dldi->vkEnumerateInstanceLayerProperties) {
            std::cerr << "vkEnumerateInstanceLayerProperties is null.\n";
            return false;
        }

        // Retrieve Layers
        std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties(*dldi);

        // Check for Available Layers
        bool allLayersAvailable = true;
        for (const char* layerName : requiredValidationLayers) {
            bool layerFound = false;
            for (const auto& layer : availableLayers) {
                if (strcmp(layer.layerName, layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                allLayersAvailable = false;
                std::cerr << "Validation layer not found: " << layerName << std::endl;
            }
        }

        if (allLayersAvailable) {
            std::cout << "\nAll required validation layers are available." << std::endl;
        } else {
            std::cerr << "Some required validation layers are missing." << std::endl;
        }
        return allLayersAvailable;
    }

    std::vector<const char*> ViraDevice::getRequiredInstanceExtensions() {

        // Get GLFW required instance extensions
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> required_instance_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Add additional platform dependent extensions needed
        #ifdef VK_USE_PLATFORM_MACOS_MVK
            required_instance_extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
            required_instance_extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME); 
        #endif

        // Set other required extensions
        required_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME); 
        required_instance_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME); 
        if (enableValidationLayers) {
            required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        // Print Required Extensions
        std::cout << "\nVira requires the following Vulkan instance extensions:" << std::endl;
        for (uint32_t i = 0; i < required_instance_extensions.size(); ++i) {
            std::cout << "\t" << required_instance_extensions[i] << std::endl;
        }
        return required_instance_extensions;
    }
    
    bool ViraDevice::checkInstanceExtensionSupport(std::vector<const char*> requiredExtensions) {
        // Enumerate supported instance extensions
        std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties(nullptr, *dldi);

        // MacOS Specific Extensions
        #ifdef VK_USE_PLATFORM_MACOS_MVK 

            // std::cout << "Available Vulkan extensions:" << std::endl;
            bool metalSurfaceSupported = false;
            for (const auto& extension : availableExtensions) {
                // std::cout << "\t" << extension.extensionName << std::endl;
                if (std::string(extension.extensionName) == "VK_EXT_metal_surface") {
                    metalSurfaceSupported = true;
                }
            }

            if (!metalSurfaceSupported) {
                std::cerr << "\n\tVK_EXT_metal_surface is not supported by your Vulkan implementation." << std::endl;
            }
        #endif

        // Check for required extensions availability
        bool allExtensionsAvailable = true;

        for (const char* requiredExtension : requiredExtensions) {
            bool extensionFound = false;

            for (const auto& extension : availableExtensions) {
                if (std::strcmp(extension.extensionName, requiredExtension) == 0) {
                    extensionFound = true;
                    break;
                }
            }

            if (!extensionFound) {
                allExtensionsAvailable = false;
                std::cerr << "Extension not found: " << requiredExtension << std::endl;
            }
        }

        if (allExtensionsAvailable) {
            std::cout << "All required instance extensions are available." << std::endl;
            return true;
        } else {
            std::cerr << "Some required instance extensions are missing." << std::endl;
            return false;
        }

    }

    bool ViraDevice::checkDeviceExtensionSupport(PhysicalDeviceInterface& pdevice) {

        // Get required device extensions
        std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

        // Enumerate supported device extensions for this physical device
        std::vector<vk::ExtensionProperties> availableExtensions = pdevice.enumerateDeviceExtensionProperties();

        // Check if all required extensions are available
        bool allExtensionsAvailable = true;
        for (const char* requiredExtension : requiredDeviceExtensions) {
            bool extensionFound = false;

            for (const auto& extension : availableExtensions) {
                if (std::strcmp(extension.extensionName, requiredExtension) == 0) {
                    extensionFound = true;
                    break;
                }
            }

            if (!extensionFound) {
                allExtensionsAvailable = false;
                std::cerr << "\tExtension not found for device: " << requiredExtension << std::endl;
            }
        }

        if (allExtensionsAvailable) {
            std::cout << "\tAll required device extensions are available for this device." << std::endl;
            return true;
        } else {
            std::cerr << "\tSome required device extensions are missing for this device." << std::endl;
            return false;
        }

    }
    
    QueueFamilyIndices ViraDevice::findQueueFamilies(PhysicalDeviceInterface& pdevice) {
        
        QueueFamilyIndices indices;
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = pdevice.getQueueFamilyProperties();
    
        int i = 0;
        for (const auto& queueFamily : queueFamilyProperties) {
            if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
                indices.graphicsFamilyHasValue = true;
            }
            
            if(surface_) {
                if (queueFamily.queueCount > 0 && pdevice.getSurfaceSupportKHR(i, surface_)) {
                    indices.presentFamily = i;
                    indices.presentFamilyHasValue = true;
                }
            }
            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }
    
    SwapchainSupportDetails ViraDevice::querySwapchainSupport(PhysicalDeviceInterface& pdevice) {
        SwapchainSupportDetails details;
        details.capabilities = pdevice.getSurfaceCapabilitiesKHR(surface_);
        details.formats      = pdevice.getSurfaceFormatsKHR(surface_);
        details.presentModes = pdevice.getSurfacePresentModesKHR(surface_);

        return details;
    }
    
    vk::Format  ViraDevice::findSupportedFormat(
        const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
        for (vk::Format format : candidates) {
            vk::FormatProperties props = physicalDevice->getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (
                tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported format!");
    }

    uint32_t ViraDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {

        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice->getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }
    
    std::unique_ptr<MemoryInterface>  ViraDevice::createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::UniqueBuffer& buffer) {

        // Create Buffer Memory
        std::unique_ptr<MemoryInterface> bufferMemory;            

        // Create VK Buffer
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.setSize(size);
        bufferInfo.setUsage(usage);
        bufferInfo.setSharingMode(vk::SharingMode::eExclusive);
        try {
           buffer = device_->createBufferUnique(bufferInfo, nullptr);
        }
        catch (const std::exception& ex) {
            std::cout << "\n" << ex.what() << "\n" << "Buffer Creation Failed";
        }
        
        // Retrieve memory requirements
        vk::MemoryRequirements memRequirements = device_->getBufferMemoryRequirements(buffer.get());

        // Add Memory Allocate Flags Info
        vk::MemoryAllocateFlagsInfo allocateFlagsInfo{};
        allocateFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;

        // Allocate Memory
        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.setAllocationSize(memRequirements.size);
        allocInfo.setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, properties));
        allocInfo.setPNext(&allocateFlagsInfo); // Attach the allocate flags info to allocInfo
        try {
            bufferMemory = device_->allocateMemory(allocInfo, nullptr); 
        }
        catch (const std::exception& ex) {
            std::cout << "\n" << ex.what() << "\n" << "Buffer Allocation Failed";
        }

        // Bind Memory
        try {      
            device_->bindBufferMemory(buffer.get(), bufferMemory, 0);
        }
        catch (const std::exception& ex) {
            std::cout << "\n" << ex.what() << "\n" << "Buffer Binding Failed";
        }

        // Return Buffer Memory
        return bufferMemory;
    }

    void ViraDevice::transitionImageLayout(
        vk::Image image,                  // Image to transition
        vk::ImageLayout oldLayout,        // Current layout of the image
        vk::ImageLayout newLayout,        // Target layout
        vk::PipelineStageFlags srcStageMask, // Source pipeline stage
        vk::PipelineStageFlags dstStageMask,  // Destination pipeline stage
        size_t layerCount,
        vk::ImageAspectFlags aspectMask
    ) 
    {

        // Begin command recording
        CommandBufferInterface* commandBuffer = beginSingleTimeCommands();

        // Create Image Memory Barrier
        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        // Specify the aspect of the image to transition (e.g., color)
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = static_cast<uint32_t>(layerCount);

        // Define access masks based on layouts
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = {}; // No reads/writes in `eUndefined`
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite; // Ready for transfer writes
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite; // Finish transfer writes
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;    // Ready for shader reads
        } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // Ensure rendering is complete
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;         // Ready for transfer read
        } else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;         // Finish transfer reads
            barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // Ready for rendering writes
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral) {
            barrier.srcAccessMask = {}; // No reads/writes in `eUndefined`
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite; // Ready for transfer writes
        } else if (oldLayout == vk::ImageLayout::eGeneral && newLayout == vk::ImageLayout::eTransferSrcOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite; // No reads/writes in `eUndefined`
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead; // Ready for transfer writes
        } else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eGeneral) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead; // No reads/writes in `eUndefined`
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite; // Ready for transfer writes
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            // Initial transition for depth images (no previous access)
            barrier.srcAccessMask = {}; 
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        
        } else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            // Transitioning depth image to be read-only in shaders
            barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        
        } else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            // Restoring depth image for rendering
            barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        
        } else if (oldLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal && newLayout == vk::ImageLayout::eTransferSrcOptimal) {
            // Depth image being copied or saved
            barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        
        } else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            // Restoring depth image for rendering after a copy
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
            barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
            barrier.srcAccessMask = {}; // No access from previous layout
            barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        } else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        // Record the pipeline barrier command
        commandBuffer->pipelineBarrier(
            srcStageMask, dstStageMask,      // Source and destination pipeline stages
            {},                              // Dependency flags (usually empty)
            nullptr, nullptr,                // Memory and buffer barriers (not used here)
            barrier                          // Image memory barrier
        );

        // End command recording and submit command buffer
        endSingleTimeCommands(commandBuffer, nullptr);

    }

    // Physical Device Properties
    void ViraDevice::queryPhysicalDeviceProperties2() {

        // Initialize property structures
        vk::PhysicalDeviceProperties2 properties2{};

        // Chain additional properties to retrieve acceleration structure and ray tracing properties
        properties2.pNext = &accelStructProperties;
        accelStructProperties.pNext = &rayTracingProperties;

        // Retrieve all properties in one call
        getPhysicalDevice()->getProperties2(&properties2);
        
        // Access general device properties and limits
        physicalDeviceProperties = properties2.properties;

    }    

    std::vector<vira::vec4<float>> ViraDevice::extractSpectralSetData(vk::Image image, Resolution resolution) {
        size_t nPixels = resolution.x * resolution.y;
        size_t imageSize = nPixels * sizeof(vira::vec4<float>);
    
        // Ensure imageSize is aligned to avoid misalignment issues
        vk::DeviceSize minStorageBufferOffsetAlignment = physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
        imageSize = (imageSize + minStorageBufferOffsetAlignment - 1) & ~(minStorageBufferOffsetAlignment - 1);
    
        // Initialize vector to hold Vec4 Data
        std::vector<vira::vec4<float>> dataVec(nPixels);
        
        // Transition image layout to be read
        transitionImageLayout(
            image,
            vk::ImageLayout::eColorAttachmentOptimal,   // Old layout
            vk::ImageLayout::eTransferSrcOptimal,      // New layout
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eTransfer
        );
    
        // Create a staging buffer for the image
        std::unique_ptr<ViraBuffer> imageStagingBuffer = std::make_unique<ViraBuffer>(
            this,
            imageSize,
            1, 
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            minStorageBufferOffsetAlignment
        );
    
        // Copy the vk::Image to the staging buffer
        copyImageToBuffer(image, imageStagingBuffer->getBuffer(), static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), 1);
    
        // Map the staging buffer memory
        imageStagingBuffer->map(imageSize, 0);
    
        // Read the raw data and copy it efficiently
        float* rawData = static_cast<float*>(imageStagingBuffer->getMappedMemory());
        std::memcpy(dataVec.data(), rawData, imageSize);
    
        imageStagingBuffer->unmap();

        // Transition image layout to be read
        transitionImageLayout(
            image,
            vk::ImageLayout::eTransferSrcOptimal,      
            vk::ImageLayout::eColorAttachmentOptimal, 
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        );

        // Return vector of Vec4 data
        return dataVec;
    }
    
    std::vector<float> ViraDevice::extractScalarFloatImageData(vk::Image image, Resolution resolution, bool isDepth) {
        size_t nPixels = resolution.x * resolution.y;
        size_t imageSize = nPixels * sizeof(float);
    
        // Ensure imageSize is aligned
        vk::DeviceSize minStorageBufferOffsetAlignment = physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
        imageSize = (imageSize + minStorageBufferOffsetAlignment - 1) & ~(minStorageBufferOffsetAlignment - 1);
    
        // Initialize vector of floats for scalar data
        std::vector<float> dataVec(nPixels);
        
        // Set layout / flags
        vk::ImageAspectFlags aspectMask;
        vk::ImageLayout imageLayout;
        vk::PipelineStageFlags stageFlags;
        if(isDepth) {
            aspectMask = vk::ImageAspectFlagBits::eDepth;
            imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

            stageFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests; 
            
        } else {
            aspectMask = vk::ImageAspectFlagBits::eColor;
            imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            stageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput; 
        }

        // Transition image layout to be read
        transitionImageLayout(
            image,
            imageLayout,
            vk::ImageLayout::eTransferSrcOptimal,
            stageFlags,
            vk::PipelineStageFlagBits::eTransfer,
            1,
            aspectMask
        );
    
        // Create a staging buffer for the image
        std::unique_ptr<ViraBuffer> imageStagingBuffer = std::make_unique<ViraBuffer>(
            this,
            imageSize,
            1, 
            vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            minStorageBufferOffsetAlignment
        );
    
        // Copy the vk::Image to the staging buffer
        copyImageToBuffer(image, imageStagingBuffer->getBuffer(), static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), 1, 0, aspectMask);
    
        // Map the staging buffer memory
        imageStagingBuffer->map(imageSize, 0);
    
        // Read the raw data and copy it efficiently
        std::memcpy(dataVec.data(), imageStagingBuffer->getMappedMemory(), imageSize);
    
        imageStagingBuffer->unmap();

        // Transition image layout back
        transitionImageLayout(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            imageLayout,
            vk::PipelineStageFlagBits::eTransfer,
            stageFlags,
            1,
            aspectMask
        );

        // Return vector of float data
        return dataVec;
    }
    
    
    CommandBufferInterface* ViraDevice::beginSingleTimeCommands() {
        
        // Allocate Command Buffer
        vk::CommandBufferAllocateInfo allocInfo{commandPool->getVkCommandPool(), vk::CommandBufferLevel::ePrimary, 1};
        std::vector<CommandBufferInterface*> commandBuffers = device_->allocateCommandBuffers(allocInfo);
        CommandBufferInterface* commandBuffer = commandBuffers[0];

        // Set Command Buffer to Recording State
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.setFlags(vk::CommandBufferUsageFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        commandBuffer->begin(beginInfo);

        // Return pointer to command buffer
        return commandBuffer;
    }

    void ViraDevice::endSingleTimeCommands(CommandBufferInterface* commandBuffer, QueueInterface* queue) {

        // If queue not specified, set to graphic queue
        if (queue == nullptr) {
            queue = this->graphicsQueue_;
        }        

        // Set command buffer in non-recording state
        commandBuffer->end();

        // Create command submit info
        std::vector<vk::CommandBuffer> commandBuffers = {commandBuffer->get()};
        vk::SubmitInfo submitInfo = {};
        submitInfo.setCommandBuffers(commandBuffers);

        // Create fence for commands
        vk::FenceCreateInfo fenceInfo = {};
        fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
        vk::Fence fence = device_->createFence(fenceInfo, nullptr);

        // Submit command buffer for execution
        queue->submit(submitInfo, fence);

        // Wait for fences before exiting method
        try {
            device_->waitForFences({fence}, VK_TRUE, UINT64_MAX);

        } catch (const vk::SystemError& e) {
            std::cout << e.what() << std::endl;

        }

        // Destroy the fence
        device_->destroyFence(fence, nullptr);
    }
    
    void ViraDevice::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {

        // Begin recording commands
        CommandBufferInterface* commandBuffer = beginSingleTimeCommands();

        // Copy Buffer
        vk::BufferCopy copyRegion = {};
        copyRegion.setSrcOffset(0);  
        copyRegion.setDstOffset(0); 
        copyRegion.setSize(size);
        commandBuffer->copyBuffer(srcBuffer, dstBuffer, copyRegion);

        // End recording commands and submit for execution
        endSingleTimeCommands(std::move(commandBuffer));
    }
    
    void ViraDevice::copyImageToBuffer(vk::Image image, vk::Buffer buffer, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t base_layer, vk::ImageAspectFlags aspectMask) {

        // Begin recording commands
        CommandBufferInterface* commandBuffer = beginSingleTimeCommands();

        // Copy vk::Image to the buffer
        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0; // Tightly packed
        region.bufferImageHeight = 0; // Tightly packed
        region.imageSubresource.aspectMask = aspectMask;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = base_layer;
        region.imageSubresource.layerCount = layerCount;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = vk::Extent3D{width, height, 1};
        commandBuffer->copyImageToBuffer(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            buffer,
            {region}
        );

        // End recording commands and submit for execution
        endSingleTimeCommands(std::move(commandBuffer));

    }

    void ViraDevice::copyBufferToImage(
        vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t base_layer) {

        (void)base_layer; // TODO Consider removing

        // Begin recording commands
        CommandBufferInterface* commandBuffer = beginSingleTimeCommands();

        // Set Copy vk::Buffer to vk::Image
        vk::BufferImageCopy region = {};
        region.setBufferOffset(0);
        region.setBufferRowLength(0);
        region.setBufferImageHeight(0);

        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;  // Set the aspect mask to color
        region.imageSubresource.mipLevel = 0;                                  // Mip level to copy from
        region.imageSubresource.baseArrayLayer = 0;                            // Starting layer
        region.imageSubresource.layerCount = layerCount;  

        region.setImageOffset({ 0, 0, 0 });
        region.setImageExtent({ width, height, 1 });

        commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

        // End recording commands and submit for execution
        endSingleTimeCommands(std::move(commandBuffer));
    }
    
    void ViraDevice::createImageWithInfo(
        const vk::ImageCreateInfo& imageInfo,
        vk::MemoryPropertyFlags properties,
        vk::Image& image,
        std::unique_ptr<MemoryInterface>& imageMemory) {
        
        // Create vk::Image
        try {
            image = device_->createImage(imageInfo, nullptr);
        }
        catch (const std::exception& ex) 
        {
            throw ex;
        }

        // Retrieve memory requirements
        vk::MemoryRequirements memRequirements = device_->getImageMemoryRequirements(image);

        // Allocate Memory
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.setAllocationSize(memRequirements.size);
        allocInfo.setMemoryTypeIndex(findMemoryType(memRequirements.memoryTypeBits, properties));
        try {
            imageMemory = device_->allocateMemory(allocInfo, nullptr);
        }
        catch (const std::exception& ex) {
            throw ex;
        }

        // Bind Memory
        try {
            device_->bindImageMemory(image, imageMemory, 0);
        }
        catch (const std::exception& ex) {
            throw ex;
        }
    }

    void ViraDevice::CreateDebugUtilsMessengerEXT(

        const vk::DebugUtilsMessengerCreateInfoEXT& createInfo,
        const vk::Optional<const vk::AllocationCallbacks> allocator) {

        debugMessenger = instance->get().createDebugUtilsMessengerEXTUnique(
            createInfo,
            allocator,
            *dldi     
        );

    }

}