#ifndef MOCK_VULKAN_QUEUE_HPP
#define MOCK_VULKAN_QUEUE_HPP

#include "gmock/gmock.h"

#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"

namespace vira::vulkan {

class MockQueue : public QueueInterface {
public:
    MOCK_METHOD(void, submit, (const vk::ArrayProxy<const vk::SubmitInfo> submitInfos, vk::Fence fence), (const, override));
    MOCK_METHOD(void, waitIdle, (), (const, override));
    MOCK_METHOD(vk::Result, presentKHR, (const vk::PresentInfoKHR& presentInfo), (const, override));
    MOCK_METHOD(void, setDispatch, (vk::DispatchLoaderDynamic* dldi), (override));
    MOCK_METHOD(vk::Queue, getQueue, (), (override));
};

} // namespace vira::vulkan

#endif // MOCK_VULKAN_QUEUE_HPP
