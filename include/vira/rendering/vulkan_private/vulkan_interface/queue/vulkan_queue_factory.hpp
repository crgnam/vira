#ifndef VULKAN_QUEUE_FACTORY_HPP
#define VULKAN_QUEUE_FACTORY_HPP

#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/vulkan_queue.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"

namespace vira::vulkan {

class VulkanQueueFactory : public QueueFactoryInterface {
public:
    QueueInterface* createQueue(std::unique_ptr<DeviceInterface> device, uint32_t queueFamilyIndex, uint32_t queueIndex, vk::DispatchLoaderDynamic dldi) const override {
        vk::Queue queue = device.getQueue(queueFamilyIndex, queueIndex, dldi);
        return new VulkanQueue(queue, dldi);
    }
};

} // namespace vira::vulkan

#endif // VULKAN_QUEUE_FACTORY_HPP
