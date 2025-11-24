#ifndef VULKAN_PHYSICAL_DEVICE_HPP
#define VULKAN_PHYSICAL_DEVICE_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"

namespace vira::vulkan {

class VulkanPhysicalDevice : public PhysicalDeviceInterface {
public:

    VulkanPhysicalDevice(vk::PhysicalDevice physicalDevice);
    VulkanPhysicalDevice(vk::PhysicalDevice physicalDevice, vk::DispatchLoaderDynamic* dldi);
    std::unique_ptr<DeviceInterface> createDeviceUnique(const vk::DeviceCreateInfo& createInfo) const override;
    std::vector<vk::ExtensionProperties> enumerateDeviceExtensionProperties(const char* layerName = nullptr) const override;
    const vk::PhysicalDevice& getDevice() const override;
    vk::PhysicalDeviceProperties getProperties() const override;
    void getProperties2(vk::PhysicalDeviceProperties2* props) const override;
    vk::PhysicalDeviceFeatures getFeatures() const override;
    std::vector<vk::QueueFamilyProperties> getQueueFamilyProperties() const override;
    vk::Bool32 getSurfaceSupportKHR(uint32_t queueFamilyIndex, vk::SurfaceKHR surface) const override;
    vk::SurfaceCapabilitiesKHR getSurfaceCapabilitiesKHR(vk::SurfaceKHR surface) const override;
    std::vector<vk::SurfaceFormatKHR> getSurfaceFormatsKHR(vk::SurfaceKHR surface) const override;
    std::vector<vk::PresentModeKHR> getSurfacePresentModesKHR(vk::SurfaceKHR surface) const override;
    vk::FormatProperties getFormatProperties(vk::Format format) const override;
    vk::PhysicalDeviceMemoryProperties getMemoryProperties() const override;
    void setDispatch(vk::DispatchLoaderDynamic* newDldi) override {this->dldi = newDldi;};
    void getFeatures2(vk::PhysicalDeviceFeatures2* props) const override;
    vk::Result getImageFormatProperties( vk::Format format,
        vk::ImageType        type,
        vk::ImageTiling      tiling,
        vk::ImageUsageFlags  usage,
        vk::ImageCreateFlags flags,
        vk::ImageFormatProperties* imageProps ) const override;

private:
    vk::PhysicalDevice physicalDevice_;
    vk::DispatchLoaderDynamic* dldi;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/physical_device/vulkan_physical_device.ipp"

#endif // VULKAN_PHYSICAL_DEVICE_HPP
