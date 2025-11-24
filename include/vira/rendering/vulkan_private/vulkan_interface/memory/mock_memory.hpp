#ifndef MOCK_VULKAN_DEVICE_MEMORY_HPP
#define MOCK_VULKAN_DEVICE_MEMORY_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"

namespace vira::vulkan {

class MockMemory : public MemoryInterface {
public:
    MOCK_METHOD(vk::DeviceMemory, getMemory, (), (const, override));
    MOCK_METHOD(vk::UniqueDeviceMemory&, getUniqueMemory, (), (override));
    MOCK_METHOD(vk::UniqueDeviceMemory, moveUniqueMemory, (), (override));

};

} // namespace vira::vulkan

#endif // MOCK_VULKAN_DEVICE_MEMORY_HPP
