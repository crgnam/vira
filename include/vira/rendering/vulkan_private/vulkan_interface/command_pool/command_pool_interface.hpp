#ifndef COMMAND_POOL_INTERFACE_HPP
#define COMMAND_POOL_INTERFACE_HPP

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

class CommandPoolInterface {
public:
    virtual ~CommandPoolInterface() = default;
    virtual const vk::CommandPool& getVkCommandPool() const = 0;
    virtual bool isValid() const = 0;
    virtual const vk::UniqueCommandPool& getUniqueCommandPool() const = 0;
};

} // namespace vira::vulkan

#endif // COMMAND_POOL_INTERFACE_HPP
