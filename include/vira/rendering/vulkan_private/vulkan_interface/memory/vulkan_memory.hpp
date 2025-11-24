#ifndef VULKAN_DEVICE_MEMORY_HPP
#define VULKAN_DEVICE_MEMORY_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"

namespace vira::vulkan {

class VulkanMemory : public MemoryInterface {
public:
    VulkanMemory(vk::UniqueDeviceMemory memory);
    ~VulkanMemory() = default;

    vk::DeviceMemory getMemory() const override;

    vk::UniqueDeviceMemory& getUniqueMemory() override;
    vk::UniqueDeviceMemory moveUniqueMemory() override;

private:
    vk::UniqueDeviceMemory memory_;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.ipp"

#endif // VULKAN_DEVICE_MEMORY_HPP


