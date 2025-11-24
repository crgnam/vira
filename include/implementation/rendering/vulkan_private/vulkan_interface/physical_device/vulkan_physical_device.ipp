#include "vira/rendering/vulkan_private/vulkan_interface/device/vulkan_device.hpp"

namespace vira::vulkan {

VulkanPhysicalDevice::VulkanPhysicalDevice(vk::PhysicalDevice physicalDevice)
    : physicalDevice_(physicalDevice) {}

VulkanPhysicalDevice::VulkanPhysicalDevice(vk::PhysicalDevice physicalDevice, vk::DispatchLoaderDynamic* dldi)
    : physicalDevice_(physicalDevice) {
        setDispatch(dldi);
    }

std::unique_ptr<DeviceInterface> VulkanPhysicalDevice::createDeviceUnique(const vk::DeviceCreateInfo& createInfo) const {
    vk::UniqueDevice device = physicalDevice_.createDeviceUnique(createInfo,nullptr, *dldi);
    return std::make_unique<VulkanDevice>(std::move(device));
}
std::vector<vk::ExtensionProperties> VulkanPhysicalDevice::enumerateDeviceExtensionProperties(const char* layerName) const {
    if (layerName) {
        return physicalDevice_.enumerateDeviceExtensionProperties(std::string(layerName), *dldi);
    } else {
        return physicalDevice_.enumerateDeviceExtensionProperties(nullptr, *dldi);
    }
}

const vk::PhysicalDevice& VulkanPhysicalDevice::getDevice() const {
    return physicalDevice_;
}
vk::PhysicalDeviceProperties VulkanPhysicalDevice::getProperties() const {
    return physicalDevice_.getProperties(*dldi);
}

void VulkanPhysicalDevice::getProperties2(vk::PhysicalDeviceProperties2* props) const {
    return physicalDevice_.getProperties2(props, *dldi);
}
vk::PhysicalDeviceFeatures VulkanPhysicalDevice::getFeatures() const {
    return physicalDevice_.getFeatures(*dldi);
}
void VulkanPhysicalDevice::getFeatures2(vk::PhysicalDeviceFeatures2* props) const {
    return physicalDevice_.getFeatures2(props, *dldi);
}

std::vector<vk::QueueFamilyProperties> VulkanPhysicalDevice::getQueueFamilyProperties() const {
    return physicalDevice_.getQueueFamilyProperties(*dldi);
}

vk::Bool32 VulkanPhysicalDevice::getSurfaceSupportKHR(uint32_t queueFamilyIndex, vk::SurfaceKHR surface) const {
    return physicalDevice_.getSurfaceSupportKHR(queueFamilyIndex, surface, *dldi);
}

vk::SurfaceCapabilitiesKHR VulkanPhysicalDevice::getSurfaceCapabilitiesKHR(vk::SurfaceKHR surface) const {
    return physicalDevice_.getSurfaceCapabilitiesKHR(surface, *dldi);
}

std::vector<vk::SurfaceFormatKHR> VulkanPhysicalDevice::getSurfaceFormatsKHR(vk::SurfaceKHR surface) const {
    return physicalDevice_.getSurfaceFormatsKHR(surface, *dldi);
}

std::vector<vk::PresentModeKHR> VulkanPhysicalDevice::getSurfacePresentModesKHR(vk::SurfaceKHR surface) const {
    return physicalDevice_.getSurfacePresentModesKHR(surface, *dldi);
}

vk::FormatProperties VulkanPhysicalDevice::getFormatProperties(vk::Format format) const {
    return physicalDevice_.getFormatProperties(format, *dldi);
}

vk::PhysicalDeviceMemoryProperties VulkanPhysicalDevice::getMemoryProperties() const {
    return physicalDevice_.getMemoryProperties(*dldi);
}

vk::Result VulkanPhysicalDevice::getImageFormatProperties(vk::Format format,
    vk::ImageType        type,
    vk::ImageTiling      tiling,
    vk::ImageUsageFlags  usage,
    vk::ImageCreateFlags flags,
    vk::ImageFormatProperties* imageProps) const {
        return physicalDevice_.getImageFormatProperties(format, type, tiling, usage, flags, imageProps, *dldi);
}

} 