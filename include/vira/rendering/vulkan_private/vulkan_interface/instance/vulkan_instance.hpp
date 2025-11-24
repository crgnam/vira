#ifndef VULKAN_INSTANCE_HPP
#define VULKAN_INSTANCE_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"

namespace vira::vulkan {

class VulkanInstance : public InstanceInterface {
public:

    VulkanInstance();
    VulkanInstance(vk::UniqueInstance uinstance);
    VulkanInstance(vk::UniqueInstance uinstance, vk::DispatchLoaderDynamic* dldi);
    ~VulkanInstance() override;
    // vk::UniqueInstance createInstanceUnique(const vk::InstanceCreateInfo& createInfo) override;
    std::vector<PhysicalDeviceInterface*> enumeratePhysicalDevices() const override;
    PFN_vkVoidFunction getProcAddress(const char* pName) const override;
    vk::Instance get() const override;
    void destroySurfaceKHR(vk::SurfaceKHR surface ) override;
    void setDispatch(vk::DispatchLoaderDynamic* newDldi) {this->dldi = newDldi;};
    // vk::UniqueInstance moveUniqueInstance() {return std::move(instance_);};

private:
    vk::UniqueInstance instance_;
    vk::DispatchLoaderDynamic* dldi;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance.ipp"
#endif // VULKAN_INSTANCE_HPP