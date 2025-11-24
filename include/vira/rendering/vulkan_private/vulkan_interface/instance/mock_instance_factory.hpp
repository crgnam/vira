#ifndef MOCK_INSTANCE_FACTORY_HPP
#define MOCK_INSTANCE_FACTORY_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/mock_instance.hpp"

namespace vira::vulkan {

class MockInstanceFactory : public InstanceFactoryInterface {
public:
    std::unique_ptr<InstanceInterface> createInstance(const vk::InstanceCreateInfo& createInfo, vk::DispatchLoaderDynamic* dldi) const override {
        (void)dldi; // TODO Consider removing
        (void)createInfo; // TODO Consider removing

        vk::UniqueInstance instance = vk::UniqueInstance(reinterpret_cast<VkInstance>(1));  // Mock valid non-null instance
        // vk::UniqueInstance instance = vk::createInstanceUnique(createInfo);
        return std::make_unique<VulkanInstance>(std::move(instance)); 
    }
};

} // namespace vira::vulkan

#endif // MOCK_INSTANCE_FACTORY_HPP
