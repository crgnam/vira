#ifndef COMMAND_POOL_FACTORY_INTERFACE_HPP
#define COMMAND_POOL_FACTORY_INTERFACE_HPP

#include <memory>

#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_interface.hpp"

namespace vira::vulkan {

class CommandPoolFactoryInterface {
public:
    virtual ~CommandPoolFactoryInterface() = default;
    virtual std::unique_ptr<CommandPoolInterface> createCommandPool(std::shared_ptr<DeviceInterface> device, const vk::CommandPoolCreateInfo& createInfo) const = 0;
};

} // namespace vira::vulkan

#endif // COMMAND_POOL_FACTORY_INTERFACE_HPP
