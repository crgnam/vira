#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

#include "vulkan/vulkan.hpp"

// Vira private headers:
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/swapchain/swapchain_interface.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"

namespace vira::vulkan {

    ViraSwapchain::ViraSwapchain() : viraDevice{nullptr} {}

    ViraSwapchain::ViraSwapchain(ViraDevice* viraDevice) : viraDevice{viraDevice} {}

    ViraSwapchain::ViraSwapchain(ViraDevice* viraDevice, vk::Extent2D extent, bool isPathTracer)
        : viraDevice{ viraDevice }, windowExtent{ extent } {
        if(isPathTracer==true) {
            initPathTracer(extent);
        } else {
            initRasterizer(extent);
        }     
    }

    ViraSwapchain::~ViraSwapchain() {

        if (viraDevice && viraDevice->device()) {
            // Destroy framebuffer
            if (frameBuffer) {
                viraDevice->device()->destroyFramebuffer(frameBuffer, nullptr);
                frameBuffer = nullptr;
            }

            // Destroy output image views
            const std::vector<vk::ImageView*> imageViews = {
                &receivedPowerImageView, &backgroundPowerImageView, &albedoImageView,
                &directRadianceImageView, &indirectRadianceImageView, &normalImageView,
                &alphaImageView, &depthImageView, &triangleSizeImageView,
                &meshIDImageView, &triangleIDImageView, &instanceIDImageView
            };

            for (auto viewPtr : imageViews) {
                if (*viewPtr) {
                    viraDevice->device()->destroyImageView(*viewPtr, nullptr);
                    *viewPtr = nullptr;
                }
            }

            // Destroy images
            const std::vector<vk::Image*> images = {
                &receivedPowerImage, &backgroundPowerImage, &albedoImage,
                &directRadianceImage, &indirectRadianceImage, &normalImage,
                &alphaImage, &depthImage, &triangleSizeImage,
                &meshIDImage, &triangleIDImage, &instanceIDImage
            };

            for (auto imagePtr : images) {
                if (*imagePtr) {
                    viraDevice->device()->destroyImage(*imagePtr, nullptr);
                    *imagePtr = nullptr;
                }
            }

            // Reset memory (std::unique_ptr will handle actual deletion)
            receivedPowerImageMemory.reset();
            backgroundPowerImageMemory.reset();
            albedoImageMemory.reset();
            directRadianceImageMemory.reset();
            indirectRadianceImageMemory.reset();
            normalImageMemory.reset();
            alphaImageMemory.reset();
            depthImageMemory.reset();
            triangleSizeImageMemory.reset();
            meshIDImageMemory.reset();
            triangleIDImageMemory.reset();
            instanceIDImageMemory.reset();
            
            for (auto image : spectralImages) {
                if (image) {
                    viraDevice->device()->destroyImage(image, nullptr);
                }
            }
            for (auto imageView : spectralImageViews) {
                if (imageView) {
                    viraDevice->device()->destroyImageView(imageView, nullptr);
                }
            }
            if (scalarImage) {
                viraDevice->device()->destroyImage(scalarImage, nullptr);
            }
            if (scalarImageView) {
                viraDevice->device()->destroyImageView(scalarImageView, nullptr);
            }

            if (renderPass) {
                viraDevice->device()->destroyRenderPass(renderPass, nullptr);
            }
            for (auto fence : inFlightFences) {
                if (fence) {
                    viraDevice->device()->destroyFence(fence, nullptr);
                }
            }
            for (auto fence : imagesInFlight) {
                if (fence) {
                    viraDevice->device()->destroyFence(fence, nullptr);
                }
            }
            for (auto semaphore : imageAvailableSemaphores) {
                if (semaphore) {
                    viraDevice->device()->destroySemaphore(semaphore, nullptr);
                }
            }
            for (auto semaphore : renderFinishedSemaphores) {
                if (semaphore) {
                    viraDevice->device()->destroySemaphore(semaphore, nullptr);
                }
            }
        }
    }

    void ViraSwapchain::initRasterizer(vk::Extent2D extent, size_t maxFramesInFlight) {

        (void)maxFramesInFlight; // TODO Consider removing

        swapchainExtent = extent;

        createFrameAttachments();
        createRenderPass();
        createFrameBuffer();
        createSyncObjects(maxFramesInFlight);

    }

    void ViraSwapchain::initPathTracer(vk::Extent2D extent, size_t maxFramesInFlight) {

        swapchainExtent = extent;

        createSyncObjects(maxFramesInFlight);
        
    }

    vk::ImageAspectFlags ViraSwapchain::getAspectMask(vk::Format format, vk::ImageUsageFlags usage) {
        if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {

            if (format == vk::Format::eD32Sfloat || format == vk::Format::eD16Unorm) {
                return vk::ImageAspectFlagBits::eDepth;  // Depth-only formats
            } 
            if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint) {
                return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;  // Depth + Stencil formats
            } 
            return vk::ImageAspectFlagBits::eStencil;  // Stencil-only format
        }

        return vk::ImageAspectFlagBits::eColor;  // Default for color attachments
    }

    void ViraSwapchain::createAttachment(
        vk::Format format,
        vk::ImageUsageFlags usage,
        vk::ImageLayout finalLayout,
        uint32_t attachmentIndex,
        vk::Image& image,
        std::unique_ptr<MemoryInterface>& imageMemory,
        vk::ImageView& imageView,
        vk::AttachmentDescription& attachmentDescription,
        vk::AttachmentReference& attachmentReference) {
        
        vk::ImageAspectFlags aspectMask = getAspectMask(format, usage);
        
        // Create Image
        vk::ImageCreateInfo imageInfo = vk::ImageCreateInfo{}
            .setImageType(vk::ImageType::e2D)
            .setFormat(format)
            .setExtent({ swapchainExtent.width, swapchainExtent.height, 1 })
            .setMipLevels(1)
            .setArrayLayers(1)
            .setSamples(vk::SampleCountFlagBits::e1) // No multisampling
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(usage)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setInitialLayout(vk::ImageLayout::eUndefined);

        // Use `createImageWithInfo` for Image + Memory Allocation
        vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        viraDevice->createImageWithInfo(imageInfo, properties, image, imageMemory);

        // Create Image View
        vk::ImageViewCreateInfo viewInfo = vk::ImageViewCreateInfo{}
            .setImage(image)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(format)
            .setSubresourceRange(
                vk::ImageSubresourceRange{}
                    .setAspectMask(aspectMask)
                    .setBaseMipLevel(0)
                    .setLevelCount(1)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)
            );

        imageView = viraDevice->device()->createImageView(viewInfo, nullptr);

        // Create Attachment Description
        bool isDepth = (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment).operator bool();

        attachmentDescription = vk::AttachmentDescription{}
            .setFormat(format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(isDepth ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(finalLayout);

        // Create Attachment Reference
        vk::ImageLayout layout = isDepth ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                        : vk::ImageLayout::eColorAttachmentOptimal;

        attachmentReference = vk::AttachmentReference{}
            .setAttachment(attachmentIndex)
            .setLayout(layout);
    }

    void ViraSwapchain::createPathTracerImageResources(size_t nSpectralSets) {

        spectralImages.resize(nSpectralSets);
        spectralImagesMemory.resize(nSpectralSets);
        spectralImageViews.resize(nSpectralSets);

        // Shared properties/info
        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent = vk::Extent3D{swapchainExtent.width, swapchainExtent.height, 1};
        imageInfo.samples = vk::SampleCountFlagBits::e1; // No multisampling
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.flags = {}; //vk::ImageCreateFlagBits::e2DArrayCompatible;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;

        vk::ImageViewCreateInfo viewInfo{};

        viewInfo.viewType = vk::ImageViewType::e2DArray;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 6;

        vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        vk::Format format;
        
        // Create Vec4 (and Spectral) Images
        for(size_t i=0; i < nSpectralSets; ++i){
            format = vk::Format::eR32G32B32A32Sfloat;
            imageInfo.format = format;

            viraDevice->createImageWithInfo(imageInfo, properties, spectralImages[i], spectralImagesMemory[i]);

            viewInfo.image = spectralImages[i];
            viewInfo.format = format; // Match the image format
            spectralImageViews[i] = viraDevice->device()->createImageView(viewInfo, nullptr);

            viraDevice->transitionImageLayout(
                spectralImages[i],                      // Image to transition
                vk::ImageLayout::eUndefined,            // Old layout
                vk::ImageLayout::eGeneral,              // New layout
                vk::PipelineStageFlagBits::eTransfer,   // Source pipeline stage
                vk::PipelineStageFlagBits::eRayTracingShaderKHR,   // Destination pipeline stage
                6
            );       
        }

        // Create scalar Image        
        format = vk::Format::eR32Sfloat;

        imageInfo.format = format;            
        viraDevice->createImageWithInfo(imageInfo, properties, scalarImage, scalarImageMemory);

        viewInfo.image = scalarImage;
        viewInfo.format = format; // Match the image format
        scalarImageView = viraDevice->device()->createImageView(viewInfo, nullptr);

        viraDevice->transitionImageLayout(
            scalarImage,                                 
            vk::ImageLayout::eUndefined,                       
            vk::ImageLayout::eGeneral,                       
            vk::PipelineStageFlagBits::eTransfer,             
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,    
            6
        );   
    }

    void ViraSwapchain::createFrameAttachments() {
      
        // Create Albedo Attachment
        createAttachment(
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            0,
            albedoImage,
            albedoImageMemory,
            albedoImageView,
            albedoAttachmentDescription,
            albedoAttachmentReference
        );

        // Create Direct Radiance Attachment
        createAttachment(
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            1,
            directRadianceImage,
            directRadianceImageMemory,
            directRadianceImageView,
            directRadianceAttachmentDescription,
            directRadianceAttachmentReference
        );

        // Create Indirect Radiance Attachment
        createAttachment(
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            2,
            indirectRadianceImage,
            indirectRadianceImageMemory,
            indirectRadianceImageView,
            indirectRadianceAttachmentDescription,
            indirectRadianceAttachmentReference
        );

        // Create Received Power Attachment
        createAttachment(
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            3,
            receivedPowerImage,
            receivedPowerImageMemory,
            receivedPowerImageView,
            receivedPowerAttachmentDescription,
            receivedPowerAttachmentReference
        );

        // Create Normals Attachment
        createAttachment(
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            4,
            normalImage,
            normalImageMemory,
            normalImageView,
            normalsAttachmentDescription,
            normalsAttachmentReference
        );

        // Create Alpha Attachment
        createAttachment(
            vk::Format::eR32Sfloat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            5,
            alphaImage,
            alphaImageMemory,
            alphaImageView,
            alphaAttachmentDescription,
            alphaAttachmentReference
        );        

        // Create Instance ID Attachment
        createAttachment(
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            6,
            instanceIDImage,
            instanceIDImageMemory,
            instanceIDImageView,
            instanceIDAttachmentDescription,
            instanceIDAttachmentReference
        );

        // Create Mesh ID Attachment
        createAttachment(
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eColorAttachmentOptimal,
            7,
            meshIDImage,
            meshIDImageMemory,
            meshIDImageView,
            meshIDAttachmentDescription,
            meshIDAttachmentReference
        );

        // TODO: Create second framebuffer to hold additional attachments
        // (FrameBuffers can hold a limited amount of attachments)

        // // Create Triangle ID Attachment
        // createAttachment(
        //     vk::Format::eR32Uint,
        //     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        //     vk::ImageLayout::eColorAttachmentOptimal,
        //     8,
        //     triangleIDImage,
        //     triangleIDImageMemory,
        //     triangleIDImageView,
        //     triangleIDAttachmentDescription,
        //     triangleIDAttachmentReference
        // );

        // // Create Triangle Size Attachment
        // createAttachment(
        //     vk::Format::eR32Sfloat,
        //     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
        //     vk::ImageLayout::eColorAttachmentOptimal,
        //     8,
        //     triangleSizeImage,
        //     triangleSizeImageMemory,
        //     triangleSizeImageView,
        //     triangleSizeAttachmentDescription,
        //     triangleSizeAttachmentReference
        // );

        // Create Depth Attachment
        vk::Format depthFormat = findDepthFormat();

        createAttachment(
            depthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            8,
            depthImage,
            depthImageMemory,
            depthImageView,
            depthAttachmentDescription,
            depthAttachmentReference
        );

    }

    void ViraSwapchain::createRenderPass() {

        std::array<vk::AttachmentDescription, 9> attachments = {
            albedoAttachmentDescription,
            directRadianceAttachmentDescription,
            indirectRadianceAttachmentDescription,
            receivedPowerAttachmentDescription,
            normalsAttachmentDescription,
            alphaAttachmentDescription,
            instanceIDAttachmentDescription,
            meshIDAttachmentDescription,
            depthAttachmentDescription
        };

        std::vector<vk::AttachmentReference> colorAttachmentRefs = {
            albedoAttachmentReference,
            directRadianceAttachmentReference,
            indirectRadianceAttachmentReference,
            receivedPowerAttachmentReference,
            normalsAttachmentReference,
            alphaAttachmentReference,
            instanceIDAttachmentReference,
            meshIDAttachmentReference
        };

        // TODO Consider removing
        //vk::AttachmentReference depthAttachmentRef = vk::AttachmentReference{}
        //    .setAttachment(9) // Index of depth attachment in 'attachments' array
        //    .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal); 
    
        // Define Subpass (Bind All Color Attachments)
        vk::SubpassDescription subpass = vk::SubpassDescription{}
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachmentRefs)
            .setPDepthStencilAttachment(&depthAttachmentReference);

        std::vector<vk::SubpassDescription> subpasses = { subpass };

        // Define Render Pass Dependencies
        vk::SubpassDependency dependency = vk::SubpassDependency{}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setSrcAccessMask({})
            .setDstSubpass(0)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        std::vector<vk::SubpassDependency> dependencies = { dependency };

        // Render Pass
        vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo{}
            .setAttachments(attachments)
            .setSubpasses(subpasses)
            .setDependencies(dependencies);

        try {
            renderPass = viraDevice->device()->createRenderPass(renderPassInfo, nullptr); 
        } catch (const vk::SystemError& err) {
            throw std::runtime_error("Swapchain: Failed to create render pass! " + std::string(err.what()));
        }

    };

    void ViraSwapchain::createFrameBuffer() {
       std::vector<vk::ImageView> framebufferAttachments = {
            albedoImageView,
            directRadianceImageView,
            indirectRadianceImageView,
            receivedPowerImageView,
            normalImageView,
            alphaImageView,
            instanceIDImageView,
            meshIDImageView,
            depthImageView
        };

        vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo{}
            .setRenderPass(renderPass)
            .setAttachments(framebufferAttachments)
            .setWidth(swapchainExtent.width)
            .setHeight(swapchainExtent.height)
            .setLayers(1);

        frameBuffer = viraDevice->device()->createFramebuffer(framebufferInfo, nullptr);
    }

    void ViraSwapchain::createSyncObjects(size_t maxFramesInFlight) {
        imageAvailableSemaphores.resize(maxFramesInFlight);
        renderFinishedSemaphores.resize(maxFramesInFlight);
        inFlightFences.resize(maxFramesInFlight);
        this->maxFramesInFlight = maxFramesInFlight;
        
        vk::SemaphoreCreateInfo semaphoreInfo = {};

        vk::FenceCreateInfo fenceInfo = {};
        fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (size_t i = 0; i < maxFramesInFlight; i++) {

            imageAvailableSemaphores[i] = viraDevice->device()->createSemaphore(semaphoreInfo, nullptr);
            renderFinishedSemaphores[i] = viraDevice->device()->createSemaphore(semaphoreInfo, nullptr);
            inFlightFences[i] = viraDevice->device()->createFence(fenceInfo, nullptr);

        }
    }

    vk::ResultValue<uint32_t> ViraSwapchain::waitForImage() {

        // TODO: Implement true synchronization here for onscreen rendering or
        // if simultaneous multiple frame rendering is implemented

        // // Wait for the GPU to finish using the off-screen image
        // viraDevice->device()->waitForFences(
        //     inFlightFences[0],
        //     VK_TRUE,
        //     std::numeric_limits<uint64_t>::max());
        
        // // Reset the fence for reuse
        // viraDevice->device()->resetFences(inFlightFences[0]);

        // Since there's only one image for offscreen rendering currently, always return index 0
        return vk::ResultValue<uint32_t>{vk::Result::eSuccess, 0};
    }

    vk::Result ViraSwapchain::submitCommandBuffers(
        const std::vector<CommandBufferInterface*> buffers, uint32_t* imageIndex) {
        (void)imageIndex; // TODO Consider removing

        std::vector<vk::CommandBuffer> vkCommandBuffers;
        vkCommandBuffers.reserve(buffers.size()); // Reserve memory for efficiency

        // Iterate over the unique_ptr references in the buffers vector
        for (const auto& buffer : buffers) {
            // Extract the vk::CommandBuffer from each CommandBufferInterface
            vkCommandBuffers.push_back(buffer->get());
        }

        viraDevice->device()->resetFences(inFlightFences[0]);

        vk::SubmitInfo submitInfo = {};
        submitInfo.setCommandBuffers(vkCommandBuffers);

        try {
            viraDevice->device()->resetFences(inFlightFences[currentFrame]);
            viraDevice->graphicsQueue()->submit({submitInfo}, inFlightFences[currentFrame]);
            viraDevice->device()->waitForFences(inFlightFences[0], VK_TRUE, UINT64_MAX);

        } catch (const vk::SystemError& err) {
           std::cerr << "failed to submit pathtracer command buffer! " << err.what() << std::endl;
        }

        vk::Result fenceStatus = viraDevice->device()->getDevice().getFenceStatus(inFlightFences[0], *viraDevice->dldi );
        (void)fenceStatus; // TODO Consider Removing

        return vk::Result::eSuccess;
    };

    vk::Format ViraSwapchain::findDepthFormat() {
        
        vk::Format supportedFormat = viraDevice->findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);


        // Get format properties to validate
        vk::FormatProperties props = viraDevice->getPhysicalDevice()->getFormatProperties(supportedFormat);

        // Debug Output
        // std::cout << "Selected Depth Format: " << vk::to_string(supportedFormat) << std::endl;
        // std::cout << "Optimal Tiling Features: " << vk::to_string(props.optimalTilingFeatures) << std::endl;
        // std::cout << "Linear Tiling Features: " << vk::to_string(props.linearTilingFeatures) << std::endl;

        if (!(props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
            throw std::runtime_error("Selected depth format does NOT support depth-stencil attachments!");
        }
        return supportedFormat;
    };


    // =======================
    // Not In Use: (On Screen Rendering)
    // =======================

    void ViraSwapchain::init()
    {
        createSwapchain();
        createImageViews();
        createOnscreenRenderPass();
        createDepthResources();
        createFramebuffers();
        createSyncObjects();
    };    

    void ViraSwapchain::createImageViews() {
      
        swapchainImageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            vk::ImageViewCreateInfo viewInfo = {};
            viewInfo.setImage(swapchainImages[i]);
            viewInfo.setViewType(vk::ImageViewType::e2D);
            viewInfo.setFormat(swapchainImageFormat);

            vk::ImageSubresourceRange subresourceRange = {};
            subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
            subresourceRange.setBaseMipLevel(0);
            subresourceRange.setLevelCount(1);
            subresourceRange.setBaseArrayLayer(0);
            subresourceRange.setLayerCount(1);
            viewInfo.setSubresourceRange(subresourceRange);

            swapchainImageViews[i] = viraDevice->device()->createImageView(viewInfo, nullptr);

        }
    };
        
    void ViraSwapchain::createFramebuffers() {
        swapchainFramebuffers.resize(imageCount());
        for (size_t i = 0; i < imageCount(); i++) {
            std::array<vk::ImageView, 2> attachments = { swapchainImageViews[i], depthImageViews[i] };

            vk::Extent2D swapchainExtent = getSwapchainExtent();
            vk::FramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.setRenderPass(renderPass);
            framebufferInfo.setAttachments(attachments);
            framebufferInfo.setWidth(swapchainExtent.width);
            framebufferInfo.setHeight(swapchainExtent.height);
            framebufferInfo.setLayers(1);

            try {
                swapchainFramebuffers[i] = viraDevice->device()->createFramebuffer(framebufferInfo, nullptr);
            }
            catch(const std::runtime_error& e) {
                (void)e; // TODO Consider removing
                throw(std::runtime_error("failed to create framebuffer!"));
            }

        }
    };


    void ViraSwapchain::createSwapchain() {
        SwapchainSupportDetails swapchainSupport = viraDevice->getSwapchainSupport();

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
        vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

        uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapchainSupport.capabilities.maxImageCount) {
            imageCount = swapchainSupport.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo = {};
        createInfo.setSurface(viraDevice->surface());

        createInfo.setMinImageCount(imageCount);
        createInfo.setImageFormat(surfaceFormat.format);
        createInfo.setImageColorSpace(surfaceFormat.colorSpace);
        createInfo.setImageExtent(extent);
        createInfo.setImageArrayLayers(1);
        createInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

        QueueFamilyIndices indices = viraDevice->findPhysicalQueueFamilies();
        std::vector<uint32_t> queueFamilyIndices = { indices.graphicsFamily, indices.presentFamily };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
            createInfo.setQueueFamilyIndices(queueFamilyIndices);
        }
        else {
            createInfo.setImageSharingMode(vk::SharingMode::eExclusive);
            createInfo.setQueueFamilyIndices(nullptr);  // Optional
        }

        createInfo.setPreTransform(swapchainSupport.capabilities.currentTransform);
        createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

        createInfo.setPresentMode(presentMode);
        createInfo.setClipped(VK_TRUE);

        if (oldViraSwapchain == nullptr) {
            createInfo.setOldSwapchain(nullptr);
        }
        else {
            createInfo.setOldSwapchain(oldViraSwapchain->getSwapchainVK());
        }

        try {
            std::unique_ptr<SwapchainInterface> swapchain = viraDevice->device()->createSwapchainKHR(createInfo, nullptr); 
        } 
        catch(const std::runtime_error& e) {
            (void)e; // TODO Consider removing
            throw std::runtime_error("failed to create swap chain!");
        }
       
        std::vector<vk::Image> swapchainImages = viraDevice->device()->getSwapchainImagesKHR(swapchain);
        
        swapchainImageFormat = surfaceFormat.format;
        swapchainExtent = extent;
    };
    
    void ViraSwapchain::createDepthResources() {
        vk::Format depthFormat = findDepthFormat();
        swapchainDepthFormat = depthFormat;
        vk::Extent2D swapchainExtent = getSwapchainExtent();
        //vk::Result result; // TODO Consider removing

        depthImages.resize(imageCount());
        depthImageMemorys.resize(imageCount());
        depthImageViews.resize(imageCount());

        for (int i = 0; i < static_cast<int>(depthImages.size()); i++) {
            vk::ImageCreateInfo imageInfo = {};
            imageInfo.setImageType(vk::ImageType::e2D);
            imageInfo.setExtent({swapchainExtent.width, swapchainExtent.height, 1});
            imageInfo.setMipLevels(1);
            imageInfo.setArrayLayers(1);
            imageInfo.setFormat(depthFormat);
            imageInfo.setTiling(vk::ImageTiling::eOptimal);
            imageInfo.setInitialLayout(vk::ImageLayout::eUndefined);
            imageInfo.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
            imageInfo.setSamples(vk::SampleCountFlagBits::e1);
            imageInfo.setSharingMode(vk::SharingMode::eExclusive);
            imageInfo.setFlags({});

            viraDevice->createImageWithInfo(
                imageInfo,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                depthImages[i],
                depthImageMemorys[i]);

            vk::ImageViewCreateInfo viewInfo = {};
            vk::ImageSubresourceRange subresourceRange = {};


            viewInfo.setImage(depthImages[i]);
            viewInfo.setViewType(vk::ImageViewType::e2D);
            viewInfo.setFormat(depthFormat);
            
            subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
            subresourceRange.setBaseMipLevel(0);
            subresourceRange.setLevelCount(1);
            subresourceRange.setBaseArrayLayer(0);
            subresourceRange.setLayerCount(1);
            viewInfo.setSubresourceRange(subresourceRange);

            depthImageViews[i] = viraDevice->device()->createImageView(viewInfo, nullptr);

        }
    };

    vk::ResultValue<uint32_t> ViraSwapchain::acquireNextImage() {
        
        viraDevice->device()->waitForFences(
            inFlightFences[currentFrame],
            VK_TRUE,
            std::numeric_limits<uint64_t>::max());

        
        vk::ResultValue<uint32_t> resultValue = viraDevice->device()->acquireNextImageKHR(
            swapchain,
            std::numeric_limits<uint64_t>::max(),
            imageAvailableSemaphores[currentFrame], 
            nullptr);            
    
        return resultValue;
    };

    vk::Result ViraSwapchain::submitOnscreenCommandBuffers(
        const std::vector<CommandBufferInterface*> buffers, uint32_t* imageIndex) {

        std::vector<vk::CommandBuffer> vkCommandBuffers;
        vkCommandBuffers.reserve(buffers.size()); // Reserve memory for efficiency

        // Iterate over the unique_ptr references in the buffers vector
        for (const auto& buffer : buffers) {
            // Extract the vk::CommandBuffer from each CommandBufferInterface
            vkCommandBuffers.push_back(buffer->get());
        }

        vk::Result result;

        if (!imagesInFlight[*imageIndex]) {
            result = viraDevice->device()->waitForFences(imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

        vk::SubmitInfo submitInfo = {};

        std::vector<vk::Semaphore> waitSemaphores = { imageAvailableSemaphores[currentFrame] };
        std::vector<vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        std::vector<vk::Semaphore> signalSemaphores = { renderFinishedSemaphores[currentFrame] };

        submitInfo.setCommandBuffers(vkCommandBuffers);
        submitInfo.setWaitSemaphores(waitSemaphores);
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setSignalSemaphores(signalSemaphores);

        try {
            viraDevice->device()->resetFences(inFlightFences[currentFrame]);
            viraDevice->graphicsQueue()->submit({submitInfo}, inFlightFences[currentFrame]);
        } catch (const vk::SystemError& err) {
           std::cerr << "failed to submit draw command buffer! " << err.what() << std::endl;
        }
        
        QueueInterface* presentQueue = viraDevice->presentQueue();

        vk::PresentInfoKHR presentInfo = {};
        
        std::vector<vk::SwapchainKHR> swapchains = { getSwapchainVK() };
        presentInfo.setSwapchains(swapchains);
        presentInfo.setWaitSemaphores(signalSemaphores);

        std::vector<uint32_t> imageIndices = {*imageIndex};

        presentInfo.setImageIndices(imageIndices);

        result = presentQueue->presentKHR(presentInfo);

        currentFrame = (currentFrame + 1) % maxFramesInFlight;

        return result;
    };

    bool ViraSwapchain::compareSwapFormats(const ViraSwapchain& swapchain) const {
        return swapchain.swapchainDepthFormat == swapchainDepthFormat &&
                swapchain.swapchainImageFormat == swapchainImageFormat;
    };

    vk::SurfaceFormatKHR ViraSwapchain::chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR ViraSwapchain::chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes) {

        // Immediate present mode.  GPU sits idle when swap chain is waiting to be displayed
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
                std::cout << "Present mode: Immediate" << std::endl;
                return availablePresentMode;
            }
        }

        std::cout << "Present mode: V-Sync" << std::endl;
        return vk::PresentModeKHR::eFifo;;
    };
    
    vk::Extent2D ViraSwapchain::chooseSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            vk::Extent2D actualExtent = windowExtent;
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    };

    void ViraSwapchain::createOnscreenRenderPass() {

        // Color Attachment
        vk::AttachmentDescription colorAttachment = vk::AttachmentDescription{}
            .setFormat(getSwapchainImageFormat())
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
                    
        vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference{}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        
        // Albedo Attachment
        vk::AttachmentDescription albedoAttachment = vk::AttachmentDescription{}
            .setFormat(vk::Format::eR16G16B16A16Sfloat) // High-precision format
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

        vk::AttachmentReference albedoAttachmentRef = vk::AttachmentReference{}
            .setAttachment(1)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        // Depth Attachment
        vk::AttachmentDescription depthAttachment = vk::AttachmentDescription{}
            .setFormat(findDepthFormat())
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        vk::AttachmentReference depthAttachmentRef = vk::AttachmentReference{}
            .setAttachment(2)
            .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        // Subpass
        std::vector<vk::AttachmentReference> colorAttachmentRefs = {
            colorAttachmentRef,
            albedoAttachmentRef
        };

        vk::SubpassDescription subpass = vk::SubpassDescription{}
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorAttachmentRefs)
            .setPDepthStencilAttachment(&depthAttachmentRef);

        vk::SubpassDependency dependency = vk::SubpassDependency{}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcAccessMask(vk::AccessFlags())
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstSubpass(0)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

        std::vector<vk::SubpassDescription> subpasses = { subpass };
        std::vector<vk::SubpassDependency> dependencies = { dependency };
        std::array<vk::AttachmentDescription, 3> attachments = {
            colorAttachment,
            albedoAttachment,
            depthAttachment
        };


        // Render Pass
        vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo{}
            .setAttachments(attachments)
            .setSubpasses(subpasses)
            .setDependencies(dependencies);

        try {
            renderPass = viraDevice->device()->createRenderPass(renderPassInfo, nullptr); 
        } catch (const vk::SystemError& err) {
            throw std::runtime_error("Swapchain: Failed to create render pass! " + std::string(err.what()));
        }

    };

    ViraSwapchain::ViraSwapchain(ViraDevice* viraDevice, vk::Extent2D extent, std::shared_ptr<ViraSwapchain> oldViraSwapchain)
        : viraDevice{ viraDevice }, windowExtent{ extent }, oldViraSwapchain{ oldViraSwapchain }
    {
        init();

        oldViraSwapchain = nullptr;
    };

    ViraSwapchain::ViraSwapchain(ViraDevice* viraDevice, vk::Extent2D extent)
        : viraDevice{ viraDevice }, windowExtent{ extent } {
        initRasterizer(extent);
    }

    
}