#ifndef INSTANCE_INTERFACE_HPP
#define INSTANCE_INTERFACE_HPP

#include <vector>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"

namespace vira::vulkan {
class PhysicalDeviceInterface;
class InstanceInterface {
public:

    virtual ~InstanceInterface() = default;
    virtual std::vector<PhysicalDeviceInterface*> enumeratePhysicalDevices() const = 0;
    virtual PFN_vkVoidFunction getProcAddress(const char* pName) const = 0;
    virtual vk::Instance get() const = 0;
    virtual void destroySurfaceKHR( vk::SurfaceKHR surface ) = 0;
    
};

} // namespace vira::vulkan

#endif // INSTANCE_INTERFACE_HPP
