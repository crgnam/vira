#include "vulkan_testing.hpp"

using namespace vira::vulkan;

// == Test Framework ==
class BufferTest : public ::testing::Test {
protected:
    void SetUp() override {

        // Create Vira Instance and Vira Device
        std::shared_ptr<InstanceFactoryInterface> instanceFactory = std::make_shared<VulkanInstanceFactory>();
        std::shared_ptr<DeviceFactoryInterface> deviceFactory = std::make_shared<VulkanDeviceFactory>();

        // Create Fully Initialized ViraDevice
        viraDevice = std::make_unique<TestableViraDevice>(instanceFactory, deviceFactory);

        // Create Data Vector
        for (std::size_t i = 0; i < nDataPoints; ++i) {
            dataVector.emplace_back(4 * i, 4 * i + 1, 4 * i + 2, 4 * i + 3);
        } 

        // Define buffer size
        bufferSize = sizeof(vira::vec4<float>)*nDataPoints;

        // Query Physical Device Properties
        vk::PhysicalDeviceProperties deviceProperties = viraDevice->getPhysicalDeviceProperties();

        // Get Alignments
        minStorageBufferOffsetAlignment = deviceProperties.limits.minStorageBufferOffsetAlignment;
        minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
        minIndexOffsetAlignment = sizeof(uint32_t);
        nonCoherentAtomSize = deviceProperties.limits.nonCoherentAtomSize;

        // Create Fully Initialized ViraBuffer with coherant memory
        viraBuffer =  std::make_unique<TestableViraBuffer>(viraDevice.get(), sizeof(vira::vec4<float>), static_cast<uint32_t>(nDataPoints), 
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
            minStorageBufferOffsetAlignment);

        // Create Fully Initialized ViraBuffer with non-coherant memory
        viraBufferNC =  std::make_unique<TestableViraBuffer>(viraDevice.get(), sizeof(vira::vec4<float>), static_cast<uint32_t>(nDataPoints), 
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress, 
            vk::MemoryPropertyFlagBits::eHostVisible, 
            minStorageBufferOffsetAlignment);

    }
    void TearDown() override {

        if (viraBuffer) {
            viraBuffer.reset();         // destroy buffer before device
        }

        if (viraBufferNC) {
            viraBufferNC.reset();       // destroy buffer before device
        }
    
        if (viraDevice) {
            viraDevice.reset();         // cleanup ViraDevice wrapper
        }
    }
    

    // Members accessible to all tests using this fixture
    std::unique_ptr<TestableViraDevice> viraDevice; 
    std::unique_ptr<TestableViraBuffer> viraBuffer;
    std::unique_ptr<TestableViraBuffer> viraBufferNC;

    std::vector<glm::vec4> dataVector;
    size_t nDataPoints = 100;

    size_t bufferSize;
    float epsilon = 1e-5f;

    size_t minStorageBufferOffsetAlignment;
    size_t minUniformBufferOffsetAlignment;
    size_t minIndexOffsetAlignment;
    size_t minAccelerationStructureScratchOffsetAlignment;
    size_t nonCoherentAtomSize;
};

// == Tests ==

TEST_F(BufferTest, constructor) {
    // Setup
    vk::DeviceSize instanceSize = sizeof(vira::vec4<float>);  // 16
    uint32_t instanceCount = 100;
    vk::BufferUsageFlags usageFlags =
        vk::BufferUsageFlagBits::eStorageBuffer |
        vk::BufferUsageFlagBits::eTransferDst |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;
    vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible;
    vk::DeviceSize minOffsetAlignment = viraDevice->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;

    // Expected values
    vk::DeviceSize expectedAlignmentSize = viraBuffer->getAlignment(instanceSize, minOffsetAlignment);
    vk::DeviceSize expectedBufferSize = expectedAlignmentSize * instanceCount;

    // Construction
    std::unique_ptr<ViraBuffer> testBuffer = std::make_unique<ViraBuffer>(
        viraDevice.get(),
        instanceSize,
        instanceCount,
        usageFlags,
        memoryFlags,
        minOffsetAlignment);

    // Verify internal state
    EXPECT_EQ(testBuffer->getInstanceSize(), instanceSize);
    EXPECT_EQ(testBuffer->getInstanceCount(), instanceCount);
    EXPECT_EQ(testBuffer->getAlignmentSize(), expectedAlignmentSize);
    EXPECT_EQ(testBuffer->getBufferSize(), expectedBufferSize);

    // Verify Vulkan buffer is valid
    EXPECT_TRUE(static_cast<bool>(testBuffer->getBuffer()));

    EXPECT_TRUE(testBuffer->getMemory().getMemory() != nullptr);
}

TEST_F(BufferTest, descriptorInfo) {

    // Inputs
    vk::DeviceSize size = 3;
    vk::DeviceSize offset = 5;

    vk::DescriptorBufferInfo output;
    EXPECT_NO_THROW({output = viraBuffer->descriptorInfo(size, offset);});

    EXPECT_EQ(output.buffer, viraBuffer->getBuffer());
    EXPECT_EQ(output.offset, offset);
    EXPECT_EQ(output.range, size);

} 

TEST_F(BufferTest, descriptorInfoForIndex) {

    int index = 2;
    vk::DescriptorBufferInfo output;

    // Expected values
    vk::DeviceSize expectedOffset = index * viraBuffer->getAlignmentSize();
    vk::DeviceSize expectedRange  = viraBuffer->getAlignmentSize();

    // Run and verify
    EXPECT_NO_THROW({
        output = viraBuffer->descriptorInfoForIndex(index);
    });

    EXPECT_EQ(output.buffer, viraBuffer->getBuffer());
    EXPECT_EQ(output.offset, expectedOffset);
    EXPECT_EQ(output.range, expectedRange);

}

TEST_F(BufferTest, flushAndInvalidate) {
    vk::DeviceSize offset = 0;
    vk::DeviceSize size = nonCoherentAtomSize;  // Aligned to nonCoherentAtomSize

    ASSERT_NO_THROW({ viraBufferNC->map(size, offset); });

    void* mappedPtr = viraBufferNC->getMappedMemory();
    ASSERT_NE(mappedPtr, nullptr);

    // Write data (simulate CPU-side modification)
    vira::vec4<float> input(1.0f, 2.0f, 3.0f, 4.0f);
    std::memcpy(static_cast<uint8_t*>(mappedPtr) + offset, &input, sizeof(input));

    // Flush 
    vk::Result flushResult;
    ASSERT_NO_THROW({
        flushResult = viraBufferNC->flush(size, offset);
    });
    EXPECT_EQ(flushResult, vk::Result::eSuccess);

    // Invalidate 
    vk::Result invalidateResult;
    ASSERT_NO_THROW({
        invalidateResult = viraBufferNC->invalidate(size, offset);
    });
    EXPECT_EQ(invalidateResult, vk::Result::eSuccess);

    // Read back and verify content is still correct
    vira::vec4<float> output;
    std::memcpy(&output, static_cast<uint8_t*>(mappedPtr) + offset, sizeof(output));
    float epsilon = 1e-5f;
    EXPECT_TRUE(glm::all(glm::epsilonEqual(output, input, epsilon)));

    // Cleanup
    ASSERT_NO_THROW({ viraBufferNC->unmap(); });
}

TEST_F(BufferTest, flushAndInvalidate_ByIndex) {
    int bytesPerElement = static_cast<int>(viraBuffer->getAlignmentSize());
    int index = static_cast<int>(nonCoherentAtomSize / bytesPerElement); 

    std::cout << "Alignment size: " << bytesPerElement << "\n";
    std::cout << "Index: " << index << "\n";
    std::cout << "Offset: " << index * bytesPerElement << "\n";
    std::cout << "Total mapped size: " << bufferSize << "\n";

    ASSERT_NO_THROW({ viraBufferNC->map(); });

    void* mappedPtr = viraBufferNC->getMappedMemory();
    ASSERT_NE(mappedPtr, nullptr);

    // Write input value at the correct offset
    vira::vec4<float> input(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_NO_THROW({
        viraBufferNC->writeToIndex(&input, index);
    });

    // Flush
    vk::Result flushResult;
    ASSERT_NO_THROW({
        flushResult = viraBufferNC->flushIndex(index);
    });
    EXPECT_EQ(flushResult, vk::Result::eSuccess);

    // Invalidate
    vk::Result invalidateResult;
    ASSERT_NO_THROW({
        invalidateResult = viraBufferNC->invalidateIndex(index);
    });
    EXPECT_EQ(invalidateResult, vk::Result::eSuccess);

    // Check result
    vira::vec4<float> output;
    vk::DeviceSize offset = index * viraBufferNC->getAlignmentSize();
    std::memcpy(&output, static_cast<uint8_t*>(mappedPtr) + offset, sizeof(output));

    EXPECT_TRUE(glm::all(glm::epsilonEqual(output, input, epsilon)));

    ASSERT_NO_THROW({ viraBufferNC->unmap(); });
}

TEST_F(BufferTest, getAlignment) {

    size_t instanceSize = 32;
    size_t minOffsetAlignment = 64;
    size_t alignment;
    size_t expectedAlignment = (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);

    EXPECT_NO_THROW({alignment = viraBuffer->getAlignment(instanceSize, minOffsetAlignment);});

    EXPECT_EQ(alignment, expectedAlignment);
    
} 

TEST_F(BufferTest, getBufferDeviceAddress) {
    vk::DeviceAddress address = 0;

    EXPECT_NO_THROW({
        address = viraBuffer->getBufferDeviceAddress();
    });

    // Check that the address is not null (0 means something went wrong)
    EXPECT_NE(address, 0) << "Device address should not be zero";

    vk::BufferDeviceAddressInfo addressInfo{};
    addressInfo.buffer = viraBuffer->getBuffer();

    vk::DeviceAddress direct = viraDevice->device()->getBufferAddress(addressInfo);
    EXPECT_EQ(address, direct) << "Mismatch between wrapper and direct device call";
}

TEST_F(BufferTest, mappingWriting) {
    vk::Result result;

    // How many data points to slice and write
    size_t nDataToWrite = 10;

    // Which data element to start slicing from
    size_t iDataToSlice = 5;

    // Where in the buffer to write the slice to
    size_t indexToWriteTo = 10;
    size_t offsetToWriteTo = indexToWriteTo*sizeof(vira::vec4<float>);

    // Size of data being written
    size_t dataSizeToWrite = nDataToWrite * sizeof(vira::vec4<float>);

    // Initialize dataOut with zeros
    std::vector<vira::vec4<float>> dataOut(nDataPoints, vira::vec4<float>(0.0));

    // Create a data slice of elements 5 through 14 of the frameworks' dataVec
    std::vector<vira::vec4<float>> dataSlice(dataVector.begin() + iDataToSlice, dataVector.begin() + iDataToSlice + nDataToWrite);

    // Map the data and test for success
    EXPECT_NO_THROW({result = viraBuffer->map();});
    EXPECT_EQ(result, vk::Result::eSuccess);

    // Write the data slice to the 10th through 19th elements of the buffer
    EXPECT_NO_THROW({viraBuffer->writeToBuffer(dataSlice.data(), dataSizeToWrite, offsetToWriteTo);});

    // Retrieve the data in the buffer
    void* pData = viraBuffer->getMappedMemory();
    std::memcpy(dataOut.data(), reinterpret_cast<vira::vec4<float>*>(pData), nDataPoints * sizeof(vira::vec4<float>));
    
    // Unmap the data
    EXPECT_NO_THROW({viraBuffer->unmap();});
    EXPECT_EQ(viraBuffer->getMappedMemory(), nullptr);

    // Check the correct data was written in the correct place
    vira::vec4<float> zerosVec4(0.0);

    // Check the Unwritten sections
    for (size_t i = 0; i < indexToWriteTo; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(zerosVec4, dataOut[i], epsilon))) << "Data at [" << i << "] should be zero.";
    }
    for (size_t i = indexToWriteTo + nDataToWrite; i < nDataPoints; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(zerosVec4, dataOut[i], epsilon))) << "Data after written region should be zero.";
    }

    // Check the Written section
    for (size_t i = 0; i < nDataToWrite; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(dataSlice[i], dataOut[indexToWriteTo + i], epsilon))) 
            << "Mismatch at written index [" << indexToWriteTo + i << "]: got " 
            << glm::to_string(dataOut[indexToWriteTo + i]) << ", expected " 
            << glm::to_string(dataSlice[i]);
    }

} 

TEST_F(BufferTest, mappingWriting_ByIndex) {
    vk::Result result;

    int indexToWriteTo = 10;
    int nDataToWrite = 10;
    int iDataToSlice = 5;

    // Write one value at a time using writeToIndex
    std::vector<vira::vec4<float>> dataOut(nDataPoints, vira::vec4<float>(0.0));
    std::vector<vira::vec4<float>> dataSlice(dataVector.begin() + iDataToSlice, dataVector.begin() + iDataToSlice + nDataToWrite);

    // Map the data and test for success
    EXPECT_NO_THROW({ result = viraBuffer->map(); });
    EXPECT_EQ(result, vk::Result::eSuccess);

    // Write using the index-based version
    for (int i = 0; i < nDataToWrite; ++i) {
        EXPECT_NO_THROW({
            viraBuffer->writeToIndex(&dataSlice[i], indexToWriteTo + i);
        });
    }

    // Retrieve the data in the buffer
    void* pData = viraBuffer->getMappedMemory();
    // std::memcpy(dataOut.data(), reinterpret_cast<vira::vec4<float>*>(pData), nDataPoints * sizeof(vira::vec4<float>));
    for (int i = 0; i < nDataToWrite; ++i) {
        std::memcpy(
            &dataOut[indexToWriteTo + i],
            static_cast<uint8_t*>(pData) + (indexToWriteTo + i) * viraBuffer->getAlignmentSize(),
            sizeof(vira::vec4<float>)
        );
    }
    
    // Unmap
    EXPECT_NO_THROW({ viraBuffer->unmap(); });
    EXPECT_EQ(viraBuffer->getMappedMemory(), nullptr);

    // Check unwritten section
    vira::vec4<float> zerosVec4(0.0);
    for (int i = 0; i < indexToWriteTo; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(zerosVec4, dataOut[i], epsilon))) << "Data at [" << i << "] should be zero.";
    }
    for (int i = indexToWriteTo + nDataToWrite; i < nDataPoints; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(zerosVec4, dataOut[i], epsilon))) << "Data after written region should be zero.";
    }

    // Check written section
    for (int i = 0; i < nDataToWrite; ++i) {
        EXPECT_TRUE(glm::all(glm::epsilonEqual(dataSlice[i], dataOut[indexToWriteTo + i], epsilon)))
            << "Mismatch at written index [" << indexToWriteTo + i << "]: got "
            << glm::to_string(dataOut[indexToWriteTo + i]) << ", expected "
            << glm::to_string(dataSlice[i]);
    }
}