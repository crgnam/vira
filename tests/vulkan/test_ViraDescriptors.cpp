#include "vulkan_testing.hpp"

using namespace vira::vulkan;

// *************** Descriptor Set Layout *********************

class DescriptorTest : public ::testing::Test {
    protected:
        void SetUp() override {
        
            // Create Vira Instance and Vira Device
            std::shared_ptr<InstanceFactoryInterface> instanceFactory = std::make_shared<VulkanInstanceFactory>();
            std::shared_ptr<DeviceFactoryInterface> deviceFactory = std::make_shared<VulkanDeviceFactory>();

            // Create Fully Initialized ViraDevice
            viraDevice = std::make_unique<TestableViraDevice>(instanceFactory, deviceFactory);

            pool = std::make_unique<ViraDescriptorPool>(
                viraDevice.get(),
                1,
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                std::vector{
                    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1},
                    vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1},
                    vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 2},
                    vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1}
                }
            );

            // Create a descriptor set layout with a binding for each type used in tests
            layout = ViraDescriptorSetLayout::Builder(viraDevice.get())
                .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment)
                .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
                .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, 2)
                .addBinding(3, vk::DescriptorType::eStorageTexelBuffer, vk::ShaderStageFlagBits::eFragment, 1)
                .build();

                writer = std::make_unique<ViraDescriptorWriter>(*layout, *pool);
            

            // ImageView
            vk::ImageCreateInfo imageInfo{};
            imageInfo.imageType = vk::ImageType::e2D;
            imageInfo.format = vk::Format::eR8G8B8A8Unorm;
            imageInfo.extent = vk::Extent3D{ 128, 128, 1 };
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = vk::SampleCountFlagBits::e1;
            imageInfo.tiling = vk::ImageTiling::eOptimal;
            imageInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
            imageInfo.sharingMode = vk::SharingMode::eExclusive;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined;

            viraDevice->createImageWithInfo(
                imageInfo,
                vk::MemoryPropertyFlagBits::eDeviceLocal,
                testImage,
                testImageMemory
            );

            vk::ImageViewCreateInfo imageViewInfo{};
            imageViewInfo.image = testImage;
            imageViewInfo.viewType = vk::ImageViewType::e2D;
            imageViewInfo.format = vk::Format::eR8G8B8A8Unorm;
            imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;
            testImageView = viraDevice->device()->createImageView(imageViewInfo, nullptr);

            // Sampler
            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;        
            testSampler = viraDevice->device()->createSampler(samplerInfo, nullptr);

            // Buffer        
            testBufferMemory = viraDevice->createBuffer(
                sizeof(float) * 256, // or whatever size you need
                vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                testBuffer
            );

        }
        void TearDown() override {

            if (testImageView) {
                viraDevice->device()->destroyImageView(testImageView, nullptr);
                testImageView = nullptr;
            }
            if (testBufferView) {
                viraDevice->device()->destroyBufferView(testBufferView, nullptr);
                testBufferView = nullptr;
            }
            if (testSampler) {
                viraDevice->device()->destroySampler(testSampler, nullptr);
                testSampler = nullptr;
            }
            if (testBuffer) {
                testBuffer.reset(); // UniqueBuffer
            }
            if (testImage) {
                viraDevice->device()->destroyImage(testImage, nullptr);
                testImage = nullptr;
            }
            if (testImageMemory) {
                testImageMemory.reset();
            }
            if (testBufferMemory) {
                testBufferMemory.reset();
            }
            if (pool) {
                pool.reset();
            }
            if (layout) {
                layout.reset();
            }
            if (viraDevice) {
                viraDevice.reset(); // Destroys the logical device
            }
        
        }

        // Member Variables
        std::unique_ptr<TestableViraDevice> viraDevice; 
        std::unique_ptr<ViraDescriptorPool> pool;
        std::unique_ptr<ViraDescriptorSetLayout> layout;
        std::unique_ptr<ViraDescriptorWriter> writer;
        vk::DescriptorSet descriptorSet;

        vk::UniqueBuffer testBuffer;
        std::unique_ptr<MemoryInterface> testBufferMemory;

        vk::Image testImage;
        std::unique_ptr<MemoryInterface> testImageMemory;

        vk::Sampler testSampler;
        vk::ImageView testImageView;
        vk::BufferView testBufferView;
};


TEST_F(DescriptorTest, AddBindingCreatesCorrectBinding) {
    ViraDescriptorSetLayout::Builder builder(viraDevice.get());
    builder.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);

    auto bindings = builder.getBindings();
    ASSERT_EQ(bindings.size(), 1);
    ASSERT_EQ(bindings.count(0), 1);

    const auto& binding = bindings.at(0);
    EXPECT_EQ(binding.binding, 0);
    EXPECT_EQ(binding.descriptorType, vk::DescriptorType::eUniformBuffer);
    EXPECT_EQ(binding.descriptorCount, 1);
    EXPECT_EQ(binding.stageFlags, vk::ShaderStageFlagBits::eVertex);
}

TEST_F(DescriptorTest, AddBindingWithCount) {
    ViraDescriptorSetLayout::Builder builder(viraDevice.get());
    builder.addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 4);

    auto bindings = builder.getBindings();
    ASSERT_EQ(bindings.count(2), 1);
    const auto& binding = bindings.at(2);
    EXPECT_EQ(binding.binding, 2);
    EXPECT_EQ(binding.descriptorType, vk::DescriptorType::eCombinedImageSampler);
    EXPECT_EQ(binding.descriptorCount, 4);
    EXPECT_EQ(binding.stageFlags, vk::ShaderStageFlagBits::eFragment);
}

TEST_F(DescriptorTest, BuildCreatesLayoutSuccessfully) {
    ViraDescriptorSetLayout::Builder builder(viraDevice.get());
    builder.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute);

    std::unique_ptr<ViraDescriptorSetLayout> layoutNew;
    EXPECT_NO_THROW({
        layoutNew = builder.build();
    });
    EXPECT_NE(layoutNew, nullptr);
    EXPECT_NE(layoutNew->getDescriptorSetLayout(), vk::DescriptorSetLayout{});
}

TEST_F(DescriptorTest, allocateDescriptorSet) {
    vk::DescriptorSet descriptorSet;
    bool success = pool->allocateDescriptorSet(layout->getDescriptorSetLayout(), descriptorSet);
    EXPECT_TRUE(success);
    EXPECT_TRUE(static_cast<bool>(descriptorSet));
}

TEST_F(DescriptorTest, writeBufferDescriptor) {
    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = 1024;
    bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::UniqueBuffer buffer = viraDevice->device()->createBufferUnique(bufferInfo, nullptr);
    vk::MemoryRequirements memReq = viraDevice->device()->getBufferMemoryRequirements(buffer.get());
    
    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = 0; // Replace with actual valid index for your setup

    std::unique_ptr<MemoryInterface>  memory = viraDevice->device()->allocateMemory(allocInfo, nullptr);
    viraDevice->device()->bindBufferMemory(buffer.get(), memory, 0);

    vk::DescriptorBufferInfo descriptorBufferInfo{};
    descriptorBufferInfo.buffer = buffer.get();
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = 1024;

    ViraDescriptorWriter writer(*layout, *pool);
    writer.writeBuffer(0, &descriptorBufferInfo);

    vk::DescriptorSet descriptorSet;
    bool success = writer.build(descriptorSet);
    EXPECT_TRUE(success);
    EXPECT_TRUE(static_cast<bool>(descriptorSet));

    // Clean up
    memory.reset();
    buffer.reset();
}

TEST_F(DescriptorTest, writeGenericBufferDescriptor) {

    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = 512;
    bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::UniqueBuffer buffer = viraDevice->device()->createBufferUnique(bufferInfo, nullptr);
    auto memReq = viraDevice->device()->getBufferMemoryRequirements(buffer.get());

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = 0; 

    std::unique_ptr<MemoryInterface>  memory = viraDevice->device()->allocateMemory(allocInfo, nullptr);
    viraDevice->device()->bindBufferMemory(buffer.get(), memory, 0);

    vk::DescriptorBufferInfo info{};
    info.buffer = buffer.get();
    info.offset = 0;
    info.range = 512;

    ViraDescriptorWriter writer(*layout, *pool);
    writer.write(0, &info); // generic write
    vk::DescriptorSet set;
    bool success = writer.build(set);

    EXPECT_TRUE(success);
    EXPECT_TRUE(static_cast<bool>(set));

    // Cleanup
    memory.reset();
    buffer.reset();
}
TEST_F(DescriptorTest, OverwriteUpdatesDescriptorSet) {
    vk::DescriptorSet descriptorSet;
    ASSERT_TRUE(pool->allocateDescriptorSet(layout->getDescriptorSetLayout(), descriptorSet));

    // Fill writer with a dummy write
    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = testBuffer.get();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(float);

    writer->writeBuffer(0, &bufferInfo);

    // Overwrite
    EXPECT_NO_THROW(writer->overwrite(descriptorSet));
}

TEST_F(DescriptorTest, WriteTexturesSucceeds) {
    vk::DescriptorImageInfo imageInfo = {};
    imageInfo.sampler = testSampler;
    imageInfo.imageView = testImageView;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    std::vector<vk::DescriptorImageInfo> imageInfos{imageInfo};

    EXPECT_NO_THROW(writer->writeTextures(1, imageInfos));
}

TEST_F(DescriptorTest, WriteBufferArraySucceeds) {
    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = testBuffer.get();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(float);

    std::vector<vk::DescriptorBufferInfo> bufferInfos{bufferInfo};

    EXPECT_NO_THROW(writer->writeBufferArray(2, bufferInfos));
}

TEST_F(DescriptorTest, WriteBufferViewsSucceeds) {

    vk::UniqueBuffer bufferViewBuffer;

    std::unique_ptr<MemoryInterface> bufferViewMemory = viraDevice->createBuffer(
        sizeof(uint32_t) * 256,
        vk::BufferUsageFlagBits::eUniformTexelBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        bufferViewBuffer
    );

    vk::BufferViewCreateInfo viewInfo{};
    viewInfo.buffer = bufferViewBuffer.get();
    viewInfo.format = vk::Format::eR32Uint;
    viewInfo.range = VK_WHOLE_SIZE;
    
    vk::BufferView bufferView = viraDevice->device()->createBufferView(viewInfo, nullptr);
    std::vector<vk::BufferView> bufferViews;
    bufferViews.push_back(bufferView);

    EXPECT_NO_THROW(writer->writeBufferViews(3, bufferViews));
    viraDevice->device()->destroyBufferView(bufferView, nullptr);
    bufferViewBuffer.reset();
    bufferViewMemory.reset();
}