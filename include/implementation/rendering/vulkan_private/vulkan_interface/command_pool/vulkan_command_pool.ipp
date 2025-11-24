namespace vira::vulkan {

VulkanCommandPool::~VulkanCommandPool() {}

VulkanCommandPool::VulkanCommandPool(vk::UniqueCommandPool&& commandPool)
    : commandPool_(std::move(commandPool)) {}

const vk::CommandPool& VulkanCommandPool::getVkCommandPool() const {
    return commandPool_.get();      // Return the underlying vk::CommandPool
}

const vk::UniqueCommandPool& VulkanCommandPool::getUniqueCommandPool() const {return commandPool_;};

}
