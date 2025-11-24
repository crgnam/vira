#include <array>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <fstream>
#include <vector>

#include "vulkan/vulkan.hpp"
#include "glm/vec4.hpp"

#include "vira/rendering/vulkan/vulkan_path_tracer.hpp"
#include "vira/rendering/vulkan/vulkan_rasterizer.hpp"
#include "vira/rendering/vulkan_private/vulkan_render_resources.hpp"
#include "vira/rendering/vulkan_private/device.hpp"

namespace vira::vulkan {

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::ViraRenderLoop(ViraDevice* device, VulkanPathTracer<TSpectral, TFloat, TMeshFloat>* pathTracer, RenderLoopOptions options) : viraDevice{device}, pathTracer{pathTracer}, options{options}
	{
		// Set Some Render Constants
		nPixels = pathTracer->nPixels;
		maxFramesInFlight = pathTracer->getOptions().maxFramesInFlight;

		// Initialize Render Loop
		initialize();
	};


	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::ViraRenderLoop(ViraDevice* device, VulkanRasterizer<TSpectral, TFloat, TMeshFloat>* rasterizer, RenderLoopOptions options) : viraDevice{device}, rasterizer{rasterizer}, options{options}
	{
		// Set Some Render Constants
		nPixels = rasterizer->nPixels;
		maxFramesInFlight = rasterizer->getOptions().maxFramesInFlight;

		// Initialize Render Loop
		initialize();
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::~ViraRenderLoop()
	{
		freeCommandBuffers();
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::initialize() 
	{
		createCommandBuffers();
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::setViraSwapchain(std::unique_ptr<ViraSwapchain> swapchain) 
	{
        viraSwapchain = std::move(swapchain);
    }

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	int ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::getFrameIndex() const
	{
		// Check that there is an active frame
		if (!isFrameStarted) {
			throw std::runtime_error("Cannot get frame index when frame not in progress");
		}

		return currentFrameIndex;
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	CommandBufferInterface& ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::getCurrentCommandBuffer() const
	{
		// Check that there is an active frame
		if (!isFrameStarted) {
			throw std::runtime_error("Cannot cget command buffer while frame is not in progress");
		}
		
		return *commandBuffers[currentFrameIndex];
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::createCommandBuffers()
	{
		// Initialize enough command buffers for each frame in flight
		commandBuffers.resize(maxFramesInFlight); 

		// Allocate Command Buffers
		try {
			vk::CommandBufferAllocateInfo allocateInfo{};
			allocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
			// Command buffers may need to be created frequently.  So commandPool is used
			// to allocate space once.  Future command buffers are suballocated from the pool
			const CommandPoolInterface& cpool = viraDevice->getCommandPool();
			allocateInfo.setCommandPool(cpool.getVkCommandPool());
			allocateInfo.setCommandBufferCount(static_cast<uint32_t>(commandBuffers.size()));
			commandBuffers = viraDevice->device()->allocateCommandBuffers(allocateInfo);
		} catch (vk::SystemError& err) {
    		std::cerr << "Failed to allocate command buffers: " << err.what() << std::endl;
		}
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::runRasterizerRenderLoop() 
	{
		// Retrieve Render Loop Parameters from options
		double et = options.et;
		double step_size = options.step_size;
		size_t numFrames = options.numFrames;
		Resolution resolution = rasterizer->getCamera()->getResolution();

		std::cout << "\nBeginning Render Loop..." << std::endl;

		// =======================
		// Loop through Render Frames
        // =======================
		for (size_t i_frame = 0; i_frame < numFrames; i_frame++) {

			std::cout << "Rendering Frame: " << i_frame + 1 << " of " << numFrames << std::endl;

			// =======================
			// Update Vira & Vulkan Scene
			// =======================

			// Update Camera
			rasterizer->getCamera()->updateSpicePose(et);

			// Update Instances
			std::vector<std::shared_ptr<Instance<TSpectral, TFloat, TMeshFloat>>> instances = rasterizer->getScene()->getInstances();
			for(size_t i_instance=0; i_instance < instances.size(); ++i_instance) {
				std::shared_ptr<Instance<TSpectral, TFloat, TMeshFloat>> instance = instances[i_instance];
				instance->updateSpicePose(et);
			}

			// Update Lights
			std::vector<std::shared_ptr<Light<TSpectral, TFloat>>>  lights = rasterizer->getScene()->getLights();
			for(size_t i_light=0; i_light < instances.size(); ++i_light) {
				std::shared_ptr<Light<TSpectral, TFloat>> light = lights[i_light];
				light->updateSpicePose(et);
			}

			// Process Scene Graph
			rasterizer->getScene()->processSceneGraph();

			// Update Vulkan Scene
			rasterizer->updateVulkanScene();
			
			// =======================
			// Render
			// =======================

			// Begin Frame
			CommandBufferInterface* commandBuffer = beginFrame();

			// Begin Swapchain Render Pass
			beginRenderPass(commandBuffer);

			// Record Ray Tracing Commands
			rasterizer->recordDrawCommands(commandBuffer->get());

			// End Swapchain Render Pass
			endRenderPass(commandBuffer);

			// Execute Command Buffer and end frame
			endFrame(commandBuffer);
		
			// Confirm Frame Complete
			viraSwapchain->waitForImage();

			// =======================
			// Output
			// =======================

			// == Write rendered attachments to Camera RenderPass ==

			// Construct spectral set images vector 
			// TODO: Need to index into spectral color type image sets (albedo, emission)
			std::vector<std::vector<std::vector<vec4<float>>>> spectralImages;
			for ( size_t iImage = 0; iImage < rasterizer->nSpectralSets; iImage++) {

				// Initialize Data Vectors
				std::vector<std::vector<vec4<float>>> images;
				std::vector<vec4<float>> imageData;
				//vec4<float> sum;  // TODO Consider Removing

				// Extract Spectral Set Data from color type spectral set images
				imageData = viraDevice->extractSpectralSetData(viraSwapchain->albedoImage, resolution);
				images.push_back(imageData);

				imageData = viraDevice->extractSpectralSetData(viraSwapchain->directRadianceImage, resolution);
				images.push_back(imageData);

				imageData = viraDevice->extractSpectralSetData(viraSwapchain->indirectRadianceImage, resolution);
				images.push_back(imageData);

				imageData = viraDevice->extractSpectralSetData(viraSwapchain->receivedPowerImage, resolution);
				images.push_back(imageData);
			  
				std::vector<vira::vec4<float>> backgroundData;
				backgroundData.resize(resolution.x * resolution.y, vira::vec4<float>(0.0f, 0.0f, 0.0f, 0.0f));
				images.push_back(backgroundData);

				imageData = viraDevice->extractSpectralSetData(viraSwapchain->normalImage, resolution);
				images.push_back(imageData);

				// Add Spectral Set Images to spectralImages
				spectralImages.push_back(images);

			}
			
			// Initialize Scalar Images / Data Vector
			std::vector<std::vector<float>> scalarFloatImages;
			std::vector<float> scalarFloatData;

			// Create Alpha Image
			scalarFloatData = viraDevice->extractScalarFloatImageData(viraSwapchain->alphaImage, resolution, false);
			scalarFloatImages.push_back(scalarFloatData);

			// Create Depth Image
			scalarFloatData = viraDevice->extractScalarFloatImageData(viraSwapchain->depthImage, resolution, true);
			scalarFloatImages.push_back(scalarFloatData);

			// Update Camera's Render Pass with spectral and scalar images
			rasterizer->getCamera()->updateImagesFromVulkanPT(spectralImages, scalarFloatImages);

			// == Write rendered camera renderpass images to files ==

			// Fetch and save rendered images:
            std::string outputName = "GPU_RASTERIZER_FRAME_" + vira::utils::padZeros<3>(i_frame) + ".png";
            vira::Image<float> capturedImage = rasterizer->getCamera()->simulateSensor();
            vira::ImageInterface::write("vira_gpu_rasterizer_output/" + outputName, capturedImage);

			// Increment the Epoch
            et += step_size;			
		}
	}	

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::runPathTracerRenderLoop() 
	{
		
		// Retrieve Render Loop Parameters from options
		double et = options.et;
		double step_size = options.step_size;
		size_t numFrames = options.numFrames;

		std::cout << "\nBeginning Render Loop..." << std::endl;

		// Loop through Render Frames
        for (size_t i_frame = 0; i_frame < numFrames; i_frame++) {

			std::cout << "Rendering Frame: " << i_frame + 1 << " of " << numFrames << std::endl;

			// =======================
			// Update Vira & Vulkan Scene
			// =======================

			// Update Vira Scene
			pathTracer->getCamera()->updateSpicePose(et);
			std::vector<std::shared_ptr<Instance<TSpectral, TFloat, TMeshFloat>>> instances = pathTracer->getScene()->getInstances();
			for(size_t i_instance=0; i_instance < instances.size(); ++i_instance) {
				std::shared_ptr<Instance<TSpectral, TFloat, TMeshFloat>> instance = instances[i_instance];
				instance->updateSpicePose(et);
			}
			std::vector<std::shared_ptr<Light<TSpectral, TFloat>>>  lights = pathTracer->getScene()->getLights();
			for(size_t i_light=0; i_light < instances.size(); ++i_light) {
				std::shared_ptr<Light<TSpectral, TFloat>> light = lights[i_light];
				light->updateSpicePose(et);
			}
			pathTracer->getCamera()->updateSpicePose(et);
			pathTracer->getScene()->processSceneGraph();
			
			// Update Vulkan Scene
			pathTracer->updateVulkanScene();


			// =======================
			// Render
			// =======================

			// Begin Frame
			isFrameStarted = true;

			// Begin recording on current command buffer
			CommandBufferInterface& commandBuffer = getCurrentCommandBuffer();

			vk::CommandBufferBeginInfo beginInfo{};
			try {
				commandBuffer.begin(beginInfo);
			} catch (const vk::SystemError& err) {
				std::cerr << "Vulkan error: " << err.what() << std::endl;
				throw std::runtime_error("Failed to begin recording command buffer due to Vulkan error");
			} catch (const std::exception& err) {
				std::cerr << "Error: " << err.what() << std::endl;
				throw; // Re-throw the exception after logging it
			}

			// Record Ray Tracing Commands
			pathTracer->recordRayTracingCommands(commandBuffer.get());

			// End Command Writing
			try {
				commandBuffer.end();  
			} catch (vk::SystemError& err) {
				std::cerr << "Failed to record command buffer: " << err.what() << std::endl;
			}
			std::vector<CommandBufferInterface*> commandBuffers;
			commandBuffers.push_back(&commandBuffer);

			// Submit Command Buffers For Execution
			vk::Result result = viraSwapchain->submitCommandBuffers(commandBuffers, 0);

			// End Frame
			isFrameStarted = false;
			currentFrameIndex = 0;		


			// =======================
			// Output
			// =======================

			// == Shader Invocation Counts ==
			if(result == vk::Result::eSuccess) {

				std::cout << "Shader executions" << std::endl;
				uint32_t counterValue;
		
				pathTracer->debugCounterBuffer->map(sizeof(uint32_t), 0);
				void* pCounter = pathTracer->debugCounterBuffer->getMappedMemory();
				counterValue = *reinterpret_cast<uint32_t*>(pCounter);
				std::cout << "\tRaygen:\t" << counterValue << std::endl;
				*reinterpret_cast<uint32_t*>(pCounter) = 0;
				pathTracer->debugCounterBuffer->unmap();        

				pathTracer->debugCounterBuffer2->map(sizeof(uint32_t), 0);
				pCounter = pathTracer->debugCounterBuffer2->getMappedMemory();
				counterValue = *reinterpret_cast<uint32_t*>(pCounter);
				std::cout << "\tMiss:\t" << counterValue << std::endl;
				*reinterpret_cast<uint32_t*>(pCounter) = 0;
				pathTracer->debugCounterBuffer2->unmap();        

				pathTracer->debugCounterBuffer3->map(sizeof(uint32_t), 0);
				pCounter = pathTracer->debugCounterBuffer3->getMappedMemory();
				counterValue = *reinterpret_cast<uint32_t*>(pCounter);
				std::cout << "\tHit:\t" << counterValue << std::endl;
				*reinterpret_cast<uint32_t*>(pCounter) = 0;
				pathTracer->debugCounterBuffer3->unmap();        
				
			}  
	
			// == Copy spectral set image layers to data vector ==
			uint32_t layerCount = 6;

			// Transition Images to copyable layout
			for(size_t i=0; i < pathTracer->nSpectralSets; ++i) {
		
				viraDevice->transitionImageLayout(
					viraSwapchain->getSpectralImage(static_cast<uint32_t>(i)),
					vk::ImageLayout::eGeneral, // Current layout
					vk::ImageLayout::eTransferSrcOptimal, // Original layout
					vk::PipelineStageFlagBits::eRayTracingShaderKHR	, // Src stage
					vk::PipelineStageFlagBits::eTransfer, // Dst stage
					layerCount
				);				
		
			}

			viraDevice->transitionImageLayout(
				viraSwapchain->getScalarImage(),
				vk::ImageLayout::eGeneral, // Current layout
				vk::ImageLayout::eTransferSrcOptimal, // Original layout
				vk::PipelineStageFlagBits::eRayTracingShaderKHR	, // Src stage
				vk::PipelineStageFlagBits::eTransfer, // Dst stage
				layerCount
			);
			

			// Initialize spectral image data vector
			// This data vector holds data of all spectral sets for all
			// Color-like output
			vk::DeviceSize imageSize = nPixels*4 * sizeof(TFloat);
			std::vector<std::vector<std::vector<vira::vec4<TFloat>>>> spectralImages(
				pathTracer->nSpectralSets, std::vector<std::vector<vira::vec4<TFloat>>>(
					layerCount, std::vector<vira::vec4<TFloat>>(nPixels)
				)
			);

			// Create Staging Buffer
			ViraBuffer* spectralImageStagingBuffer = new ViraBuffer(viraDevice, sizeof(TFloat), 	
				static_cast<uint32_t>(nPixels*4), 
				vk::BufferUsageFlagBits::eTransferDst, 
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
				pathTracer->minStorageBufferOffsetAlignment);		

			// Copy all spectral sets of color-like output images to data vector
			for(size_t i=0; i<pathTracer->nSpectralSets; ++i){
				for (int j=0; j<6; ++j) {

					// Copy Spectral Set Image to Staging Buffer
					viraDevice->copyImageToBuffer(
						viraSwapchain->getSpectralImage(static_cast<uint32_t>(i)),  
						spectralImageStagingBuffer->getBuffer(), 
						pathTracer->useExtent.width, 
						pathTracer->useExtent.height, 
						1, 
						j
					);
		
					// Copy Staging Buffer to data vector
					spectralImageStagingBuffer->map(imageSize, 0);
					std::memcpy(spectralImages[i][j].data(), spectralImageStagingBuffer->getMappedMemory(), imageSize);
					spectralImageStagingBuffer->unmap();
		
				}
		
			}
		
			// == Copy scalar image layers to data vector ==

			// Copy float Image Layers to data vector
			imageSize = nPixels * sizeof(TFloat);
			layerCount = 6;
			std::vector<std::vector<TFloat>> scalarImageData(layerCount, std::vector<TFloat>(nPixels));		

			// Create Staging Buffer
			ViraBuffer* scalarStagingBuffer = new ViraBuffer(viraDevice, sizeof(TFloat), 	
				static_cast<uint32_t>(nPixels), 
				vk::BufferUsageFlagBits::eTransferDst, 
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
				pathTracer->minStorageBufferOffsetAlignment);		

			// Copy Scalar Image Layers to data vector
			for (int i=0; i<6; ++i) {

				viraDevice->copyImageToBuffer(
					viraSwapchain->getScalarImage(),  
					scalarStagingBuffer->getBuffer(), 
					pathTracer->useExtent.width, 
					pathTracer->useExtent.height, 
					1, i
				);

				scalarStagingBuffer->map(imageSize, 0);
				std::memcpy(scalarImageData[i].data(), scalarStagingBuffer->getMappedMemory(), imageSize);
				scalarStagingBuffer->unmap();

			}

			// == Write Images to Camera RenderPass ==
			//Resolution resolution = pathTracer->getCamera()->getResolution();  // TODO Consider removing
			pathTracer->getCamera()->updateImagesFromVulkanPT(spectralImages, scalarImageData);

			// Transition the framebuffer images back to their original layout
			for(int i = 0; i < static_cast<int>(pathTracer->nSpectralSets); ++i) {
				viraDevice->transitionImageLayout(
					viraSwapchain->getSpectralImage(i),
					vk::ImageLayout::eTransferSrcOptimal, 
					vk::ImageLayout::eGeneral, 
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eRayTracingShaderKHR,
					layerCount
				);				
			}
			viraDevice->transitionImageLayout(
				viraSwapchain->getScalarImage(),
				vk::ImageLayout::eTransferSrcOptimal, 
				vk::ImageLayout::eGeneral, 
				vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eRayTracingShaderKHR,
				layerCount
			);	

			// == Write Camera RenderPass Images to Files ==

			// Simulate Sensor to create Visual Image
			vira::Image<float> capturedImage = pathTracer->getCamera()->simulateSensor();

			// Write Images to File
			std::string outputName = "GPU_PATHTRACER_FRAME_" + vira::utils::padZeros<3>(i_frame) + ".png";
            vira::ImageInterface::write("vira_gpu_pathtracer_output/" + outputName, capturedImage);

			// Increment the Epoch
            et += step_size;

		}

	}	

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	CommandBufferInterface* ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::beginFrame() {

		// Check if frame is already active
		if (isFrameStarted) {
			throw std::runtime_error("Cannot call beginFrame while frame is in progress");
		}

		// Set frame status to active
		isFrameStarted = true;

		// Retrieve Current Frame Output Image
		vk::ResultValue<uint32_t> currentImageResult = viraSwapchain->waitForImage();
		vk::Result result = currentImageResult.result;
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
			throw std::runtime_error("Failed to acquire wait for image");
		}

		// Get Command Buffer for current frame 
		CommandBufferInterface& commandBuffer = getCurrentCommandBuffer();

		// Begin Command Recording
		vk::CommandBufferBeginInfo beginInfo{};
		try {
			commandBuffer.begin(beginInfo);
		} catch (const vk::SystemError& err) {
			std::cerr << "Vulkan error: " << err.what() << std::endl;
			throw std::runtime_error("Failed to begin recording command buffer due to Vulkan error");
		} catch (const std::exception& err) {
			std::cerr << "Error: " << err.what() << std::endl;
			throw; // Re-throw the exception after logging it
		}

		// Return active command buffer
		return &commandBuffer;
		
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::endFrame(CommandBufferInterface* commandBuffer) 
	{
		// Check if frame is active
		if (!isFrameStarted) {
			throw std::runtime_error("Cannot call endFrame while frame is not in progress");
		}

		// End Command Recording for active frame command buffer
		try {
			commandBuffer->end();  
		} catch (vk::SystemError& err) {
			std::cerr << "Failed to record command buffer: " << err.what() << std::endl;
		}

		// Add active command buffer to vector
		std::vector<CommandBufferInterface*> commandBuffers;
		commandBuffers.push_back(commandBuffer);

		// Submit command buffers for execution
		auto result = viraSwapchain->submitCommandBuffers(commandBuffers, 0);
        (void)result; // TODO Consider removing

		// Set Frame status to inactive
		isFrameStarted = false;

		// Reset current frame index
		currentFrameIndex = 0;        

	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::beginRenderPass(CommandBufferInterface* commandBuffer) 
	{

		// Check if frame is active
		if (!isFrameStarted) {
			throw std::runtime_error("Cannot call beginRenderPass while frame is not in progress");
		}

		// == Create RenderPassBeginInfo ==

		// Initialize Info
		vk::RenderPassBeginInfo renderPassInfo{};
		renderPassInfo.setRenderPass(viraSwapchain->getRenderPass());
		renderPassInfo.setFramebuffer(viraSwapchain->getFrameBuffer());
		renderPassInfo.renderArea.setOffset({ 0,0 });
		renderPassInfo.renderArea.setExtent(viraSwapchain->getSwapchainExtent());

		// Set clear values for color attachments
		std::array<vk::ClearValue, 9> clearValues{};

		// Albedo (default gray)
		clearValues[0].setColor(vk::ClearColorValue(std::array<float, 4>{ 0.5f, 0.5f, 0.5f, 1.0f })); 

		// Direct Radiance 
		clearValues[1].setColor(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f })); 

		// Indirect Radiance 
		clearValues[2].setColor(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f })); 

		// Received Power Color
		clearValues[3].setColor(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f })); 

		// Normals 
		clearValues[4].setColor(vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 1.0f, 1.0f })); 

		// Alpha
		clearValues[5].setColor(vk::ClearColorValue(std::array<float, 4>{ std::numeric_limits<float>::infinity(), 0.0f, 0.0f, 0.0f })); 

		// Instance ID (Clear to 0)
		clearValues[6].setColor(vk::ClearColorValue(std::array<uint32_t, 4>{ 0, 0, 0, 0 })); 

		// Mesh ID (Clear to 0)
		clearValues[7].setColor(vk::ClearColorValue(std::array<uint32_t, 4>{ 0, 0, 0, 0 })); 

		// Depth Attachment (Must be between 0.0 - 1.0)
		clearValues[8].setDepthStencil({ 1.0f, 0 }); 

		// Pass the correct number of clear values to renderPassInfo
		renderPassInfo.setClearValueCount(static_cast<uint32_t>(clearValues.size()));
		renderPassInfo.setClearValues(clearValues);

		// Begin Vulkan RenderPass
		commandBuffer->beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		// Set Viewport and Scissor for RenderPass (for onscreen rendering)
		vk::Viewport viewport{};
		viewport.setX(0.0f);
		viewport.setY(0.0f);
		viewport.setWidth(static_cast<float>(viraSwapchain->getSwapchainExtent().width));
		viewport.setHeight(static_cast<float>(viraSwapchain->getSwapchainExtent().height));
		viewport.setMinDepth(0.0f);
		viewport.setMaxDepth(1.0f);
		vk::Rect2D scissor{ {0,0}, viraSwapchain->getSwapchainExtent() };

		commandBuffer->setViewport(0, viewport);
		commandBuffer->setScissor(0, scissor);
	};

	template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	void ViraRenderLoop<TSpectral, TFloat, TMeshFloat>::endRenderPass(CommandBufferInterface* commandBuffer) 
	{

		// Check if frame is active
		if (!isFrameStarted) {
			throw std::runtime_error("Cannot call endRenderPass while frame is not in progress");
		}

		// Get current renderpass command buffer
		if (commandBuffer->get() != getCurrentCommandBuffer().get()) {
			throw std::runtime_error("Cannot end render pass on a command buffer from a different frame");
		}

		// End Vulkan RenderPass
		commandBuffer->endRenderPass();
	};


}