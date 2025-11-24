#ifndef MOCK_INSTANCE_HPP
#define MOCK_INSTANCE_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"

namespace vira::vulkan {

class MockInstance : public InstanceInterface {
public:
    MockInstance() = default;

    MockInstance(vk::UniqueInstance uinstance) : instance_(std::move(uinstance)) {}

    ~MockInstance() override = default;

    // Mock method for enumeratePhysicalDevices
    MOCK_METHOD(std::vector<PhysicalDeviceInterface*>, enumeratePhysicalDevices, (), (const, override));

    // Mock method for getProcAddress
    MOCK_METHOD(PFN_vkVoidFunction, getProcAddress, (const char* pName), (const, override));

    // Mock method for get
    MOCK_METHOD(vk::Instance, get, (), (const, override));

    // Mock method for destroySurfaceKHR
    MOCK_METHOD(void, destroySurfaceKHR, (vk::SurfaceKHR surface), (override));

private:

    vk::UniqueInstance instance_;

};

} // namespace vira::vulkan

#endif // MOCK_INSTANCE_HPP






