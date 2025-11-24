#ifndef VULKAN_INSTANCE_FACTORY_HPP
#define VULKAN_INSTANCE_FACTORY_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance.hpp"

namespace vira::vulkan {

class VulkanInstanceFactory : public InstanceFactoryInterface {
public:
    std::unique_ptr<InstanceInterface> createInstance(const vk::InstanceCreateInfo& createInfo, vk::DispatchLoaderDynamic* dldi) const override {
        vk::UniqueInstance instance = vk::createInstanceUnique(createInfo,nullptr, *dldi);
        return std::make_unique<VulkanInstance>(std::move(instance), dldi);        
    }
};

} // namespace vira::vulkan

#endif // VULKAN_INSTANCE_FACTORY_HPP
