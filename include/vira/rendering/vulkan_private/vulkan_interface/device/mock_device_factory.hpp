#ifndef MOCK_DEVICE_FACTORY_HPP
#define MOCK_DEVICE_FACTORY_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/mock_device.hpp"

namespace vira::vulkan {

class MockDeviceFactory : public DeviceFactoryInterface {
public:
    virtual std::unique_ptr<DeviceInterface> createDevice(PhysicalDeviceInterface& physicalDevice, const vk::DeviceCreateInfo& createInfo) const override {
        (void)createInfo; // TODO Consider removing
        (void)physicalDevice; // TODO Consider removing

        return std::make_unique<MockDevice>();
    }
    
};

} // namespace vira::vulkan

#endif // MOCK_DEVICE_FACTORY_HPP
