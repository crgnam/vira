#ifndef VULKAN_COMMAND_POOL_FACTORY_HPP
#define VULKAN_COMMAND_POOL_FACTORY_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/vulkan_command_pool.hpp"
#include "device/device_interface.hpp"

namespace vira::vulkan {

class VulkanCommandPoolFactory : public CommandPoolFactoryInterface {
public:
    std::unique_ptr<CommandPoolInterface> createCommandPool(std::unique_ptr<DeviceInterface> device, const vk::CommandPoolCreateInfo& createInfo) const override {
        std::unique_ptr<CommandPoolInterface> commandPool = device->createCommandPool(createInfo);
        return std::make_unique<VulkanCommandPool>(commandPool);
    }
};

} // namespace vira::vulkan


#endif // VULKAN_COMMAND_POOL_FACTORY_HPP
