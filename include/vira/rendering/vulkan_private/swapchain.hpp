#ifndef VIRA_RENDERING_VULKAN_SWAPCHAIN_HPP
#define VIRA_RENDERING_VULKAN_SWAPCHAIN_HPP

#include <memory>
#include <string>
#include <vector>

#include "vulkan/vulkan.hpp"

// Vira private headers:
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"
#include "vira/rendering/vulkan/vulkan_path_tracer.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"

namespace vira::vulkan {
    /**
     * @class ViraSwapchain
     * @brief Manages the Vulkan swapchain and related rendering resources.
     *
     * This class is responsible for handling Vulkan swapchains, framebuffers, image attachments, 
     * and synchronization primitives necessary for rendering operations. It supports both 
     * rasterization and path-tracing workflows.
     *
     * ## Member Variables
     *
     * ### Public:
     *
     * #### Framebuffer:
     * - `vk::Framebuffer frameBuffer` : The framebuffer used for rendering.
     *
     * #### Image & Memory Attachments:
     * - `vk::Image receivedPowerImage` : Image storing received power data.
     * - `vk::Image backgroundPowerImage` : Image storing background power.
     * - `vk::Image albedoImage` : Image for albedo color data.
     * - `vk::Image directRadianceImage` : Image for direct radiance.
     * - `vk::Image indirectRadianceImage` : Image for indirect radiance.
     * - `vk::Image normalImage` : Image storing normal vectors.
     * - `vk::Image depthImage` : Depth buffer image.
     * - `vk::Image triangleSizeImage` : Image storing triangle size data.
     * - `vk::Image alphaImage` : Image storing alpha transparency values.
     * - `vk::Image meshIDImage` : Image storing mesh IDs for object identification.
     * - `vk::Image triangleIDImage` : Image storing triangle IDs.
     * - `vk::Image instanceIDImage` : Image storing instance IDs.
     *
     * - `std::unique_ptr<MemoryInterface> receivedPowerImageMemory` : Memory allocation for received power image.
     * - `std::unique_ptr<MemoryInterface> backgroundPowerImageMemory` : Memory allocation for background power image.
     * - `std::unique_ptr<MemoryInterface> albedoImageMemory` : Memory allocation for albedo image.
     * - `std::unique_ptr<MemoryInterface> directRadianceImageMemory` : Memory allocation for direct radiance image.
     * - `std::unique_ptr<MemoryInterface> indirectRadianceImageMemory` : Memory allocation for indirect radiance image.
     * - `std::unique_ptr<MemoryInterface> normalImageMemory` : Memory allocation for normal image.
     * - `std::unique_ptr<MemoryInterface> depthImageMemory` : Memory allocation for depth image.
     * - `std::unique_ptr<MemoryInterface> triangleSizeImageMemory` : Memory allocation for triangle size image.
     * - `std::unique_ptr<MemoryInterface> alphaImageMemory` : Memory allocation for alpha image.
     * - `std::unique_ptr<MemoryInterface> meshIDImageMemory` : Memory allocation for mesh ID image.
     * - `std::unique_ptr<MemoryInterface> triangleIDImageMemory` : Memory allocation for triangle ID image.
     * - `std::unique_ptr<MemoryInterface> instanceIDImageMemory` : Memory allocation for instance ID image.
     *
     * #### Image Views:
     * - `vk::ImageView receivedPowerImageView` : Image view for received power.
     * - `vk::ImageView backgroundPowerImageView` : Image view for background power.
     * - `vk::ImageView albedoImageView` : Image view for albedo.
     * - `vk::ImageView directRadianceImageView` : Image view for direct radiance.
     * - `vk::ImageView indirectRadianceImageView` : Image view for indirect radiance.
     * - `vk::ImageView normalImageView` : Image view for normals.
     * - `vk::ImageView depthImageView` : Image view for depth.
     * - `vk::ImageView triangleSizeImageView` : Image view for triangle size.
     * - `vk::ImageView alphaImageView` : Image view for alpha values.
     * - `vk::ImageView meshIDImageView` : Image view for mesh IDs.
     * - `vk::ImageView triangleIDImageView` : Image view for triangle IDs.
     * - `vk::ImageView instanceIDImageView` : Image view for instance IDs.
     *
     * #### Render Pass Attachments:
     * - `vk::AttachmentDescription receivedPowerAttachmentDescription` : Attachment description for received power.
     * - `vk::AttachmentDescription backgroundPowerAttachmentDescription` : Attachment description for background power.
     * - `vk::AttachmentDescription albedoAttachmentDescription` : Attachment description for albedo.
     * - `vk::AttachmentDescription directRadianceAttachmentDescription` : Attachment description for direct radiance.
     * - `vk::AttachmentDescription indirectRadianceAttachmentDescription` : Attachment description for indirect radiance.
     * - `vk::AttachmentDescription normalsAttachmentDescription` : Attachment description for normals.
     * - `vk::AttachmentDescription depthAttachmentDescription` : Attachment description for depth buffer.
     * - `vk::AttachmentDescription triangleSizeAttachmentDescription` : Attachment description for triangle size.
     * - `vk::AttachmentDescription alphaAttachmentDescription` : Attachment description for alpha.
     * - `vk::AttachmentDescription meshIDAttachmentDescription` : Attachment description for mesh IDs.
     * - `vk::AttachmentDescription triangleIDAttachmentDescription` : Attachment description for triangle IDs.
     * - `vk::AttachmentDescription instanceIDAttachmentDescription` : Attachment description for instance IDs.
     *
     * - `vk::AttachmentReference receivedPowerAttachmentReference` : Attachment reference for received power.
     * - `vk::AttachmentReference backgroundPowerAttachmentReference` : Attachment reference for background power.
     * - `vk::AttachmentReference albedoAttachmentReference` : Attachment reference for albedo.
     * - `vk::AttachmentReference directRadianceAttachmentReference` : Attachment reference for direct radiance.
     * - `vk::AttachmentReference indirectRadianceAttachmentReference` : Attachment reference for indirect radiance.
     * - `vk::AttachmentReference normalsAttachmentReference` : Attachment reference for normals.
     * - `vk::AttachmentReference depthAttachmentReference` : Attachment reference for depth.
     * - `vk::AttachmentReference triangleSizeAttachmentReference` : Attachment reference for triangle size.
     * - `vk::AttachmentReference alphaAttachmentReference` : Attachment reference for alpha.
     * - `vk::AttachmentReference meshIDAttachmentReference` : Attachment reference for mesh IDs.
     * - `vk::AttachmentReference triangleIDAttachmentReference` : Attachment reference for triangle IDs.
     * - `vk::AttachmentReference instanceIDAttachmentReference` : Attachment reference for instance IDs.
     *
     * #### Path Tracing Resources:
     * - `std::vector<vk::Image> spectralImages` : A vector of vk::Images each of type vec4<float>. each image can hold four bands of the spectral data type, with the following pathtracer output in individual layers: (1: albedo, 2: direct radiance, 3: indirect radiance, 4: received power, 5: background power, 6: normal). The last layer is a padded vec3 image (non-spectral) for the normal.
     * - `std::vector<std::unique_ptr<MemoryInterface>> spectralImagesMemory` : Memory allocations for spectral images.
     * - `std::vector<vk::ImageView> spectralImageViews` : Image views for spectral images.
     * - `vk::Image scalarImage` : Scalar image with the following path tracer output in individual layers: (1: depth, 2: alpha, 3: triangle size, 4: mesh ID, 5: triangle ID, 6: instance ID)
     * - `std::unique_ptr<MemoryInterface> scalarImageMemory` : Memory allocation for scalar image.
     * - `vk::ImageView scalarImageView` : Image view for scalar image.
     *
     * ### Protected:
     * - `ViraDevice* viraDevice` : Pointer to the Vulkan device.
     * - `vk::Extent2D windowExtent` : The dimensions of the window that the swapchain is rendering to.
     * - `std::unique_ptr<SwapchainInterface> swapchain` : Vulkan swapchain wrapper.
     * - `vk::Format swapchainImageFormat` : Format used for swapchain images.
     * - `vk::Format swapchainDepthFormat` : Format used for depth buffering.
     * - `vk::Extent2D swapchainExtent` : The extent (resolution) of the swapchain images.
     * - `vk::RenderPass renderPass` : The Vulkan render pass associated with this swapchain.
     *
     * #### Synchronization Objects:
     * - `std::vector<vk::Semaphore> imageAvailableSemaphores` : Semaphores signaling image availability.
     * - `std::vector<vk::Semaphore> renderFinishedSemaphores` : Semaphores signaling rendering completion.
     * - `std::vector<vk::Fence> inFlightFences` : Fences used to track frame synchronization.
     * - `std::vector<vk::Fence> imagesInFlight` : Fences tracking which images are being used.
     * - `size_t currentFrame` : The index of the current frame being rendered.
     * - `static size_t maxFramesInFlight` : The maximum number of frames that can be in flight at once.
     */
    class ViraSwapchain {
    public:
    
        /**
         * @brief Default constructor for ViraSwapchain.
         */
        ViraSwapchain();

        /**
         * @brief Constructs a ViraSwapchain with a Vulkan device.
         * @param viraDevice Pointer to the Vulkan device.
         */
        ViraSwapchain(ViraDevice* viraDevice);

        /**
         * @brief Constructs a ViraSwapchain with a device, window extent, and path-tracer flag.
         * @param viraDevice Pointer to the Vulkan device.
         * @param windowExtent Dimensions of the window for swapchain creation.
         * @param isPathTracer Flag indicating if the swapchain is used for path tracing.
         */        
        ViraSwapchain(ViraDevice* viraDevice, vk::Extent2D windowExtent, bool isPathTracer);
        
        /**
         * @brief Destructor that cleans up Vulkan resources.
         */
        ~ViraSwapchain();

        // Deleted copy constructor and assignment operator
        ViraSwapchain(const ViraSwapchain&) = delete;
        ViraSwapchain& operator=(const ViraSwapchain&) = delete;


        // Image Properties
        /**
         * @brief Gets the swapchain extent (dimensions).
         * @return The swapchain extent as a `vk::Extent2D` object.
         */        
        virtual vk::Extent2D getSwapchainExtent() { return swapchainExtent; }

        /**
         * @brief Gets the width of the swapchain.
         * @return The width in pixels.
         */
        uint32_t width() { return swapchainExtent.width; }

        /**
         * @brief Gets the height of the swapchain.
         * @return The height in pixels.
         */
        uint32_t height() { return swapchainExtent.height; }

        /**
         * @brief Computes the aspect ratio of the swapchain extent.
         * @return The aspect ratio as a floating-point value.
         */
        virtual float extentAspectRatio() {
            return static_cast<float>(swapchainExtent.width) / static_cast<float>(swapchainExtent.height);
        }

        /**
         * @brief Determines the appropriate aspect mask for an image format and usage.
         * @param format The image format.
         * @param usage The intended usage of the image.
         * @return The aspect mask as `vk::ImageAspectFlags`.
         */
         vk::ImageAspectFlags getAspectMask(vk::Format format, vk::ImageUsageFlags usage);

        // Control & Synchronization
        /**
         * @brief Waits for an available swapchain image.
         * @return A `vk::ResultValue<uint32_t>` containing the image index.
         */
        vk::ResultValue<uint32_t> waitForImage(); 

        /**
         * @brief Acquires the next available swapchain image.
         * @return A `vk::ResultValue<uint32_t>` containing the image index.
         */        
        virtual vk::ResultValue<uint32_t> acquireNextImage();

        /**
         * @brief Submits command buffers for rendering.
         * @param buffers A vector of command buffer interfaces to be submitted.
         * @param imageIndex Pointer to the index of the image being rendered to.
         * @return The Vulkan result of the submission.
         */
        vk::Result submitCommandBuffers(const std::vector<CommandBufferInterface*> buffers, uint32_t* imageIndex);

        /**
         * @brief Gets the Vulkan render pass associated with this swapchain.
         * @return The Vulkan render pass.
         */
        virtual vk::RenderPass getRenderPass() { return renderPass; }

        // Rasterizer Resources
        /**
         * @brief Gets the framebuffer associated with this swapchain.
         * @return The Vulkan framebuffer.
         */
        virtual vk::Framebuffer getFrameBuffer() { return frameBuffer; }

        /**
         * @brief Finds the appropriate depth format for the swapchain.
         * @return The selected depth format.
         */
        virtual vk::Format findDepthFormat();

        // PathTracer Resources

        /**
         * @brief Creates image resources for the path tracer.
         * @param nVec4ImagesPerSpectrum Number of `vec4` images per spectral buffer.
         */
        void createPathTracerImageResources(size_t nVec4ImagesPerSpectrum);

        /**
         * @brief Gets the scalar image used in the path tracer to store the scalar output images. 
         * @return The Vulkan scalar image.
         */
        vk::Image getScalarImage() {return scalarImage;}

        /**
         * @brief Gets the spectral image for the 4*index to 4*index+3 bands of the spectral data type.
         * @param index The index of the spectral image.
         * @return The Vulkan spectral image.
         */
        vk::Image getSpectralImage(uint32_t index) {return spectralImages[index];}

       /**
         * @brief Gets the scalar image view.
         * @return The Vulkan scalar image view.
         */
        vk::ImageView getScalarImageView() {return scalarImageView;}
        
        /**
         * @brief Gets the spectral image view at a given index.
         * @param index The index of the spectral image view.
         * @return The Vulkan spectral image view.
         */        
        vk::ImageView getSpectralImageView(uint32_t index) {return spectralImageViews[index];}


        // Member Variables
        
        /// Rasterizer Resources
        // Framebuffer
        vk::Framebuffer frameBuffer;

        // Image, ImageView, & Memory for Each Attachment
        vk::Image   receivedPowerImage, 
                    backgroundPowerImage,    
                    albedoImage,
                    directRadianceImage, 
                    indirectRadianceImage,
                    normalImage, 
                    alphaImage,
                    depthImage,
                    triangleSizeImage,
                    meshIDImage, 
                    triangleIDImage, 
                    instanceIDImage;


        std::unique_ptr<MemoryInterface>    receivedPowerImageMemory, 
                                            backgroundPowerImageMemory, 
                                            albedoImageMemory, 
                                            directRadianceImageMemory, 
                                            indirectRadianceImageMemory, 
                                            normalImageMemory, 
                                            alphaImageMemory,
                                            depthImageMemory,
                                            triangleSizeImageMemory,
                                            meshIDImageMemory, 
                                            triangleIDImageMemory, 
                                            instanceIDImageMemory;


        vk::ImageView   receivedPowerImageView, 
                        backgroundPowerImageView, 
                        albedoImageView, 
                        directRadianceImageView, 
                        indirectRadianceImageView, 
                        normalImageView, 
                        alphaImageView,
                        depthImageView,
                        triangleSizeImageView,                            
                        meshIDImageView, 
                        triangleIDImageView, 
                        instanceIDImageView;

        // Render Pass Attachments
        vk::AttachmentDescription   receivedPowerAttachmentDescription,    
                                    backgroundPowerAttachmentDescription,         
                                    albedoAttachmentDescription, 
                                    directRadianceAttachmentDescription, 
                                    indirectRadianceAttachmentDescription, 
                                    normalsAttachmentDescription,
                                    alphaAttachmentDescription,
                                    depthAttachmentDescription, 
                                    triangleSizeAttachmentDescription,
                                    meshIDAttachmentDescription, 
                                    triangleIDAttachmentDescription, 
                                    instanceIDAttachmentDescription;

        // Render Pass Attachment References
        vk::AttachmentReference     receivedPowerAttachmentReference, 
                                    backgroundPowerAttachmentReference,
                                    albedoAttachmentReference, 
                                    directRadianceAttachmentReference, 
                                    indirectRadianceAttachmentReference, 
                                    normalsAttachmentReference, 
                                    alphaAttachmentReference,
                                    depthAttachmentReference,
                                    triangleSizeAttachmentReference,
                                    meshIDAttachmentReference, 
                                    triangleIDAttachmentReference, 
                                    instanceIDAttachmentReference;

        // Pathtracer Resources
        std::vector<vk::Image> spectralImages;
        std::vector<std::unique_ptr<MemoryInterface>> spectralImagesMemory;
        std::vector<vk::ImageView> spectralImageViews;

        vk::Image scalarImage;
        std::unique_ptr<MemoryInterface> scalarImageMemory;
        vk::ImageView scalarImageView;

        /// Not In Use: On Screen Rendering
        ViraSwapchain(ViraDevice* viraDevice, vk::Extent2D windowExtent, std::shared_ptr<ViraSwapchain> calledPrevious);
        ViraSwapchain(ViraDevice* viraDevice, vk::Extent2D windowExtent);
        virtual vk::Framebuffer getOnScreenFramebuffer(int index) { return swapchainFramebuffers[index]; }
        vk::ImageView getImageView(int index) { return swapchainImageViews[index]; }
        virtual size_t imageCount() { return swapchainImages.size(); }
        virtual vk::Format getSwapchainImageFormat() { return swapchainImageFormat; }
        virtual vk::SwapchainKHR getSwapchainVK() {return swapchain->getVkSwapchain();}
        virtual vk::Result submitOnscreenCommandBuffers(const std::vector<CommandBufferInterface*> buffers, uint32_t* imageIndex);
        virtual bool compareSwapFormats(const ViraSwapchain& swapchain) const;
        
    protected:

        /**
         * @brief Creates the Vulkan render pass for the rasterizer pipeline.
         */
        void createRenderPass();

        /**
         * @brief Creates an individual framebuffer that can hold data of 4 bands of the spectral data type.
         */
        void createFrameBuffer();

        /**
         * @brief Creates the frame attachments for a rasterizer frame buffer.
         */
        void createFrameAttachments();

        /**
         * @brief Creates an image attachment.
         * @param format Image format.
         * @param usage Image usage flags.
         * @param finalLayout Final layout of the image.
         * @param attachmentIndex Index of the attachment.
         * @param image Vulkan image object.
         * @param imageMemory Memory interface for the image.
         * @param imageView Vulkan image view.
         * @param attachmentDescription Attachment description.
         * @param attachmentReference Attachment reference.
         */        
        void createAttachment( 
            vk::Format format,
            vk::ImageUsageFlags usage,
            vk::ImageLayout finalLayout,
            uint32_t attachmentIndex,
            vk::Image& image,
            std::unique_ptr<MemoryInterface>& imageMemory,
            vk::ImageView& imageView,
            vk::AttachmentDescription& attachmentDescription,
            vk::AttachmentReference& attachmentReference);

        /**
         * @brief Creates Vulkan synchronization objects.
         */
        void createSyncObjects(size_t maxFramesInFlight = 2);

        /**
         * @brief Initializes the rasterizer-specific swapchain configuration, creating the frame buffer and attachments, the render pass, and the sync objects.
         * @param extent The extent of the swapchain window.
         */
        void initRasterizer(vk::Extent2D extent, size_t maxFramesInFlight = 2);

        /**
         * @brief Initializes the path tracer-specific swapchain configuration and creates the sync objects.
         * @param extent The extent of the swapchain window.
         */        
        void initPathTracer(vk::Extent2D extent, size_t maxFramesInFlight = 2);

        // Member Variables

        ViraDevice* viraDevice;
        vk::Extent2D windowExtent;
        vk::Extent2D swapchainExtent;
        vk::RenderPass renderPass;

        std::vector<vk::Semaphore> imageAvailableSemaphores;
        std::vector<vk::Semaphore> renderFinishedSemaphores;
        std::vector<vk::Fence> inFlightFences;
        std::vector<vk::Fence> imagesInFlight;
        size_t currentFrame = 0;
        size_t maxFramesInFlight = 2;

        /// Not In Use: On Screen Rendering
        void createFramebuffers();
        void createSwapchain();
        void createImageViews();
        void createDepthResources();

        void createOnscreenRenderPass();
        void init();
        virtual vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        virtual vk::PresentModeKHR chooseSwapPresentMode(
            const std::vector<vk::PresentModeKHR>& availablePresentModes);
        virtual vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

        std::unique_ptr<SwapchainInterface> swapchain;
        vk::Format swapchainImageFormat;
        vk::Format swapchainDepthFormat;

        std::vector<vk::Framebuffer> swapchainFramebuffers;
        std::vector<vk::Image> depthImages;
        std::vector<std::unique_ptr<MemoryInterface>> depthImageMemorys;
        std::vector<vk::ImageView> depthImageViews;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::ImageView> swapchainImageViews;
        vk::Image offscreenImage;
        std::unique_ptr<MemoryInterface> offscreenImageMemory;
        vk::ImageView offscreenImageView;
        vk::Image offscreenDepthImage;
        std::unique_ptr<MemoryInterface> offscreenDepthImageMemory;
        vk::ImageView offscreenDepthImageView;
        vk::Framebuffer offscreenDepthFramebuffer;
        std::shared_ptr<ViraSwapchain> oldViraSwapchain;


    };

}

#include "implementation/rendering/vulkan_private/swapchain.ipp"

#endif