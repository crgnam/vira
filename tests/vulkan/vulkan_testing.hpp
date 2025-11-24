#include "gtest/gtest.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <cstdlib> 
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib> 
#include <stdexcept>
#include <vector>
#include <chrono>
#include <vulkan/vulkan.hpp>

#include "vira/vira-vulkan.hpp"
#include "vira/vira.hpp"

using namespace vira::vulkan;

const uint32_t WIDTH = 1944;
const uint32_t HEIGHT = 2592;
using TFloat = float;
using TSpectral = vira::UniformVisibleData<4>;
using TMeshFloat = float;
const float EXPTIME = 0.00001f;

// Testable Vira Classes

class TestableViraWindow : public ViraWindow {
    public:
        TestableViraWindow(int w, int h) : ViraWindow(w, h) {};
};

class TestableDescriptorSetLayout : public ViraDescriptorSetLayout {

    public:
        TestableDescriptorSetLayout(ViraDevice* viraDevice, std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings) : ViraDescriptorSetLayout(viraDevice, bindings) {};

        using ViraDescriptorSetLayout::bindings;
        using ViraDescriptorSetLayout::descriptorSetLayout;
    
};

class TestableViraDevice : public ViraDevice {
    public:
        TestableViraDevice() : ViraDevice() {}
        TestableViraDevice(ViraWindow* window) : ViraDevice(window) {}
        TestableViraDevice(
            std::shared_ptr<InstanceFactoryInterface> instanceFactory,
            std::shared_ptr<DeviceFactoryInterface> deviceFactory,
            ViraWindow* window) : ViraDevice(window) {
                instanceFactory_ = instanceFactory;
                deviceFactory_ = deviceFactory;
            }
        TestableViraDevice(
            std::shared_ptr<InstanceFactoryInterface> instanceFactory,
            std::shared_ptr<DeviceFactoryInterface> deviceFactory) : ViraDevice(instanceFactory, deviceFactory) {
            }

        void setDevice(std::unique_ptr<DeviceInterface> dev) {
            device_ = std::move(dev);
        }    

        using ViraDevice::graphicsQueue_;
        using ViraDevice::presentQueue_;

        using ViraDevice::commandPool;
        using ViraDevice::surface_;
        using ViraDevice::window;
        using ViraDevice::device_;
        using ViraDevice::instance;
        using ViraDevice::physicalDevice;
        using ViraDevice::debugMessenger;

        using ViraDevice::rateDeviceSuitability;
        using ViraDevice::getRequiredInstanceExtensions;
        using ViraDevice::checkInstanceExtensionSupport;
        using ViraDevice::requiredDeviceExtensions;
        using ViraDevice::checkDeviceExtensionSupport;
        using ViraDevice::findQueueFamilies;
        using ViraDevice::querySwapchainSupport;
        using ViraDevice::populateDebugMessengerCreateInfo;
        using ViraDevice::checkValidationLayerSupport;
        using ViraDevice::queryPhysicalDeviceProperties2;
};

class TestableViraBuffer : public ViraBuffer {
    public:
        TestableViraBuffer() : ViraBuffer() {};
        TestableViraBuffer(ViraDevice* viraDevice) : ViraBuffer(viraDevice) {
        };
        TestableViraBuffer(
            ViraDevice* viraDevice,
            vk::DeviceSize instanceSize,
            uint32_t instanceCount,
            vk::BufferUsageFlags usageFlags,
            vk::MemoryPropertyFlags memoryPropertyFlags,
            vk::DeviceSize minOffsetAlignment) : ViraBuffer(viraDevice, instanceSize, instanceCount, usageFlags, memoryPropertyFlags, minOffsetAlignment) {};

        using ViraBuffer::flushIndex;
        using ViraBuffer::writeToIndex;
        using ViraBuffer::getBuffer;
        using ViraBuffer::alignmentSize;
        using ViraBuffer::instanceSize;
        using ViraBuffer::memory;
        using ViraBuffer::buffer;
        using ViraBuffer::mapped;
        using ViraBuffer::viraDevice;

};

class TestableViraSwapchain : public ViraSwapchain {
    public:
        TestableViraSwapchain(vk::Extent2D extent) : ViraSwapchain() {
            windowExtent = extent;
        };
        TestableViraSwapchain(ViraDevice* viraDevice) : ViraSwapchain(viraDevice) {}

        TestableViraSwapchain(ViraDevice* viraDevice, vk::Extent2D extent) : ViraSwapchain(viraDevice) {
            windowExtent = extent;
        };

        using ViraSwapchain::inFlightFences;
        using ViraSwapchain::imagesInFlight;
        using ViraSwapchain::imageAvailableSemaphores;
        using ViraSwapchain::renderFinishedSemaphores;
        using ViraSwapchain::currentFrame;
        using ViraSwapchain::createSyncObjects;
        using ViraSwapchain::createDepthResources;
        using ViraSwapchain::createPathTracerImageResources;
        using ViraSwapchain::windowExtent;
        using ViraSwapchain::swapchainExtent;
        using ViraSwapchain::viraDevice;
        using ViraSwapchain::createFramebuffers;
        using ViraSwapchain::createRenderPass;
        using ViraSwapchain::createImageViews;
        using ViraSwapchain::createSwapchain;
        using ViraSwapchain::createAttachment;
        using ViraSwapchain::swapchainImageFormat;
        using ViraSwapchain::swapchainDepthFormat;
        using ViraSwapchain::swapchain;
        using ViraSwapchain::createFrameAttachments;
        using ViraSwapchain::renderPass;
        using ViraSwapchain::createFrameBuffer;
        
};

class TestableViraPipeline : public ViraPipeline<TSpectral, TFloat> {
    public:
        TestableViraPipeline(ViraDevice* device) : ViraPipeline<TFloat,TSpectral>(device) {};
        TestableViraPipeline(ViraDevice* device,
                                 const std::string& vertFilepath,
                                 const std::string& fragFilepath,
                                 const PipelineConfigInfo* configInfo) : ViraPipeline<TSpectral, TFloat>(device, vertFilepath, fragFilepath, configInfo) {};
        using ViraPipeline<TSpectral, TFloat>::viraDevice;
        using ViraPipeline<TSpectral, TFloat>::pipeline;
        using ViraPipeline<TSpectral, TFloat>::vertShaderModule;
        using ViraPipeline<TSpectral, TFloat>::fragShaderModule;
        using ViraPipeline<TSpectral, TFloat>::createRasterizationPipeline;
};

std::vector<vk::QueueFamilyProperties> createQueueFamilyProperties() {
    std::vector<vk::QueueFamilyProperties> queueFamilyPropertiesList;

    // Graphics family
    vk::QueueFamilyProperties graphicsFamily;
    graphicsFamily.queueFlags = vk::QueueFlagBits::eGraphics; // Graphics queue
    graphicsFamily.queueCount = 1; // Assuming one queue for simplicity
    graphicsFamily.timestampValidBits = 64; 
    graphicsFamily.minImageTransferGranularity = vk::Extent3D{1, 1, 1}; 
    queueFamilyPropertiesList.push_back(graphicsFamily);

    // Present family (for simplicity, assume it's separate, though it can often be combined with graphics)
    vk::QueueFamilyProperties presentFamily;
    presentFamily.queueFlags = {};          // No specific queue flags for presentation (handled outside)
    presentFamily.queueCount = 1;           // Assuming one queue for presentation
    presentFamily.timestampValidBits = 64;  
    presentFamily.minImageTransferGranularity = vk::Extent3D{1, 1, 1}; 
    queueFamilyPropertiesList.push_back(presentFamily);

    // Another valid family (e.g., compute)
    vk::QueueFamilyProperties computeFamily;
    computeFamily.queueFlags = vk::QueueFlagBits::eCompute; // Compute queue
    computeFamily.queueCount = 1; // Assuming one queue
    computeFamily.timestampValidBits = 64; 
    computeFamily.minImageTransferGranularity = vk::Extent3D{1, 1, 1}; 
    queueFamilyPropertiesList.push_back(computeFamily);

    return queueFamilyPropertiesList;
}

std::vector<vk::ExtensionProperties> createExtensionProperties(const std::vector<const char*>& extensionNames) {
    std::vector<vk::ExtensionProperties> extensionPropertiesList;

    for (const auto& extensionName : extensionNames) {
        vk::ExtensionProperties extensionProperties;

        // Fill the extensionName in vk::ExtensionProperties 
        std::strncpy(extensionProperties.extensionName, extensionName, VK_MAX_EXTENSION_NAME_SIZE - 1);

        // Fill in dummy data for other fields
        extensionProperties.specVersion = 1; 

        // Add to the list
        extensionPropertiesList.push_back(extensionProperties);
    }

    return extensionPropertiesList;
}

// Helper function to check if an item exists in the vector
bool containsItem(const std::vector<const char*>& vec, const char* item) {
    return std::find_if(vec.begin(), vec.end(),
                        [item](const char* str) { return std::strcmp(str, item) == 0; }) != vec.end();
}


