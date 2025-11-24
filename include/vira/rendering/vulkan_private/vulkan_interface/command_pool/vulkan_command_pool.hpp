#ifndef VULKAN_COMMAND_POOL_HPP
#define VULKAN_COMMAND_POOL_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_interface.hpp"

namespace vira::vulkan {

class VulkanCommandPool : public CommandPoolInterface {
public:
    VulkanCommandPool(vk::UniqueCommandPool&& commandPool);
    ~VulkanCommandPool() override;
    const vk::CommandPool& getVkCommandPool() const override;
    // Override the isValid() method to check if the Vulkan CommandPool is valid
    bool isValid() const override {
        return static_cast<bool>(commandPool_);  // Command pool is valid if not VK_NULL_HANDLE
    }
    virtual const vk::UniqueCommandPool& getUniqueCommandPool() const override;

private:
    vk::UniqueCommandPool commandPool_;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/command_pool/vulkan_command_pool.ipp"

#endif // VULKAN_COMMAND_POOL_HPP
