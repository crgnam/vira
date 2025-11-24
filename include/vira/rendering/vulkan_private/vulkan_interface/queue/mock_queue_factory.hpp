#ifndef MOCK_QUEUE_FACTORY_HPP
#define MOCK_QUEUE_FACTORY_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/mock_queue.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"

namespace vira::vulkan {

class MockQueueFactory : public QueueFactoryInterface {
public:
    QueueInterface* createQueue(std::unique_ptr<DeviceInterface> device, uint32_t queueFamilyIndex, uint32_t queueIndex) const override {
        return new MockQueue();
    }
};

} // namespace vira::vulkan

#endif // MOCK_QUEUE_FACTORY_HPP
