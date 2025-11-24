#ifndef QUEUE_INTERFACE_HPP
#define QUEUE_INTERFACE_HPP

#include "vulkan/vulkan.hpp"

namespace vira::vulkan {

class QueueInterface {
public:
    virtual ~QueueInterface() = default;

    virtual void submit(const vk::ArrayProxy<const vk::SubmitInfo> submitInfos, vk::Fence fence) const = 0;
    virtual void waitIdle() const = 0;
    virtual vk::Result presentKHR(const vk::PresentInfoKHR& presentInfo) const = 0;
    virtual void setDispatch(vk::DispatchLoaderDynamic* dldi) = 0;
    virtual vk::Queue getQueue() = 0;
};

} // namespace vira::vulkan

#endif // QUEUE_INTERFACE_HPP
