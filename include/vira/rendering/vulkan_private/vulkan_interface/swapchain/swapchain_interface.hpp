#ifndef SWAPCHAIN_INTERFACE_HPP
#define SWAPCHAIN_INTERFACE_HPP

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

class SwapchainInterface {
public:
    virtual ~SwapchainInterface() = default;

    virtual std::vector<vk::Image> getSwapchainImages() const = 0;
    virtual vk::SwapchainKHR getVkSwapchain() const = 0;
};

} // namespace vira::vulkan

#endif // SWAPCHAIN_INTERFACE_HPP
