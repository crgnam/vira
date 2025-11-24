#ifndef VIRA_RENDERING_VULKAN_PIPELINE_HPP
#define VIRA_RENDERING_VULKAN_PIPELINE_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"

namespace vira::vulkan {

	// Forward declare
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	class VulkanPathTracer;

	/**
     * @struct PipelineConfigInfo 
     * @brief Manages CreateInfo objects for pipeline creation/configuration
	 * 
     */	
	struct PipelineConfigInfo {
        PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		vk::PipelineViewportStateCreateInfo viewportInfo;
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
		vk::PipelineMultisampleStateCreateInfo multisampleInfo;
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<vk::DynamicState> dynamicStateEnables;
		vk::PipelineDynamicStateCreateInfo dynamicStateInfo;

		// These cannot get default value and must be set for each pipeline:
		vk::PipelineLayout pipelineLayout;
		vk::RenderPass renderPass;
		uint32_t subpass = 0;
	};

	/**
	 * @brief Represents a Vulkan graphics or ray tracing pipeline.
	 *
	 * @details This class manages the creation and binding of Vulkan pipelines for 
	 * rasterization and ray tracing. It handles shader module management, 
	 * pipeline configuration, and command buffer binding.
	 *
	 * @tparam TMeshFloat Floating-point type for mesh data.
	 * @tparam TSpectral Spectral representation type used for shading.
	 *
	 * ### Member Variables:
	 *
	 * - **Device and Pipeline:**
	 *   - `viraDevice`: Pointer to the Vulkan device managing resources.
	 *   - `pipeline`: Vulkan pipeline handle.
	 *
	 * - **Shader Modules:**
	 *   - `vertShaderModule`: Unique handle to the vertex shader module.
	 *   - `fragShaderModule`: Unique handle to the fragment shader module.
	 *
	 * - **Ray Tracing Pipeline Properties:**
	 *   - `raygenShaderCount`: Number of ray generation shaders.
	 *   - `missShaderCount`: Number of miss shaders.
	 *   - `hitShaderCount`: Number of hit shaders.
	 *   - `shaderGroups`: List of ray tracing shader groups.
	 *
	 */
	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	class ViraPipeline {
	public:
		
		ViraPipeline(ViraDevice* device);
		ViraPipeline(ViraDevice* device, 
			        const std::string& vertFilepath, 
			        const std::string& fragFilepath, 
			        const PipelineConfigInfo* configInfo);

		~ViraPipeline();

		ViraPipeline(const ViraPipeline&) = delete;
		ViraPipeline& operator=(const ViraPipeline&) = delete;

        /**
         * @brief Binds the pipeline to a command buffer
         */          		
		virtual void bind(CommandBufferInterface* commandBuffer);

		
        /**
         * @brief Populates configInfo with default configuration settings
         */          		
		static void defaultPipelineConfigInfo(PipelineConfigInfo* configInfo);

		/**
		 * @brief Creates a pathtracing pipeline (not implemented, done manually for now)
		 */
		void createPathTracingPipeline(vk::RayTracingPipelineCreateInfoKHR pipelineInfo);

		/**
		 * @brief Creates a rasterization (general graphics) pipeline 
		 * given shaders and config info
		 */
		void createRasterizationPipeline(const std::string& vertFilepath,
									const std::string& fragFilepath,
									const PipelineConfigInfo* configInfo);

		// Get Methods
		vk::Pipeline& getPipeline() { return pipeline; };									
		uint32_t getRaygenShaderCount() {return raygenShaderCount;};
		uint32_t getMissShaderCount() {return missShaderCount;};
		uint32_t getHitShaderCount() {return hitShaderCount;};

		// Public Member Variables
		std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;


	protected:

		// Protected Member Variables
		ViraDevice* viraDevice; 
		vk::Pipeline pipeline;
		
		vk::UniqueShaderModule vertShaderModule;
		vk::UniqueShaderModule fragShaderModule;

		uint32_t raygenShaderCount = 0;
		uint32_t missShaderCount = 0;
		uint32_t hitShaderCount = 0;

	};

}

#include "implementation/rendering/vulkan_private/pipeline.ipp"

#endif