#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"

namespace vira::vulkan {

VulkanSwapchain::VulkanSwapchain(vk::UniqueSwapchainKHR swapchain)
    : swapchain_(std::move(swapchain)) {}

std::vector<vk::Image> VulkanSwapchain::getSwapchainImages() const {
    // TODO: return vector of swapchain images when onscreen rendering implemented
    return {}; 
}

vk::SwapchainKHR VulkanSwapchain::getVkSwapchain() const {
    return *swapchain_;
}

} 
