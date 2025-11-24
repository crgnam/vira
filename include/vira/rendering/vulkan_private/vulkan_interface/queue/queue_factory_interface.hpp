#ifndef QUEUE_FACTORY_INTERFACE_HPP
#define QUEUE_FACTORY_INTERFACE_HPP

#include <memory>

#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"

namespace vira::vulkan {

class DeviceInterface;

class QueueFactoryInterface {
public:
    virtual ~QueueFactoryInterface() = default;
    virtual QueueInterface* createQueue(std::unique_ptr<DeviceInterface> device, uint32_t queueFamilyIndex, uint32_t queueIndex) const = 0;
};

} // namespace vira::vulkan

#endif // QUEUE_FACTORY_INTERFACE_HPP
