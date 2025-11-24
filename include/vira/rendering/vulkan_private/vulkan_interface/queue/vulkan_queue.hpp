#ifndef VULKAN_QUEUE_HPP
#define VULKAN_QUEUE_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"

namespace vira::vulkan {

class VulkanQueue : public QueueInterface {
public:
    VulkanQueue(vk::Queue queue);
    VulkanQueue(vk::Queue queue, vk::DispatchLoaderDynamic* dldi);

    void submit(const vk::ArrayProxy<const vk::SubmitInfo> submitInfos, vk::Fence fence = vk::Fence(nullptr)) const override;
    void waitIdle() const override;
    vk::Result presentKHR(const vk::PresentInfoKHR& presentInfo) const override;
    void setDispatch(vk::DispatchLoaderDynamic* newDldi) override {this->dldi = newDldi;};
    vk::Queue getQueue() {return queue_;};
    
private:
    vk::Queue queue_;
    vk::DispatchLoaderDynamic* dldi;
};

} // namespace vira::vulkan

#include "implementation/rendering/vulkan_private/vulkan_interface/queue/vulkan_queue.ipp"

#endif // VULKAN_QUEUE_HPP
