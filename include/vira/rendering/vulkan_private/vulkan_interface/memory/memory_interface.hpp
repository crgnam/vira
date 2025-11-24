#ifndef DEVICE_MEMORY_INTERFACE_HPP
#define DEVICE_MEMORY_INTERFACE_HPP

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

class MemoryInterface {
public:
    virtual ~MemoryInterface() = default;

    virtual vk::DeviceMemory getMemory() const = 0;
    virtual vk::UniqueDeviceMemory& getUniqueMemory() = 0; 
    virtual vk::UniqueDeviceMemory moveUniqueMemory() = 0; 

};

} // namespace vira::vulkan

#endif // DEVICE_MEMORY_INTERFACE_HPP
