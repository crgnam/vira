#ifndef MOCK_COMMAND_POOL_FACTORY_HPP
#define MOCK_COMMAND_POOL_FACTORY_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/mock_command_pool.hpp"

namespace vira::vulkan {

class MockCommandPoolFactory : public CommandPoolFactoryInterface {
public:
    std::unique_ptr<CommandPoolInterface> createCommandPool(std::shared_ptr<DeviceInterface> device, const vk::CommandPoolCreateInfo& createInfo) const override {
        return std::make_unique<MockCommandPool>();
    }
};

} // namespace vira::vulkan

#endif // MOCK_COMMAND_POOL_FACTORY_HPP
