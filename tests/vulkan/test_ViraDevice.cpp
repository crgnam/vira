#include "vulkan_testing.hpp"

using namespace vira::vulkan;

class ViraDeviceTest : public ::testing::Test {
    protected:
        void SetUp() override {
        
            // Create Vira Instance and Vira Device
            std::shared_ptr<InstanceFactoryInterface> instanceFactory = std::make_shared<VulkanInstanceFactory>();
            std::shared_ptr<DeviceFactoryInterface> deviceFactory = std::make_shared<VulkanDeviceFactory>();

            // Create Fully Initialized ViraDevice
            viraDevice = std::make_unique<TestableViraDevice>();
            
            // Attach factories
            viraDevice->instanceFactory_ = instanceFactory;
            viraDevice->deviceFactory_ = deviceFactory;

        }
        void TearDown() override {

            if (viraDevice) {
                viraDevice.reset();   
            }
        }
        std::unique_ptr<TestableViraDevice> viraDevice; 

};

TEST_F(ViraDeviceTest, createDispatchLoaderDynamic)
{

    ASSERT_NO_THROW(viraDevice->createDispatchLoaderDynamic());
    EXPECT_NE(viraDevice->dldi, nullptr);
}

TEST_F(ViraDeviceTest, createInstance) {
    viraDevice->createDispatchLoaderDynamic();

    ASSERT_NO_THROW({
        viraDevice->createInstance();
    });
    ASSERT_NE(viraDevice->instance, nullptr);
}

TEST_F(ViraDeviceTest, pickPhysicalDevice) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();

    ASSERT_NO_THROW({
        viraDevice->pickPhysicalDevice();
    });

    ASSERT_TRUE(viraDevice->getPhysicalDevice());
}

TEST_F(ViraDeviceTest, rateDeviceSuitability) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();

    auto& pdevice = *viraDevice->getPhysicalDevice();
    int score = viraDevice->rateDeviceSuitability(pdevice);
    ASSERT_GE(score, 0);  // Should be non-negative if suitable
}

TEST_F(ViraDeviceTest, createLogicalDevice) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();

    ASSERT_NO_THROW({
        viraDevice->createLogicalDevice();
    });

    const auto& vkDevice = viraDevice->getVkDevice();
    ASSERT_TRUE(static_cast<bool>(vkDevice));
}

TEST_F(ViraDeviceTest, createCommandPool) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();
    viraDevice->createLogicalDevice();

    ASSERT_NO_THROW({
        viraDevice->createCommandPool();
    });

    ASSERT_TRUE(viraDevice->commandPool);
}

TEST_F(ViraDeviceTest, createShaderModule) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();
    viraDevice->createLogicalDevice();

    auto testCode = readFile("./shaders/miss_radiance.spv");
    vk::UniqueShaderModule shaderModule;

    ASSERT_NO_THROW({
        shaderModule = viraDevice->createShaderModule(testCode);
    });

    ASSERT_TRUE(static_cast<bool>(shaderModule));
}

TEST_F(ViraDeviceTest, populateDebugMessengerCreateInfo) {
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
    ASSERT_NO_THROW({
        viraDevice->populateDebugMessengerCreateInfo(createInfo);
    });

    ASSERT_TRUE(static_cast<bool>(createInfo.pfnUserCallback));
    ASSERT_TRUE(createInfo.messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
}

TEST_F(ViraDeviceTest, setupDebugMessenger) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();

    // Enable validation layers explicitly if not already
    if (!viraDevice->enableValidationLayers) GTEST_SKIP() << "Validation layers disabled";

    ASSERT_NO_THROW({
        viraDevice->setupDebugMessenger();
    });

    ASSERT_TRUE(static_cast<bool>(viraDevice->debugMessenger));
}

TEST_F(ViraDeviceTest, checkValidationLayerSupport) {
    viraDevice->createDispatchLoaderDynamic();

    bool result = viraDevice->checkValidationLayerSupport();
    ASSERT_TRUE(result || !viraDevice->enableValidationLayers);
}

TEST_F(ViraDeviceTest, getRequiredInstanceExtensions) {
    std::vector<const char*> extensions;

    ASSERT_NO_THROW({
        extensions = viraDevice->getRequiredInstanceExtensions();
    });

    ASSERT_FALSE(extensions.empty());
}

TEST_F(ViraDeviceTest, checkInstanceExtensionSupport) {
    viraDevice->createDispatchLoaderDynamic();
    std::vector<const char*> requiredExtensions = viraDevice->getRequiredInstanceExtensions();

    bool supported = false;
    ASSERT_NO_THROW({
        supported = viraDevice->checkInstanceExtensionSupport(requiredExtensions);
    });

    ASSERT_TRUE(supported || !viraDevice->enableValidationLayers);  // skip if layers are off
}

TEST_F(ViraDeviceTest, checkDeviceExtensionSupport) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();

    auto& pdevice = *viraDevice->getPhysicalDevice();
    bool result = false;

    ASSERT_NO_THROW({
        result = viraDevice->checkDeviceExtensionSupport(pdevice);
    });

    ASSERT_TRUE(result);
}

TEST_F(ViraDeviceTest, findQueueFamilies) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();

    auto& pdevice = *viraDevice->getPhysicalDevice();
    QueueFamilyIndices indices;

    ASSERT_NO_THROW({
        indices = viraDevice->findQueueFamilies(pdevice);
    });

    ASSERT_TRUE(indices.graphicsFamilyHasValue);
    if (viraDevice->surface_) {
        ASSERT_TRUE(indices.presentFamilyHasValue);
    }
}

TEST_F(ViraDeviceTest, findSupportedFormat) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();

    std::vector<vk::Format> candidates = {
        vk::Format::eR32Sfloat, 
        vk::Format::eR8G8B8A8Unorm
    };

    ASSERT_NO_THROW({
        vk::Format result = viraDevice->findSupportedFormat(
            candidates, 
            vk::ImageTiling::eOptimal, 
            vk::FormatFeatureFlagBits::eSampledImage
        );
        ASSERT_NE(result, vk::Format::eUndefined);
    });
}

TEST_F(ViraDeviceTest, findMemoryType) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->pickPhysicalDevice();

    vk::PhysicalDeviceMemoryProperties memProps = viraDevice->getPhysicalDevice()->getMemoryProperties();
    uint32_t memoryTypeBits = (1 << memProps.memoryTypeCount) - 1;
    EXPECT_NO_THROW({
        uint32_t typeIndex = viraDevice->findMemoryType(memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        EXPECT_LT(typeIndex, memProps.memoryTypeCount);
    });
}

TEST_F(ViraDeviceTest, createBuffer) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->setupDebugMessenger();
    viraDevice->pickPhysicalDevice();
    viraDevice->createLogicalDevice();
    viraDevice->createCommandPool();
    
    
    vk::UniqueBuffer buffer;
    vk::DeviceSize size = 1024;


    auto mem = viraDevice->createBuffer(
        size,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        buffer
    );
    EXPECT_TRUE(buffer);
    EXPECT_TRUE(mem != nullptr);
}

TEST_F(ViraDeviceTest, transitionImageLayout) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->setupDebugMessenger();
    viraDevice->pickPhysicalDevice();
    viraDevice->createLogicalDevice();
    viraDevice->createCommandPool();
    
    
    // Create a dummy image
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = vk::Extent3D(64, 64, 1);
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = vk::Format::eR8G8B8A8Unorm;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::Image image;
    std::unique_ptr<MemoryInterface> memory;
    viraDevice->createImageWithInfo(imageInfo, vk::MemoryPropertyFlagBits::eDeviceLocal, image, memory);

    EXPECT_NO_THROW({
        viraDevice->transitionImageLayout(
            image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            1,
            vk::ImageAspectFlagBits::eColor
        );
    });

    // Clean up
    viraDevice->device()->destroyImage(image, nullptr);
}

TEST_F(ViraDeviceTest, queryPhysicalDeviceProperties2) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->setupDebugMessenger();
    viraDevice->pickPhysicalDevice();
    viraDevice->createLogicalDevice();

    EXPECT_NO_THROW({
        viraDevice->queryPhysicalDeviceProperties2();
    });
}

TEST_F(ViraDeviceTest, extractSpectralSetData) {
    viraDevice->createDispatchLoaderDynamic();
    viraDevice->createInstance();
    viraDevice->setupDebugMessenger();
    viraDevice->pickPhysicalDevice();
    viraDevice->createLogicalDevice();
    viraDevice->createCommandPool();
    

    // Create Image
    vira::Resolution resolution(16, 16);
    vk::Image image;
    std::unique_ptr<MemoryInterface> memory;

    vk::ImageCreateInfo imageInfo = vk::ImageCreateInfo{}
        .setImageType(vk::ImageType::e2D)
        .setFormat(vk::Format::eR32G32B32A32Sfloat)
        .setExtent({ static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1) // No multisampling
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setInitialLayout(vk::ImageLayout::eUndefined);
    vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    viraDevice->createImageWithInfo(imageInfo, properties, image, memory);

    viraDevice->transitionImageLayout(
        image,                      // Image to transition
        vk::ImageLayout::eUndefined,            // Old layout
        vk::ImageLayout::eColorAttachmentOptimal,              // New layout
        vk::PipelineStageFlagBits::eTopOfPipe,   // Source pipeline stage
        vk::PipelineStageFlagBits::eColorAttachmentOutput,   // Destination pipeline stage
        1
    );  

    auto data = viraDevice->extractSpectralSetData(image, resolution);
    EXPECT_EQ(data.size(), resolution.x * resolution.y);

    viraDevice->device()->destroyImage(image, nullptr);
}
