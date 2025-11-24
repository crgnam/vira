#ifndef PHYSICAL_DEVICE_INTERFACE_HPP
#define PHYSICAL_DEVICE_INTERFACE_HPP

#include <vector>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"

namespace vira::vulkan {

class DeviceInterface;

class PhysicalDeviceInterface {
public:
    virtual std::unique_ptr<DeviceInterface> createDeviceUnique(const vk::DeviceCreateInfo& createInfo) const = 0;
    virtual std::vector<vk::ExtensionProperties> enumerateDeviceExtensionProperties(const char* layerName = nullptr) const = 0;
    virtual const vk::PhysicalDevice& getDevice() const = 0;
    virtual vk::PhysicalDeviceProperties getProperties() const = 0;
    virtual void getProperties2(vk::PhysicalDeviceProperties2* props) const = 0;
    virtual void getFeatures2(vk::PhysicalDeviceFeatures2* props) const = 0;
    virtual vk::PhysicalDeviceFeatures getFeatures() const = 0;
    virtual std::vector<vk::QueueFamilyProperties> getQueueFamilyProperties() const = 0;
    virtual vk::Bool32 getSurfaceSupportKHR(uint32_t queueFamilyIndex, vk::SurfaceKHR surface) const = 0;
    virtual vk::SurfaceCapabilitiesKHR getSurfaceCapabilitiesKHR(vk::SurfaceKHR surface) const = 0;
    virtual std::vector<vk::SurfaceFormatKHR> getSurfaceFormatsKHR(vk::SurfaceKHR surface) const = 0;
    virtual std::vector<vk::PresentModeKHR> getSurfacePresentModesKHR(vk::SurfaceKHR surface) const = 0;
    virtual vk::FormatProperties getFormatProperties(vk::Format format) const = 0;
    virtual vk::PhysicalDeviceMemoryProperties getMemoryProperties() const = 0;
    virtual void setDispatch(vk::DispatchLoaderDynamic* dldi) = 0;
    virtual vk::Result getImageFormatProperties(vk::Format format,
        vk::ImageType        type,
        vk::ImageTiling      tiling,
        vk::ImageUsageFlags  usage,
        vk::ImageCreateFlags flags,
        vk::ImageFormatProperties* imageProps ) const = 0;
};

} // namespace vira::vulkan

#endif // PHYSICAL_DEVICE_INTERFACE_HPP
