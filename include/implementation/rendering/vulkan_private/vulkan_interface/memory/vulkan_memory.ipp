namespace vira::vulkan {

VulkanMemory::VulkanMemory(vk::UniqueDeviceMemory memory)
    : memory_(std::move(memory)) {}

vk::DeviceMemory VulkanMemory::getMemory() const {
    return *memory_;
}

vk::UniqueDeviceMemory& VulkanMemory::getUniqueMemory() {
    return memory_;
}
vk::UniqueDeviceMemory VulkanMemory::moveUniqueMemory() {
    return std::move(memory_);
}

} 
