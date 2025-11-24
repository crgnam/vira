#ifndef MOCK_VULKAN_SWAPCHAIN_HPP
#define MOCK_VULKAN_SWAPCHAIN_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"

namespace vira::vulkan {

class MockSwapchain : public SwapchainInterface {
public:
    MOCK_METHOD(std::vector<vk::Image>, getSwapchainImages, (), (const, override));
    MOCK_METHOD(vk::SwapchainKHR, getVkSwapchain, (),  (const, override));
};

} // namespace vira::vulkan

#endif // MOCK_VULKAN_SWAPCHAIN_HPP
