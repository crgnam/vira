#ifndef MOCK_VULKAN_COMMAND_POOL_HPP
#define MOCK_VULKAN_COMMAND_POOL_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_interface.hpp"

namespace vira::vulkan {

class MockCommandPool : public CommandPoolInterface {
public:
    MOCK_METHOD(const vk::CommandPool&, getVkCommandPool, (), (const, override));
    bool isValid() const override {
        return true;
    }
    MOCK_METHOD(const vk::UniqueCommandPool&, getUniqueCommandPool, (), (const, override));

};

} // namespace vira::vulkan

#endif // MOCK_VULKAN_COMMAND_POOL_HPP
