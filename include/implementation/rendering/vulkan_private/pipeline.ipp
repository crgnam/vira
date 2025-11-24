#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <string>

#include "vulkan/vulkan.hpp"

#include "vira/math.hpp"
#include "vira/utils/utils.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/vulkan_private/vulkan_render_resources.hpp"

namespace vira::vulkan {

	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	ViraPipeline<TSpectral, TMeshFloat>::ViraPipeline(ViraDevice* device) : viraDevice{device} {};

	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	ViraPipeline<TSpectral, TMeshFloat>::ViraPipeline(ViraDevice* device,
							 const std::string& vertFilepath,
							 const std::string& fragFilepath,
							 const PipelineConfigInfo* configInfo) : viraDevice{device}
	{
		createRasterizationPipeline(vertFilepath, fragFilepath, configInfo);
	};

	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	ViraPipeline<TSpectral, TMeshFloat>::~ViraPipeline()
	{
		if (pipeline) {
			viraDevice->device()->destroyPipeline(pipeline, nullptr);
			pipeline = nullptr;
		}
	
		if (fragShaderModule) fragShaderModule.reset();
		if (vertShaderModule) vertShaderModule.reset();

	};


	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	void ViraPipeline<TSpectral, TMeshFloat>::createPathTracingPipeline(vk::RayTracingPipelineCreateInfoKHR pipelineInfo)
	{
		// TODO: encapsulate pathtracing pipeline creation here
	};

	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	void ViraPipeline<TSpectral, TMeshFloat>::createRasterizationPipeline(const std::string& vertFilepath,
											 const std::string& fragFilepath,
											 const PipelineConfigInfo* configInfo)
	{
		if (!configInfo->pipelineLayout) {
			throw std::runtime_error("Cannot create graphics pipeline: no pipelineLayout provided in configInfo");
		}

		if (!configInfo->renderPass) {
			throw std::runtime_error("Cannot create graphics pipeline: no renderPass provided in configInfo");
		};

		auto vertCode = readFile(vertFilepath);
		auto fragCode = readFile(fragFilepath);

		// Initialize shader modules:
		vertShaderModule = viraDevice->createShaderModule(vertCode);
		fragShaderModule = viraDevice->createShaderModule(fragCode);

		std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex);
		shaderStages[0].setModule(vertShaderModule.get());
		shaderStages[0].setPName("main");
		shaderStages[0].setFlags({});
		shaderStages[0].setPNext(nullptr);
		shaderStages[0].setPSpecializationInfo(nullptr);

		shaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment);
		shaderStages[1].setModule(fragShaderModule.get());
		shaderStages[1].setPName("main");
		shaderStages[1].setFlags({});
		shaderStages[1].setPNext(nullptr);
		shaderStages[1].setPSpecializationInfo(nullptr);

		auto bindingDescriptions = VertexVec4<TSpectral, TMeshFloat>::getBindingDescriptions();
		auto attributeDescriptions = VertexVec4<TSpectral, TMeshFloat>::getAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDescriptions.size()));
		vertexInputInfo.setVertexBindingDescriptionCount(static_cast<uint32_t>(bindingDescriptions.size()));
		vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);
		vertexInputInfo.setVertexBindingDescriptions(bindingDescriptions);


		std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments(8);
		for (auto& blendAttachment : blendAttachments) {
			blendAttachment = vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(VK_FALSE)
				.setColorWriteMask(
					vk::ColorComponentFlagBits::eR | 
					vk::ColorComponentFlagBits::eG | 
					vk::ColorComponentFlagBits::eB | 
					vk::ColorComponentFlagBits::eA)
				.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
				.setAlphaBlendOp(vk::BlendOp::eAdd);
		}

		vk::PipelineColorBlendStateCreateInfo colorBlendInfo = vk::PipelineColorBlendStateCreateInfo{}
			.setLogicOpEnable(VK_FALSE)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachments(blendAttachments);

		vk::GraphicsPipelineCreateInfo pipelineInfo{};

		// Set shader stages directly
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();

		// Set the vertex input state
		pipelineInfo.pVertexInputState = &vertexInputInfo;

		// Set the input assembly state
		pipelineInfo.pInputAssemblyState = &configInfo->inputAssemblyInfo;

		// Set the viewport state
		pipelineInfo.pViewportState = &configInfo->viewportInfo;

		// Set the rasterization state
		pipelineInfo.pRasterizationState = &configInfo->rasterizationInfo;

		// Set the multisample state
		pipelineInfo.pMultisampleState = &configInfo->multisampleInfo;

		// Set the color blend state
		pipelineInfo.pColorBlendState = &colorBlendInfo;

		// Set the depth stencil state
		pipelineInfo.pDepthStencilState = &configInfo->depthStencilInfo;

		// Set the dynamic state
		pipelineInfo.pDynamicState = &configInfo->dynamicStateInfo;

		// Set the pipeline layout
		pipelineInfo.layout = configInfo->pipelineLayout;

		// Set the render pass and subpass index
		pipelineInfo.renderPass = configInfo->renderPass;
		pipelineInfo.subpass = configInfo->subpass;

		// Set the base pipeline index and handle
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = nullptr;

		vk::ResultValue<vk::Pipeline> result = viraDevice->device()->createGraphicsPipeline(nullptr, pipelineInfo, nullptr);
		if (result.result != vk::Result::eSuccess) {
 		   std::cerr << "Failed to create rasterization pipeline: " << vk::to_string(result.result) << std::endl;
		} else {
    		pipeline = result.value;
   		}
	};
	
	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	void ViraPipeline<TSpectral, TMeshFloat>::bind(CommandBufferInterface* commandBuffer)
	{
		commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	};

	template <IsFloat TMeshFloat, IsSpectral TSpectral>
	void ViraPipeline<TSpectral, TMeshFloat>::defaultPipelineConfigInfo(PipelineConfigInfo* configInfo)
	{
		// Configure input assembly:
		configInfo->inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
		configInfo->inputAssemblyInfo.setPrimitiveRestartEnable(VK_FALSE);

		// Configure viewport and scissor:
		configInfo->viewportInfo.setViewports(nullptr);
		configInfo->viewportInfo.setScissors(nullptr);
		configInfo->viewportInfo.setViewportCount(1);
		configInfo->viewportInfo.setScissorCount(1);
		
		// Configure rasterization:
		configInfo->rasterizationInfo.setDepthClampEnable(VK_FALSE);
		configInfo->rasterizationInfo.setRasterizerDiscardEnable(VK_FALSE);
		configInfo->rasterizationInfo.setPolygonMode(vk::PolygonMode::eFill);
		configInfo->rasterizationInfo.setLineWidth(1.0f);
		configInfo->rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
		configInfo->rasterizationInfo.setFrontFace(vk::FrontFace::eCounterClockwise);
		configInfo->rasterizationInfo.setDepthBiasEnable(VK_FALSE);
		configInfo->rasterizationInfo.setDepthBiasConstantFactor(0.0f);
		configInfo->rasterizationInfo.setDepthBiasClamp(0.0f);
		configInfo->rasterizationInfo.setDepthBiasSlopeFactor(0.0f);

		// Configure multi-sampling:
		configInfo->multisampleInfo.setSampleShadingEnable(VK_FALSE);
		configInfo->multisampleInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);
		configInfo->multisampleInfo.setMinSampleShading(1.0f);
		configInfo->multisampleInfo.pSampleMask = nullptr;
		configInfo->multisampleInfo.setAlphaToCoverageEnable(VK_FALSE);
		configInfo->multisampleInfo.setAlphaToOneEnable(VK_FALSE);

		// Configure color blend attachment:
		configInfo->colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |vk::ColorComponentFlagBits::eA);
		configInfo->colorBlendAttachment.setBlendEnable(VK_FALSE);
		configInfo->colorBlendAttachment.setSrcColorBlendFactor(vk::BlendFactor::eOne);
		configInfo->colorBlendAttachment.setDstColorBlendFactor(vk::BlendFactor::eZero);
		configInfo->colorBlendAttachment.setColorBlendOp(vk::BlendOp::eAdd);
		configInfo->colorBlendAttachment.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
		configInfo->colorBlendAttachment.setDstAlphaBlendFactor(vk::BlendFactor::eZero);
		configInfo->colorBlendAttachment.setAlphaBlendOp(vk::BlendOp::eAdd);

		// Configure depth and stencil buffer:
		configInfo->depthStencilInfo.setDepthTestEnable(VK_FALSE);
		configInfo->depthStencilInfo.setDepthWriteEnable(VK_TRUE);
		configInfo->depthStencilInfo.setDepthCompareOp(vk::CompareOp::eLess);
		configInfo->depthStencilInfo.setDepthBoundsTestEnable(VK_FALSE);
		configInfo->depthStencilInfo.setMinDepthBounds(0.0f);
		configInfo->depthStencilInfo.setMaxDepthBounds(1.0f);
		configInfo->depthStencilInfo.setStencilTestEnable(VK_FALSE);
		configInfo->depthStencilInfo.setFront({});
		configInfo->depthStencilInfo.setBack({});

		configInfo->dynamicStateEnables = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		configInfo->dynamicStateInfo.setDynamicStates(configInfo->dynamicStateEnables);
		configInfo->dynamicStateInfo.setDynamicStateCount(static_cast<uint32_t>(configInfo->dynamicStateEnables.size()));
		configInfo->dynamicStateInfo.setFlags({});
	};
}