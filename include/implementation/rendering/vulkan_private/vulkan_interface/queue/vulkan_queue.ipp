namespace vira::vulkan {

VulkanQueue::VulkanQueue(vk::Queue queue) : queue_(queue) {}

VulkanQueue::VulkanQueue(vk::Queue queue, vk::DispatchLoaderDynamic* dldi) : queue_(queue) {
        setDispatch(dldi);
    }

void VulkanQueue::submit(const vk::ArrayProxy<const vk::SubmitInfo> submitInfos, vk::Fence fence) const {
    queue_.submit(submitInfos, fence, *dldi);
}

void VulkanQueue::waitIdle() const {
    queue_.waitIdle(*dldi);
}

vk::Result VulkanQueue::presentKHR(const vk::PresentInfoKHR& presentInfo) const {
    return queue_.presentKHR(presentInfo, *dldi);
}

} 
