#include "vulkan_testing.hpp"

using namespace vira::vulkan;

class TestableCommandBuffer : public CommandBufferInterface {
    public:
        TestableCommandBuffer(vk::CommandBuffer buffer) : buffer_(buffer) {}
        vk::CommandBuffer get() const override { return buffer_; }
    
    private:
        vk::CommandBuffer buffer_;
};

class SwapchainTest : public ::testing::Test {
    protected:
        void SetUp() override {
    
        // Create Vira Instance and Vira Device
        std::shared_ptr<InstanceFactoryInterface> instanceFactory = std::make_shared<VulkanInstanceFactory>();
        std::shared_ptr<DeviceFactoryInterface> deviceFactory = std::make_shared<VulkanDeviceFactory>();

        // Create Fully Initialized ViraDevice
        viraDevice = std::make_unique<TestableViraDevice>(instanceFactory, deviceFactory);

        }
        void TearDown() override {

            if (viraDevice) {
                viraDevice.reset();  
            }
        }
        std::unique_ptr<TestableViraDevice> viraDevice; 

};
        
TEST_F(SwapchainTest, submitCommandBuffers) {

    // Allocate a command buffer
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.setCommandPool(viraDevice->getCommandPool().getVkCommandPool())
             .setLevel(vk::CommandBufferLevel::ePrimary)
             .setCommandBufferCount(1);

    std::vector<CommandBufferInterface*> commandBuffers =
        viraDevice->device()->allocateCommandBuffers(allocInfo);

    ASSERT_EQ(commandBuffers.size(), 1);
    ASSERT_NE(commandBuffers[0], nullptr);

    // Begin and end the command buffer to avoid validation issues
    vk::CommandBufferBeginInfo beginInfo{};
    EXPECT_NO_THROW({ commandBuffers[0]->begin(beginInfo); });
    EXPECT_NO_THROW({ commandBuffers[0]->end(); });

    // Create and assign a real fence
    vk::FenceCreateInfo fenceInfo{};
    vk::Fence fence = viraDevice->device()->createFence(fenceInfo, nullptr);

    // Create a swapchain instance and set minimal required fields
    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());
    
    viraSwapchain->inFlightFences = { fence };
    viraSwapchain->currentFrame = 0;

    uint32_t imageIndex = 0;

    // Run the method under test
    EXPECT_NO_THROW({
        vk::Result result = viraSwapchain->submitCommandBuffers(commandBuffers, &imageIndex);
        EXPECT_EQ(result, vk::Result::eSuccess);
    });

    // Cleanup
    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, getAspectMask) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    EXPECT_EQ(viraSwapchain->getAspectMask(vk::Format::eD32Sfloat, vk::ImageUsageFlagBits::eDepthStencilAttachment),
              vk::ImageAspectFlagBits::eDepth);

    EXPECT_EQ(viraSwapchain->getAspectMask(vk::Format::eD32SfloatS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment),
              vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil);

    EXPECT_EQ(viraSwapchain->getAspectMask(vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment),
              vk::ImageAspectFlagBits::eColor);

    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, createAttachment) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    viraSwapchain->swapchainExtent = vk::Extent2D( 128, 128 ); 

    vk::Image image;
    std::unique_ptr<MemoryInterface> memory;
    vk::ImageView imageView;
    vk::AttachmentDescription description;
    vk::AttachmentReference reference;

    viraSwapchain->createAttachment(
        vk::Format::eR32G32B32A32Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::ImageLayout::eColorAttachmentOptimal,
        0,
        image, memory, imageView, description, reference);

    EXPECT_NE(image, vk::Image{});
    EXPECT_NE(imageView, vk::ImageView{});
    EXPECT_EQ(description.format, vk::Format::eR32G32B32A32Sfloat);
    EXPECT_EQ(reference.layout, vk::ImageLayout::eColorAttachmentOptimal);

    viraDevice->device()->destroyImageView(imageView, nullptr);
    viraDevice->device()->destroyImage(image, nullptr);
    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, createPathTracerImageResources) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    viraSwapchain->swapchainExtent = vk::Extent2D( 64, 64); 
    viraSwapchain->createPathTracerImageResources(3);

    EXPECT_EQ(viraSwapchain->spectralImages.size(), 3);
    EXPECT_EQ(viraSwapchain->spectralImageViews.size(), 3);
    EXPECT_TRUE(viraSwapchain->scalarImageView);  // Just checks it was created
    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, createFrameAttachments) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    viraSwapchain->swapchainExtent = vk::Extent2D(128, 128);

    EXPECT_NO_THROW({ viraSwapchain->createFrameAttachments(); });

    EXPECT_NE(viraSwapchain->albedoImageView, vk::ImageView{});
    EXPECT_NE(viraSwapchain->depthImageView, vk::ImageView{});
    EXPECT_EQ(viraSwapchain->albedoAttachmentDescription.format, vk::Format::eR32G32B32A32Sfloat);
    EXPECT_EQ(viraSwapchain->depthAttachmentReference.layout, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, createRenderPass) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    viraSwapchain->swapchainExtent = vk::Extent2D(128, 128);
    viraSwapchain->createFrameAttachments();

    EXPECT_NO_THROW({ viraSwapchain->createRenderPass(); });

    EXPECT_NE(viraSwapchain->renderPass, vk::RenderPass{});
    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, createFrameBuffer) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    viraSwapchain->swapchainExtent = vk::Extent2D(128, 128);
    viraSwapchain->createFrameAttachments();
    viraSwapchain->createRenderPass();

    EXPECT_NO_THROW({ viraSwapchain->createFrameBuffer(); });

    EXPECT_NE(viraSwapchain->frameBuffer, vk::Framebuffer{});
    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, createSyncObjects) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    EXPECT_NO_THROW({ viraSwapchain->createSyncObjects(2); });

    EXPECT_EQ(viraSwapchain->imageAvailableSemaphores.size(), 2);
    EXPECT_EQ(viraSwapchain->renderFinishedSemaphores.size(), 2);
    EXPECT_EQ(viraSwapchain->inFlightFences.size(), 2);

    viraSwapchain.reset();   

}

TEST_F(SwapchainTest, waitForImage) {

    std::unique_ptr<TestableViraSwapchain> viraSwapchain = std::make_unique<TestableViraSwapchain>(viraDevice.get());

    auto result = viraSwapchain->waitForImage();

    EXPECT_EQ(result.result, vk::Result::eSuccess);
    EXPECT_EQ(result.value, 0u);

    viraSwapchain.reset();   

}