#include "vulkan_testing.hpp"

using namespace vira::vulkan;

class PipelineTest : public ::testing::Test {
    protected:
        void SetUp() override {
    
        // Create Vira Instance and Vira Device
        std::shared_ptr<InstanceFactoryInterface> instanceFactory = std::make_shared<VulkanInstanceFactory>();
        std::shared_ptr<DeviceFactoryInterface> deviceFactory = std::make_shared<VulkanDeviceFactory>();

        // Create Fully Initialized ViraDevice
        viraDevice = std::make_unique<TestableViraDevice>(instanceFactory, deviceFactory);
        viraPipeline = std::make_unique<TestableViraPipeline>(viraDevice.get());

        }
        void TearDown() override {

            if (viraPipeline) {
                viraPipeline.reset();
            }
            if (viraDevice) {
                viraDevice.reset();     // cleanup ViraDevice wrapper
            }
        }
        std::unique_ptr<TestableViraDevice> viraDevice; 
        std::unique_ptr<TestableViraPipeline> viraPipeline;
};


TEST_F(PipelineTest, createRasterizationPipelineAndBind) {

    const std::string& vertShader = "./shaders/testVert.spv";
    const std::string& fragShader = "./shaders/testFrag.spv";
    
    vk::PipelineLayoutCreateInfo layoutInfo{};
    auto pipelineLayout = viraDevice->device()->createPipelineLayout(layoutInfo, nullptr);

    vk::AttachmentDescription colorAttachment{};
    colorAttachment.setFormat(vk::Format::eR8G8B8A8Unorm);
    colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
    colorAttachment.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    std::vector<vk::AttachmentDescription> attachments(8, colorAttachment);
    std::vector<vk::AttachmentReference> colorAttachmentRefs;
    for (uint32_t i = 0; i < 8; ++i) {
        colorAttachmentRefs.push_back(
            vk::AttachmentReference{}
                .setAttachment(i)
                .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
        );
    }
    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(colorAttachmentRefs);
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.setAttachments(attachments);
    renderPassInfo.setSubpasses(subpass);
    
    vk::RenderPass renderPass = viraDevice->device()->createRenderPass(renderPassInfo, nullptr);
    
    // Fill in pipeline config
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };    
    PipelineConfigInfo config{};
    config.pipelineLayout = pipelineLayout;
    config.renderPass = renderPass;
    config.subpass = 0;
    config.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
    config.inputAssemblyInfo.setPrimitiveRestartEnable(VK_FALSE);
    config.viewportInfo.setViewportCount(1);
    config.viewportInfo.setScissorCount(1);
    config.rasterizationInfo.setPolygonMode(vk::PolygonMode::eFill);
    config.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
    config.rasterizationInfo.setFrontFace(vk::FrontFace::eClockwise);
    config.rasterizationInfo.setLineWidth(1.0f);
    config.multisampleInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    config.depthStencilInfo.setDepthTestEnable(VK_TRUE);
    config.depthStencilInfo.setDepthWriteEnable(VK_TRUE);
    config.depthStencilInfo.setDepthCompareOp(vk::CompareOp::eLessOrEqual);
    config.dynamicStateInfo.setDynamicStates(dynamicStates);
    config.dynamicStateInfo.setDynamicStateCount(2);
   
    
    EXPECT_NO_THROW({
        viraPipeline->createRasterizationPipeline(vertShader, fragShader, &config);
    });

    CommandBufferInterface* commandBuffer = viraDevice->beginSingleTimeCommands();
    EXPECT_NO_THROW({
        viraPipeline->bind(commandBuffer);
    });


    viraDevice->device()->destroyRenderPass(renderPass, nullptr);
    viraDevice->device()->destroyPipelineLayout(pipelineLayout, nullptr);

}
