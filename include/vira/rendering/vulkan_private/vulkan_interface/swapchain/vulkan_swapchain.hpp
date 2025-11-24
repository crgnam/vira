#ifndef VULKAN_SWAPCHAIN_HPP
#define VULKAN_SWAPCHAIN_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"

namespace vira::vulkan {

class VulkanSwapchain : public SwapchainInterface {
public:
    VulkanSwapchain(vk::UniqueSwapchainKHR swapchain);
    std::vector<vk::Image> getSwapchainImages() const override;
    virtual vk::SwapchainKHR getVkSwapchain() const override;

private:
    vk::UniqueSwapchainKHR swapchain_;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/swapchain/vulkan_swapchain.ipp"

#endif // VULKAN_SWAPCHAIN_HPP
