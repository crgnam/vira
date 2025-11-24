#ifndef INSTANCE_FACTORY_INTERFACE_HPP
#define INSTANCE_FACTORY_INTERFACE_HPP

#include <memory>

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"

namespace vira::vulkan {

class InstanceFactoryInterface {
public:
    virtual ~InstanceFactoryInterface() = default;
    virtual std::unique_ptr<InstanceInterface> createInstance(const vk::InstanceCreateInfo& createInfo, vk::DispatchLoaderDynamic* dldi) const = 0;
};

} // namespace vira::vulkan

#endif // INSTANCE_FACTORY_INTERFACE_HPP
