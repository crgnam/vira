#ifndef DEVICE_FACTORY_INTERFACE_HPP
#define DEVICE_FACTORY_INTERFACE_HPP

#include <memory>

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"

namespace vira::vulkan {
class DeviceInterface;
class PhysicalDeviceInterface;
class DeviceFactoryInterface {
public:
    virtual ~DeviceFactoryInterface() = default;
    virtual std::unique_ptr<DeviceInterface> createDevice(PhysicalDeviceInterface& physicalDevice, const vk::DeviceCreateInfo& createInfo) const = 0;
};

} // namespace vira::vulkan

#endif // DEVICE_FACTORY_INTERFACE_HPP
