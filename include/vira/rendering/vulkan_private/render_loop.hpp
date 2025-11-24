#ifndef VIRA_RENDERING_VULKAN_RENDERER_HPP
#define VIRA_RENDERING_VULKAN_RENDERER_HPP

#include <memory>
#include <vector>
#include <stdexcept>

#include "vulkan/vulkan.hpp"

#include "vira/rendering/vulkan_private/window.hpp"
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/swapchain.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan/vulkan_path_tracer.hpp"
#include "vira/rendering/vulkan/vulkan_rasterizer.hpp"

namespace vira::vulkan {
	
	// Forward Declarations
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class VulkanPathTracer;
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class VulkanRasterizer;

    /**
     * @brief Run-specific options for the render loop
     * 
     * @details This struct carries any user defined or data defined options defining the specifics of the render loop
     * Members:
     * - et: ET of the first frame.
     * - step_size: Time step between frames (seconds).
     * - numFrames: Number of frames to render.
	 * 
     * @see VulkanRenderLoop
    */
   struct RenderLoopOptions {

		double et;
		double step_size = 60; // seconds
		size_t numFrames = 1;

	};      

    
    /**
     * @class VulkanRenderLoop
     * @brief Manages and executes Vulkan render commands and any post-processing.
     *
     * @details The VulkanRenderLoop class manages the top level rendering loop, including the command buffers used
	 * to issue the Vulkan rendering commands. The render loop manages both rasterizer and path-tracer vulkan rendering.
     *
     * @tparam TFloat The float type used by Vira, which obeys the "IsFloat" concept.
     * @tparam TSpectral The spectral type of light used in the render, such as the material albedos, the light irradiance, etc.
     * This parameter obeys the "IsSpectral" concept.
     * @tparam TMeshFloat The float type used in the meshes.
     * 
     * @param[in] viraDevice (ViraDevice) The ViraDevice manages the physical and logical Vulkan devices that manage all Vulkan functionality.
     * @param[in] pathTracer (VulkanPathTracer) The pathtracer renderer.
     * @param[in] rasterizer (VulkanRasterizer) The rasterizer renderer.
     * 
	 * Member Variables:
	 * 
	 * - ViraWindow* viraWindow: Pointer to the window used for presenting rendered images.
	 * - ViraDevice* viraDevice: Pointer to the Vulkan device used for rendering operations and resource management.
	 * - VulkanPathTracer<TSpectral, TFloat, TMeshFloat>* pathTracer: Pointer to the path tracer responsible for ray tracing-based rendering.
	 * - VulkanRasterizer<TSpectral, TFloat, TMeshFloat>* rasterizer: Pointer to the rasterizer responsible for raster-based rendering.
	 * - RenderLoopOptions options: Configuration options that control how the render loop behaves.
	 * - size_t nPixels: Total number of pixels in the current rendering surface.
	 * - std::vector<CommandBufferInterface*> commandBuffers: Command buffers used for recording rendering render commands each frame.
	 * - std::unique_ptr<ViraSwapchain> viraSwapchain: Unique pointer to the swapchain handling image presentation to the screen.
	 * - uint32_t currentImageIndex: Index of the current swapchain image being rendered to.
	 * - int currentFrameIndex: Index of the current frame within the frame loop.
	 * - bool isFrameStarted: Flag indicating whether a frame has been started (i.e., command buffer recording is active).
     * - maxFramesInFlight: The maximum number of frames that can be in flight at once.
	 *
     * @note The pathtracer and rasterizer render loops do differ in structure slightly, as the two renderers have some small differences
	 * in their responsibilities.
     *
     * @see VulkanPathTracer, VulkanRasterizer, VulkanSwapchain
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
	class ViraRenderLoop {

	public:

		// Constructors / Destructor

		/**
		 * @brief Constructor for a PathTracer Render Loop
		 */
		ViraRenderLoop(ViraDevice* device, VulkanPathTracer<TSpectral, TFloat, TMeshFloat>* pathTracer, RenderLoopOptions options);

		/**
		 * @brief Constructor for a Rasterizer Render Loop
		 */		
		ViraRenderLoop(ViraDevice* device, VulkanRasterizer<TSpectral, TFloat, TMeshFloat>* rasterizer, RenderLoopOptions options);

		~ViraRenderLoop();

		// Delete Copy Methods
		ViraRenderLoop(const ViraRenderLoop&) = delete;
		ViraRenderLoop& operator=(const ViraRenderLoop&) = delete;
    	
		/**
		 * @brief Runs the rasterizer render loop, updating the Vira scene
		 * before each new frame. 
		 */
		void runRasterizerRenderLoop();

		/**
		 * @brief Runs the pathtracer render loop, updating the Vira scene
		 * before each new frame. 
		 */
		void runPathTracerRenderLoop();

		/**
		 * @brief Specify the ViraSwapchain to use for the render loop
		 */
		void setViraSwapchain(std::unique_ptr<ViraSwapchain> swapchain);

		/**
		 * @brief Begins new rendering frame. Checks output image synchronization and begins
		 * command writing in current command buffer.
		 */
		CommandBufferInterface* beginFrame();

		/**
		 * @brief Completes current frame. Ends command buffer writing and
		 * submits command buffer via swapchain.
		 */
		void endFrame(CommandBufferInterface* commandBuffer);

		/**
		 * @brief Configures and begins vulkan renderpass
		 */
		void beginRenderPass(CommandBufferInterface* commandBuffer);

		/**
		 * @brief Ends vulkan renderpass
		 */
		void endRenderPass(CommandBufferInterface* commandBuffer);

		/**
		 * @brief Retrieve the current frame index
		 */
		int   getFrameIndex() const;


	protected:
		/**
		 * @brief Initializes render loop, creating any remaining
		 * required resources like command buffers.
		 */
		void initialize();

		/**
		 * @brief Creates vulkan command buffers used for rendering commands.
		 */
		virtual void createCommandBuffers();

		/**
		 * @brief Frees vulkan command buffers
		 */
		void freeCommandBuffers();

		/**
		 * @brief Retrieves currently used command buffer
		 */
		virtual CommandBufferInterface& getCurrentCommandBuffer() const;

		ViraWindow* viraWindow;
		ViraDevice* viraDevice;

		VulkanPathTracer<TSpectral, TFloat, TMeshFloat>* pathTracer;
		VulkanRasterizer<TSpectral, TFloat, TMeshFloat>* rasterizer;

		RenderLoopOptions options;
		
		size_t nPixels;
		size_t maxFramesInFlight;

		std::vector<CommandBufferInterface*> commandBuffers;
		std::unique_ptr<ViraSwapchain> viraSwapchain;

		uint32_t currentImageIndex{ 0 };
		int currentFrameIndex{ 0 };
		bool isFrameStarted{ false };

	};

};

#include "implementation/rendering/vulkan_private/render_loop.ipp"

#endif