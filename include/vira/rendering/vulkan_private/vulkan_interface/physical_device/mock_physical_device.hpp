#ifndef MOCK_VULKAN_PHYSICAL_DEVICE_HPP
#define MOCK_VULKAN_PHYSICAL_DEVICE_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"

namespace vira::vulkan {

class VulkanDevice;

class MockPhysicalDevice : public PhysicalDeviceInterface {
public:
    MOCK_METHOD(std::unique_ptr<DeviceInterface>, createDeviceUnique, (const vk::DeviceCreateInfo& createInfo), (const, override));
    MOCK_METHOD(std::vector<vk::ExtensionProperties>, enumerateDeviceExtensionProperties, (const char* layerName), (const, override));
    MOCK_METHOD(vk::PhysicalDevice&, getDevice, (), (const, override));
    MOCK_METHOD(vk::PhysicalDeviceProperties, getProperties, (), (const, override));
    MOCK_METHOD(void, getProperties2, (vk::PhysicalDeviceProperties2* props), (const, override));
    MOCK_METHOD(void, getFeatures2, (vk::PhysicalDeviceFeatures2* features), (const, override));
    MOCK_METHOD(vk::PhysicalDeviceFeatures, getFeatures, (), (const, override));
    MOCK_METHOD(std::vector<vk::QueueFamilyProperties>, getQueueFamilyProperties, (), (const, override));
    MOCK_METHOD(vk::Bool32, getSurfaceSupportKHR, (uint32_t queueFamilyIndex, vk::SurfaceKHR surface), (const, override));
    MOCK_METHOD(vk::SurfaceCapabilitiesKHR, getSurfaceCapabilitiesKHR, (vk::SurfaceKHR surface), (const, override));
    MOCK_METHOD(std::vector<vk::SurfaceFormatKHR>, getSurfaceFormatsKHR, (vk::SurfaceKHR surface), (const, override));
    MOCK_METHOD(std::vector<vk::PresentModeKHR>, getSurfacePresentModesKHR, (vk::SurfaceKHR surface), (const, override));
    MOCK_METHOD(vk::FormatProperties, getFormatProperties, (vk::Format format), (const, override));
    MOCK_METHOD(vk::PhysicalDeviceMemoryProperties, getMemoryProperties, (), (const, override));
    MOCK_METHOD(void, setDispatch, (vk::DispatchLoaderDynamic* dldi), (override));
    MOCK_METHOD(vk::Result, getImageFormatProperties, (
        vk::Format           format,
        vk::ImageType        type,
        vk::ImageTiling      tiling,
        vk::ImageUsageFlags  usage,
        vk::ImageCreateFlags flags,
        vk::ImageFormatProperties* imageProps), (const, override));
};

} // namespace vira::vulkan

#endif // MOCK_VULKAN_PHYSICAL_DEVICE_HPP
