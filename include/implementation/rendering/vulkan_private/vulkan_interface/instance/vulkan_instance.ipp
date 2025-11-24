#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/vulkan_physical_device.hpp"

namespace vira::vulkan {

VulkanInstance::VulkanInstance() {}
    
VulkanInstance::VulkanInstance(vk::UniqueInstance uinstance) : instance_(std::move(uinstance)) {}  

VulkanInstance::VulkanInstance(vk::UniqueInstance uinstance, vk::DispatchLoaderDynamic* dldi) : instance_(std::move(uinstance)) {
    setDispatch(dldi);
};

VulkanInstance::~VulkanInstance() {}

std::vector<PhysicalDeviceInterface*> VulkanInstance::enumeratePhysicalDevices() const {

    std::vector<vk::PhysicalDevice> physicalDevices = instance_->enumeratePhysicalDevices(*dldi);
    std::vector<PhysicalDeviceInterface*> physicalDeviceInterfaces;

    for (auto& physicalDevice : physicalDevices) {
        physicalDeviceInterfaces.push_back(new VulkanPhysicalDevice(physicalDevice, dldi));
    }
    return physicalDeviceInterfaces;
}

PFN_vkVoidFunction VulkanInstance::getProcAddress(const char* pName) const {
    return instance_->getProcAddr(pName, *dldi);
}

vk::Instance VulkanInstance::get() const {
    return instance_.get();
}

void VulkanInstance::destroySurfaceKHR(vk::SurfaceKHR surface ) {
    instance_->destroySurfaceKHR(surface, nullptr, *dldi);
}

} 
