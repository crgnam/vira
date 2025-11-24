// Preprocessor Macros
#ifndef VIRA_VULKAN_HPP

#define VIRA_VULKAN_HPP
#undef VULKAN_HPP_DISABLE_ENHANCED_MODE
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_ENABLE_BETA_EXTENSIONS
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME "VK_KHR_get_physical_device_properties2"        
#define VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME "VK_KHR_ray_tracing_pipeline"
#define VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME "VK_KHR_acceleration_structure"
#define VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME "VK_KHR_deferred_host_operations"
#define VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME "VK_EXT_descriptor_indexing"
#define VK_KHR_RAY_QUERY_EXTENSION_NAME "VK_KHR_ray_query"
#define VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME "VK_KHR_shader_non_semantic_info"

#endif

#include "vulkan/vulkan.hpp"

// Include Vulkan Renderer Headers
#include "vira/rendering/vulkan/viewport_camera.hpp"
#include "vira/rendering/vulkan/vulkan_path_tracer.hpp"
#include "vira/rendering/vulkan/vulkan_rasterizer.hpp"

#include "vira/rendering/vulkan_private/buffer.hpp"
#include "vira/rendering/vulkan_private/descriptors.hpp"
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/pipeline.hpp"
#include "vira/rendering/vulkan_private/render_loop.hpp"
#include "vira/rendering/vulkan_private/swapchain.hpp"
#include "vira/rendering/vulkan_private/vulkan_camera.hpp"
#include "vira/rendering/vulkan_private/vulkan_light.hpp"
#include "vira/rendering/vulkan_private/vulkan_material.hpp"
#include "vira/rendering/vulkan_private/window.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance_factory.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/mock_instance_factory.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/mock_instance.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/vulkan_device_factory.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/mock_device_factory.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/vulkan_device.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/mock_device.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/vulkan_physical_device.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/mock_physical_device.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/vulkan_command_buffer.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/mock_command_buffer.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/command_pool_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/vulkan_command_pool.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_pool/mock_command_pool.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/vulkan_queue.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/mock_queue.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/mock_memory.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/vulkan_swapchain.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/mock_swapchain.hpp"