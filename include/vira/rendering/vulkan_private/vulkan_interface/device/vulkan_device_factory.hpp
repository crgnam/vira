#ifndef VULKAN_DEVICE_FACTORY_HPP
#define VULKAN_DEVICE_FACTORY_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/vulkan_device.hpp"
// #include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/vulkan_physical_device.hpp"

namespace vira::vulkan {

class VulkanDeviceFactory : public DeviceFactoryInterface {
public:
    std::unique_ptr<DeviceInterface> createDevice(PhysicalDeviceInterface& physicalDevice, const vk::DeviceCreateInfo& createInfo) const override {

        return physicalDevice.createDeviceUnique(createInfo);
    }
};

} // namespace vira::vulkan

#endif // VULKAN_DEVICE_FACTORY_HPP
