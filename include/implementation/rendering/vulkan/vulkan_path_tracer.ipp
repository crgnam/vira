#include <stdexcept>

#include "vulkan/vulkan.hpp"

#include "vira/cameras/camera.hpp"
#include "vira/scene/scene.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/geometry/vertex.hpp"

#include "vira/rendering/vulkan/viewport_camera.hpp"

#include "vira/rendering/vulkan_private/vulkan_render_resources.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/render_loop.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.hpp"

namespace vira::vulkan {  

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::VulkanPathTracer(
        ViewportCamera<TSpectral, TFloat>* viewportCamera, 
        Scene<TSpectral, TFloat, TMeshFloat>* scene, 
        ViraDevice* viraDevice, 
        ViraSwapchain* viraSwapchain,
        VulkanRendererOptions options) : viewportCamera{viewportCamera}, scene{scene}, viraDevice{viraDevice}, viraSwapchain{viraSwapchain}, options{options} {

        // Query Device Properties
        getPhysicalDeviceProperties2();

        // Initialize spectral number related objects
        nSpectralBands = TSpectral::size();
        nSpectralSets = static_cast<size_t>(std::ceil(static_cast<float>(nSpectralBands) / 4.0f));
        nSpectralPad = 4 * nSpectralSets - nSpectralBands;
        
        // Initialize Image Parameters
        Resolution resolution = viewportCamera->getResolution();
        nPixels = resolution.x * resolution.y;
        
        // Initialize Geometry Parameters
        numBLAS = scene->getMeshes().size();

        // Run PathTracer Initialization
        std::cout << "\nInitializing Vulkan PathTracer Renderer" << std::endl;
        initialize();
        std::cout << "Vulkan PathTracer Renderer Created" << std::endl;
        
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::initialize() {
        viraSwapchain->createPathTracerImageResources(nSpectralSets);
        createCamera();
        createLights();
        createMaterials();
        createTLAS();
        setupDescriptors();
        setupRayTracingPipeline();
        createShaderBindingTable(*pipelines[0]);
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::setupDescriptors() {

        // =======================
        // DESCRIPTOR SET 0: Camera
        // =======================

        // Layout    
        std::unique_ptr<ViraDescriptorSetLayout> cameraDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR) // viewportCamera parameters
                .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)
                .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR) // viewportCamera parameters                    
                .build();

        // Create and build Descriptor Pool
        cameraDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eUniformBuffer, 1) 
                .addPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
                .addPoolSize(vk::DescriptorType::eStorageBuffer, 1)
                .setMaxSets(1)
                .build();

        // Create Descriptor Writer
        ViraDescriptorWriter cameraDescriptorWriter = ViraDescriptorWriter(*cameraDescriptorSetLayout, *cameraDescriptorPool);

        // Write Binding 0: Camera
        vk::DescriptorBufferInfo cameraBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(cameraBuffer->getBuffer())
            .setOffset(0)
            .setRange(sizeof(VulkanCamera<TSpectral, TFloat>));
        cameraDescriptorWriter.write(0, &cameraBufferInfo);

        // Write Binding 1: Pixel Solid Angle Texture
        vk::DescriptorImageInfo pixelSolidAngleImageInfo = vk::DescriptorImageInfo()
                .setSampler(pixelSolidAngleTexture.sampler)
                .setImageView(pixelSolidAngleTexture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        
        cameraDescriptorWriter.write(1, &pixelSolidAngleImageInfo);


        // Write Binding 2: Optical Efficiency
        vk::DescriptorBufferInfo optEffBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(optEffBuffer->getBuffer())
            .setOffset(0)
            .setRange(sizeof(TFloat)*nSpectralBands);
        
        cameraDescriptorWriter.write(2, &optEffBufferInfo);


        // Build Descriptor Set
        if (!cameraDescriptorWriter.build(cameraDescriptorSet)) {
            throw std::runtime_error("Failed to build global descriptor set");
        }

        // =======================
        // DESCRIPTOR SET 1: Material Info and Textures
        // =======================

        // Create Layout    
            std::unique_ptr<ViraDescriptorSetLayout> materialDescriptorSetLayout = ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, 1)
                .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, static_cast<uint32_t>(numMaterials))
                .addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, static_cast<uint32_t>(numMaterials))
                .addBinding(3, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, static_cast<uint32_t>(numMaterials))
                .addBinding(4, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, static_cast<uint32_t>(numMaterials))
                .addBinding(5, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, static_cast<uint32_t>(numMaterials))
                .addBinding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR, static_cast<uint32_t>(numMaterials))
                .build();


        // Create Descriptor Pool
        materialDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eStorageBuffer, 1)
                .addPoolSize(vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(numMaterials * 6))
                .setMaxSets(1)
                .build();            

        // Create Descriptor Writer
        ViraDescriptorWriter materialDescriptorWriter = ViraDescriptorWriter(*materialDescriptorSetLayout, *materialDescriptorPool);

        // **Binding 0: Material Data Buffer**
        vk::DescriptorBufferInfo materialBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(materialBuffer->getBuffer())
            .setOffset(0)
            .setRange(sizeof(MaterialInfo)*numMaterials);
        materialDescriptorWriter.write(0, &materialBufferInfo);


        // **Binding 1: Normal Textures
        std::vector<vk::DescriptorImageInfo> normalImageInfos;
        for (const auto& texture : normalTextures) {
            normalImageInfos.push_back(vk::DescriptorImageInfo()
                .setSampler(texture.sampler)
                .setImageView(texture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
        }
        materialDescriptorWriter.write(1, normalImageInfos.data());

        // **Binding 2: Roughness Textures
        std::vector<vk::DescriptorImageInfo> roughnessImageInfos;
        for (const auto& texture : roughnessTextures) {
            roughnessImageInfos.push_back(vk::DescriptorImageInfo()
                .setSampler(texture.sampler)
                .setImageView(texture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
        }
        materialDescriptorWriter.write(2, roughnessImageInfos.data());

        // **Binding 3: Metalness Textures
        std::vector<vk::DescriptorImageInfo> metalnessImageInfos;
        for (const auto& texture : metalnessTextures) {
            metalnessImageInfos.push_back(vk::DescriptorImageInfo()
                .setSampler(texture.sampler)
                .setImageView(texture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
        }
        materialDescriptorWriter.write(3, metalnessImageInfos.data());

        // **Binding 4: Transmission Textures
        std::vector<vk::DescriptorImageInfo> transmissionImageInfos;
        for (const auto& texture : transmissionTextures) {
            transmissionImageInfos.push_back(vk::DescriptorImageInfo()
                .setSampler(texture.sampler)
                .setImageView(texture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
        }
        materialDescriptorWriter.write(4, transmissionImageInfos.data());

        // **Binding 5: Albedo Textures
        std::vector<vk::DescriptorImageInfo> albedoImageInfos;
        for (const auto& texture : albedoTextures) {
            albedoImageInfos.push_back(vk::DescriptorImageInfo()
                .setSampler(texture.sampler)
                .setImageView(texture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
        }
        materialDescriptorWriter.write(5, albedoImageInfos.data());

        // **Binding 6: Emission Textures
        std::vector<vk::DescriptorImageInfo> emissionImageInfos;
        for (const auto& texture : emissionTextures) {
            emissionImageInfos.push_back(vk::DescriptorImageInfo()
                .setSampler(texture.sampler)
                .setImageView(texture.imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
        }
        materialDescriptorWriter.write(6, emissionImageInfos.data());

        if (!materialDescriptorWriter.build(materialDescriptorSet)) {
            throw std::runtime_error("Failed to build material descriptor set");
        }
        
        // =======================
        // DESCRIPTOR SET 2: Lights
        // =======================

        std::unique_ptr<ViraDescriptorSetLayout> lightDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR)       // Light data buffer
                .build();

        // Descriptor Pool for Light Resources
        lightDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eUniformBuffer, 1) 
                .setMaxSets(1)
                .build();
        if (!lightDescriptorPool) {
            throw std::runtime_error("Failed to create light descriptor pool");
        } 

        // Lights Descriptor Set
        ViraDescriptorWriter lightDescriptorWriter = ViraDescriptorWriter(*lightDescriptorSetLayout, *lightDescriptorPool);

        // Binding 0: Light Buffer
        vk::DescriptorBufferInfo lightBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(lightBuffer->getBuffer())
            .setOffset(0)
            .setRange(sizeof(VulkanLight<TSpectral, TFloat>) * numLights);
        lightDescriptorWriter.write(0, &lightBufferInfo);

        // Build Descriptor Set
        if (!lightDescriptorWriter.build(lightDescriptorSet)) {
            throw std::runtime_error("Failed to build light descriptor set");
        }


        // =======================
        // DESCRIPTOR SET 3: Geometry Resources
        // =======================

        geometryDescriptorSetLayout = 
        ViraDescriptorSetLayout::Builder(viraDevice)
            .addBinding(0, vk::DescriptorType::eAccelerationStructureKHR, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR) // TLAS  
            .build();

        // Descriptor Pool for Geometry Resources
        geometryDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1)
                .setMaxSets(1)
                .build();

        ViraDescriptorWriter geometryDescriptorWriter = ViraDescriptorWriter(*geometryDescriptorSetLayout, *geometryDescriptorPool);

        // Binding 0 : TLAS
        std::array<vk::AccelerationStructureKHR, 1> tlasArray = {tlas};
        const vk::ArrayProxyNoTemporaries<const vk::AccelerationStructureKHR> vTLAS(tlasArray);

        vk::WriteDescriptorSetAccelerationStructureKHR accelStructureInfo = vk::WriteDescriptorSetAccelerationStructureKHR()
            .setAccelerationStructures(vTLAS);

        geometryDescriptorWriter.writeAccelStructDescriptor(0, &accelStructureInfo);
        
        // Build Descriptor Set
        if (!geometryDescriptorWriter.build(geometryDescriptorSet)) {
            throw std::runtime_error("Failed to build geometry descriptor set");
        }


        // =======================
        // DESCRIPTOR SET 4: Mesh Resources (Vertices, triangleMaterialIDs, global material offsets)
        // =======================

        for (size_t i = 0; i < numBLAS; ++i) {
            assert(vec4VertexViraBuffers[i]->getBuffer() != VK_NULL_HANDLE);
            assert(vertexIndexBuffers[i] != VK_NULL_HANDLE);
            assert(blasResources[i]->matIDFaceBuffer.get() != VK_NULL_HANDLE);
        }
        assert(materialOffsetsBuffer->getBuffer() != VK_NULL_HANDLE);

        std::unique_ptr<ViraDescriptorSetLayout> meshDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR, static_cast<uint32_t>(numBLAS)) // Vertices (array of vertex buffers)
                .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR, static_cast<uint32_t>(numBLAS)) // Index buffers for each mesh (flattened)
                .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR, static_cast<uint32_t>(numBLAS)) //  Face Material IDs (array of materialID buffers)
                .addBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR, static_cast<uint32_t>(numBLAS)) // Material Offsets (single material offset vector)
                .addBinding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eClosestHitKHR, static_cast<uint32_t>(numBLAS)) // Vertices' Excess Spectral Albedo Channels (all but first four channels)
                .build();        

        // Descriptor Pool for Mesh Resources
        meshDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eStorageBuffer, static_cast<uint32_t>(5*numBLAS))
                .setMaxSets(1)
                .build();

        ViraDescriptorWriter meshDescriptorWriter = ViraDescriptorWriter(*meshDescriptorSetLayout, *meshDescriptorPool);

        // Binding 0: Vertex Buffer Array (storage buffer)
        std::vector<vk::DescriptorBufferInfo> vertexBufferInfos(numBLAS);
        for (size_t i = 0; i < numBLAS; ++i) {
            vertexBufferInfos[i] = vk::DescriptorBufferInfo()
                .setBuffer(vec4VertexViraBuffers[i]->getBuffer())
                .setOffset(0)
                .setRange(VK_WHOLE_SIZE);
        }
        meshDescriptorWriter.writeBufferDescriptor(0, vertexBufferInfos.data());


        // Binding 1: Index Buffer Array (storage buffer)
        std::vector<vk::DescriptorBufferInfo> indexBufferInfos(numBLAS);
        for (size_t i = 0; i < numBLAS; ++i) {
            indexBufferInfos[i] = vk::DescriptorBufferInfo()
                .setBuffer(vertexIndexBuffers[i])
                .setOffset(0)
                .setRange(VK_WHOLE_SIZE);
        }
        meshDescriptorWriter.writeBufferDescriptor(1, indexBufferInfos.data());


        // Binding 2: Face Material IDs Buffer Array (storage buffer)
        std::vector<vk::DescriptorBufferInfo> faceMatIDBufferInfos(numBLAS);
        for (size_t i = 0; i < numBLAS; ++i) {
            faceMatIDBufferInfos[i] = vk::DescriptorBufferInfo()
                .setBuffer(blasResources[i]->matIDFaceBuffer.get())
                .setOffset(0)
                .setRange(VK_WHOLE_SIZE);
        }
        meshDescriptorWriter.writeBufferDescriptor(2, faceMatIDBufferInfos.data());


        // Binding 3: Material Offsets
        vk::DescriptorBufferInfo materialOffsetsBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(materialOffsetsBuffer->getBuffer()) 
            .setOffset(0)                                  
            .setRange(materialOffsets.size() * sizeof(uint8_t));
        meshDescriptorWriter.writeBufferDescriptor(3, &materialOffsetsBufferInfo); 

        
        // Binding 4 (optional): Extra Albedo Channels (for nSpectralBands > 4)
        vk::DescriptorBufferInfo albedoBufferInfo = vk::DescriptorBufferInfo()
        .setBuffer(albedoExtraChannelsBuffer->getBuffer()) 
        .setOffset(0)                                  
        .setRange(albedoExtraChannelsBuffer->getInstanceCount() * albedoExtraChannelsBuffer->getInstanceSize());
        meshDescriptorWriter.write(4, &albedoBufferInfo); 

        // Build Descriptor Set
        if (!meshDescriptorWriter.build(meshDescriptorSet)) {
            throw std::runtime_error("Failed to build vertex descriptor set");
        }
    

        // =======================
        // DESCRIPTOR SET 5: Image Resources
        // =======================

        std::unique_ptr<ViraDescriptorSetLayout> imageDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eStorageImage,  vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eRaygenKHR, static_cast<uint32_t>(nSpectralSets))
                .addBinding(1, vk::DescriptorType::eStorageImage,  vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eRaygenKHR)
                .build();

        // Descriptor Pool for Image Resources
        imageDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eStorageImage, static_cast<uint32_t>(nSpectralSets + 1)) 
                .setMaxSets(1)
                .build();

        // Image Descriptor Set
        ViraDescriptorWriter imageDescriptorWriter = ViraDescriptorWriter(*imageDescriptorSetLayout, *imageDescriptorPool);

        // **Binding 0: Spectral Image Data**
        std::vector<vk::DescriptorImageInfo> imageInfos(nSpectralSets);
        for (int i = 0; i < nSpectralSets; ++i) {
            imageInfos[i] = vk::DescriptorImageInfo()
                .setImageView(viraSwapchain->getSpectralImageView(i))
                .setImageLayout(vk::ImageLayout::eGeneral); // Storage image layout
        }
        imageDescriptorWriter.writeImageDescriptor(0, imageInfos.data());

        // **Binding 1: Scalar Image Data**
        vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
                .setImageView(viraSwapchain->getScalarImageView())
                .setImageLayout(vk::ImageLayout::eGeneral); // Storage image layout
        
        imageDescriptorWriter.writeImageDescriptor(1, &imageInfo);

        // Build Descriptor
        if (!imageDescriptorWriter.build(imageDescriptorSet)) {
            throw std::runtime_error("Failed to build image descriptor set");
        }


        // =======================
        // DESCRIPTOR SET 6 (DEBUG BUFFERS): Special storage buffers to capture gpu calculations and compare to CPU calcs. For Debug Purposes.
        // =======================

        vk::DeviceSize bufferSize = sizeof(uint32_t); // One uint32_t counter

        debugCounterBuffer  = new ViraBuffer(viraDevice, bufferSize, 1, 
                                        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, 
                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
                                        minUniformBufferOffsetAlignment);
        debugCounterBuffer2 = new ViraBuffer(viraDevice, bufferSize, 1, 
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
            minUniformBufferOffsetAlignment);
        debugCounterBuffer3 = new ViraBuffer(viraDevice, bufferSize, 1, 
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
            minUniformBufferOffsetAlignment);
            
        // viraSwapchain->setDebugBuffer(debugCounterBuffer);

        uint32_t counterData = 0;
        debugCounterBuffer->map(bufferSize, 0);
        debugCounterBuffer->writeToBuffer(&counterData, bufferSize, 0);
        debugCounterBuffer->unmap();
    
        debugCounterBuffer2->map(bufferSize, 0);
        debugCounterBuffer2->writeToBuffer(&counterData, bufferSize, 0);
        debugCounterBuffer2->unmap();

        debugCounterBuffer3->map(bufferSize, 0);
        debugCounterBuffer3->writeToBuffer(&counterData, bufferSize, 0);
        debugCounterBuffer3->unmap();

        bufferSize = sizeof(vec4<float>)*nPixels;
        std::vector<vec4<float>> debugData(nPixels, vec4<float>(-10000.0f));
        
        debugVec4Buffer = new ViraBuffer(viraDevice, bufferSize, 1, 
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
            minUniformBufferOffsetAlignment);

        debugVec4Buffer->map(bufferSize, 0);
        debugVec4Buffer->writeToBuffer(debugData.data(), bufferSize, 0);
        debugVec4Buffer->unmap();

        
        // Layout    
        debugDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR)
                .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR)
                .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR)
                .addBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR)
                .build();

        // Create and build Descriptor Pool
        debugDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eStorageBuffer, 4) 
                .setMaxSets(1)
                .build();

        // Create Descriptor Writer
        ViraDescriptorWriter debugDescriptorWriter = ViraDescriptorWriter(*debugDescriptorSetLayout, *debugDescriptorPool);

        // Write Binding 0-2: Debug Counter Buffer
        vk::DescriptorBufferInfo debugBufferInfo = vk::DescriptorBufferInfo()
            .setBuffer(debugCounterBuffer->getBuffer())
            .setOffset(0)
            .setRange(sizeof(uint32_t));

        debugDescriptorWriter.write(0, &debugBufferInfo);

        vk::DescriptorBufferInfo debugBufferInfo2 = vk::DescriptorBufferInfo()
            .setBuffer(debugCounterBuffer2->getBuffer())
            .setOffset(0)
            .setRange(sizeof(uint32_t));

        debugDescriptorWriter.write(1, &debugBufferInfo2);

        vk::DescriptorBufferInfo debugBufferInfo3 = vk::DescriptorBufferInfo()
        .setBuffer(debugCounterBuffer3->getBuffer())
        .setOffset(0)
        .setRange(sizeof(uint32_t));

        debugDescriptorWriter.write(2, &debugBufferInfo3);

        // Write Binding 3: Debug Vec4 Buffer
        vk::DescriptorBufferInfo debugVec4Info = vk::DescriptorBufferInfo()
            .setBuffer(debugVec4Buffer->getBuffer())
            .setOffset(0)
            .setRange(sizeof(vec4<float>) * nPixels);
        debugDescriptorWriter.write(3, &debugVec4Info);


        // Build Descriptor Set
        if (!debugDescriptorWriter.build(debugDescriptorSet)) {
            throw std::runtime_error("Failed to build global descriptor set");
        }        


        // =======================
        // CREATE PIPELINE LAYOUT
        // =======================

        // Define Pipeline Descriptor Sets
        descriptorSets = {
            cameraDescriptorSet,
            materialDescriptorSet,
            lightDescriptorSet,
            geometryDescriptorSet,
            meshDescriptorSet,
            imageDescriptorSet,
            debugDescriptorSet
        };     

        std::array<vk::DescriptorSetLayout, 7> descriptorSetLayouts = {
            cameraDescriptorSetLayout->getDescriptorSetLayout(),
            materialDescriptorSetLayout->getDescriptorSetLayout(),
            lightDescriptorSetLayout->getDescriptorSetLayout(),
            geometryDescriptorSetLayout->getDescriptorSetLayout(),
            meshDescriptorSetLayout->getDescriptorSetLayout(),
            imageDescriptorSetLayout->getDescriptorSetLayout(),
            debugDescriptorSetLayout->getDescriptorSetLayout()
        };
    
        size_t layoutCount = static_cast<size_t>(descriptorSetLayouts.size());
        
        // Define Pipeline Push Constants Layout
        vk::PushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR; 
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);

        // Build Pipeline Layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
            .setPushConstantRanges(pushConstantRange)
            .setSetLayoutCount(static_cast<uint32_t>(layoutCount))
            .setPSetLayouts(descriptorSetLayouts.data());
        pipelineLayout = viraDevice->device()->createPipelineLayout(pipelineLayoutInfo, nullptr);

    }    

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::setupRayTracingPipeline() {

        // =======================
        // Ray Tracing Pipeline Configuration
        // =======================

        // Shader Modules and Stages
        shaderStages.resize(5);

        // Load SPIR-V shaders for ray tracing stages
        auto raygenCode = readFile(options.rayGenShader);
        raygenShaderModule = viraDevice->createShaderModule(raygenCode);
        shaderStages[0].setStage(vk::ShaderStageFlagBits::eRaygenKHR)
                       .setModule(raygenShaderModule.get())
                       .setPName("main");


        auto missRadianceCode = readFile(options.missRadianceShader);
        missShaderModule = viraDevice->createShaderModule(missRadianceCode);
        shaderStages[1].setStage(vk::ShaderStageFlagBits::eMissKHR)
                       .setModule(missShaderModule.get())
                       .setPName("main");

        auto missShadowCode = readFile(options.missShadowShader);
        missShadowShaderModule = viraDevice->createShaderModule(missShadowCode);
        shaderStages[2].setStage(vk::ShaderStageFlagBits::eMissKHR)
                        .setModule(missShadowShaderModule.get())
                        .setPName("main");
                
        auto hitRadianceCode = readFile(options.hitRadianceShader);
        hitRadianceShaderModule = viraDevice->createShaderModule(hitRadianceCode);
        shaderStages[3].setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
                       .setModule(hitRadianceShaderModule.get())
                       .setPName("main");

        auto hitShadowCode = readFile(options.shadowHitShader);
        shadowHitShaderModule = viraDevice->createShaderModule(hitShadowCode);
        shaderStages[4].setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
                        .setModule(shadowHitShaderModule.get())
                        .setPName("main");
               
        assert(shaderStages.size() == 5 && "Shader stages not properly initialized!");

        // for (size_t i = 0; i < shaderStages.size(); ++i) {
        //     std::cout << "ShaderStage[" << i << "] = " 
        //               << vk::to_string(shaderStages[i].stage) 
        //               << ", module = " << (shaderStages[i].module ? "OK" : "NULL") 
        //               << std::endl;
        //     assert(shaderStages[i].module && "Shader stage has null module!");
        // }
        
        // =======================
        // Shader Groups
        // =======================

        /// Ray Generation Shader Group
        shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(0) // Raygen shader index
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR));
  
        /// Miss Radiance Shader Group
        shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(1) // Miss shader index
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR));

        /// Miss Shadow Shader Group
        shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(2) // Shadow miss shader index
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR));

        /// Radiance Hit Shader Group
        shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(3) // Closest hit shader index
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR));

        /// Shadow Hit Shader Group
        shaderGroups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(4)
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR));

        /// Debug Shader Group Count
        assert(shaderGroups.size() == 5 && "Shader groups were not created!");


        // =======================
        // Ray Tracing Pipeline Info
        // =======================

        vk::RayTracingPipelineCreateInfoKHR pipelineCreateInfo{};

        // Set the pipeline layout 
        pipelineCreateInfo.layout = pipelineLayout;

        // Set shader stages
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();

        // Set shader groups
        pipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
        pipelineCreateInfo.pGroups = shaderGroups.data();    

        pipelines = viraDevice->device()->getUniqueDevice()->createRayTracingPipelinesKHRUnique({}, {}, pipelineCreateInfo, nullptr, *viraDevice->dldi).value;

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createShaderBindingTable(vk::Pipeline& inputPipeline) {

        // Shader Counts
        uint32_t raygenShaderCount = options.nRaygenShaders;
        uint32_t missShaderCount = options.nMissShaders;
        uint32_t hitShaderCount = options.nHitShaders;
        uint32_t totalShaderCount = raygenShaderCount + missShaderCount + hitShaderCount;        

        // Aligned Shader Handle Sizes
        //vk::DeviceSize alignedHandleSize = ViraBuffer::getAlignment(shaderGroupHandleSize, shaderGroupHandleAlignment); // TODO Consider removing
        vk::DeviceSize alignedSectionSize = ViraBuffer::getAlignment(shaderGroupHandleSize, shaderGroupBaseAlignment);

        // SBT Section Sizes
        uint32_t raygenSectionSize  = raygenShaderCount * static_cast<uint32_t>(alignedSectionSize);
        uint32_t missSectionSize    = missShaderCount   * static_cast<uint32_t>(alignedSectionSize);
        uint32_t hitSectionSize     = hitShaderCount    * static_cast<uint32_t>(alignedSectionSize);
        uint32_t sbtSize            = totalShaderCount  * static_cast<uint32_t>(alignedSectionSize);

        // Retrieve shader handles        
        shaderGroupHandles.resize(totalShaderCount*shaderGroupHandleSize);
        vk::Result result = viraDevice->device()->getRayTracingShaderGroupHandlesKHR(inputPipeline, 0, totalShaderCount, sbtSize, shaderGroupHandles.data() );
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to retrieve shader group handles.");
        }
      
        // Create SBT
        sbtViraBuffer = new ViraBuffer(viraDevice, sbtSize, 1, vk::BufferUsageFlagBits::eShaderBindingTableKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress, 
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, shaderGroupBaseAlignment);            

        // Write To SBT
        sbtViraBuffer->map(sbtSize);

        // Write raygen shader to SBT
        sbtViraBuffer->writeToBuffer(shaderGroupHandles.data(), raygenSectionSize, 0);

        // Write miss shaders to SBT
        sbtViraBuffer->writeToBuffer(
            reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(shaderGroupHandles.data()) + raygenShaderCount * shaderGroupHandleSize),
            missSectionSize, 
            raygenSectionSize
        );

        // Write hit shaders to SBT
        sbtViraBuffer->writeToBuffer(
            reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(shaderGroupHandles.data()) + (raygenShaderCount + missShaderCount) * shaderGroupHandleSize),
            hitSectionSize, 
            raygenSectionSize + missSectionSize
        );

        sbtViraBuffer->unmap();

        // SBT Regions

        // Get SBT buffer address
        vk::DeviceAddress sbtBufferAddress = sbtViraBuffer->getBufferDeviceAddress();

        // Set up Raygen SBT region
        sbtRegions.raygen = vk::StridedDeviceAddressRegionKHR{
            sbtBufferAddress,       // Base address of the raygen section
            alignedSectionSize,     // Stride (aligned)
            alignedSectionSize      // Total size
        };

        // Set up Miss SBT region
        sbtRegions.miss = vk::StridedDeviceAddressRegionKHR{
            sbtBufferAddress + raygenSectionSize,   
            alignedSectionSize,                     
            alignedSectionSize * 2
        };

        // Set up Hit SBT region
        sbtRegions.hit = vk::StridedDeviceAddressRegionKHR{
            sbtBufferAddress + raygenSectionSize + missSectionSize,
            alignedSectionSize,                     
            alignedSectionSize * 2
        };

        auto printRegion = [](const char* name, const vk::StridedDeviceAddressRegionKHR& region) {
            std::cout << name << ": addr = " << region.deviceAddress
                      << ", stride = " << region.stride
                      << ", size = " << region.size << std::endl;
        };
                
        assert(sbtRegions.miss.size >= 2 * sbtRegions.miss.stride);
        assert(sbtRegions.hit.size >= 2 * sbtRegions.hit.stride);

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::recordRayTracingCommands(vk::CommandBuffer commandBuffer)
    {
        
        // Bind the ray tracing pipeline
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, *pipelines[0], *viraDevice->dldi);
        
        // Check that Descriptor Sets are valid
        for (size_t i = 0; i < descriptorSets.size(); i++) {
            if (!descriptorSets[i]) {        
                std::cerr << "Descriptor set " << i << " is NULL!" << std::endl;
            }
        }

        // Bind Descriptor Sets
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eRayTracingKHR,
            pipelineLayout,
            0, // Starting index of the descriptor sets
            descriptorSets,
            {}, // No dynamic offsets
            *viraDevice->dldi
        );

        // Define and Bind Push Constants
        PushConstants pushConstants = {};
        pushConstants.nSpectralBands = static_cast<uint32_t>(nSpectralBands);
        pushConstants.nLights = static_cast<uint32_t>(numLights);
        pushConstants.nSamples = options.nSamples;
        commandBuffer.pushConstants(
            pipelineLayout,
            vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR, 
            0,                                     
            sizeof(PushConstants),
            &pushConstants,                                    
            *viraDevice->dldi
        );

        // Get Extent of Image for sizing Ray Dispatch
        vk::Extent2D swapExtent = viraSwapchain->getSwapchainExtent();
        useExtent.setWidth(swapExtent.width);
        useExtent.setHeight(swapExtent.height);

        // Create Memory Barrier
        vk::MemoryBarrier memoryBarrier(
            vk::AccessFlagBits::eShaderWrite, 
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite // Ensure full visibility        
        );

        // Create Pipeline Barrier
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eRayTracingShaderKHR, // Source stage (Ray tracing shader writes)
            vk::PipelineStageFlagBits::eRayTracingShaderKHR, // Destination stage (Ray tracing shader reads)
            vk::DependencyFlags{},
            memoryBarrier, // Pass memory barrier
            {}, // No buffer barriers
            {},  // No image barriers
            *viraDevice->dldi
        );
                

        // Dispatch the ray tracing command
        commandBuffer.traceRaysKHR(
            sbtRegions.raygen,          // Ray generation SBT
            sbtRegions.miss,            // Miss SBT
            sbtRegions.hit,             // Hit SBT
            sbtRegions.callable,        // Callable SBT (optional)
            useExtent.width,            // Width of dispatch
            useExtent.height,           // Height of dispatch
            1,                          // Depth of dispatch (typically 1 for 2D rendering)
            *viraDevice->dldi           // Dynamic Dispatcher
        );
       
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::unique_ptr<BLASResources>  VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createBLAS(Mesh<TSpectral, TFloat, TMeshFloat>& mesh) {

        // =======================
        // Create Vertex / Index / Material Buffers
        // =======================

        // == Material Buffer ==

        // Retrieve mesh material IDs (per facet)
        const std::vector<uint8_t> perFaceMaterialIndices = mesh.getMaterialIDs();

        // Create material ID buffer
        vk::MemoryPropertyFlags matIDProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags matIDUsage =  vk::BufferUsageFlagBits::eStorageBuffer |
                                            vk::BufferUsageFlagBits::eTransferDst;

        ViraBuffer matIDFaceViraBuffer = ViraBuffer(viraDevice, sizeof(uint8_t), static_cast<uint32_t>(perFaceMaterialIndices.size()), matIDUsage, matIDProperties, minUniformBufferOffsetAlignment);

        // Write to material ID buffer via staging buffer
        vk::DeviceSize matIDSize = perFaceMaterialIndices.size() * sizeof(uint8_t);
        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, sizeof(uint8_t), static_cast<uint32_t>(perFaceMaterialIndices.size()), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(matIDSize, 0);
        stagingBuffer->writeToBuffer(perFaceMaterialIndices.data(), matIDSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), matIDFaceViraBuffer.getBuffer(), matIDSize);
        
        // == Initialize Vertex Related Buffers ==

        // Retrieve vertices from mesh and create Vec4Vertex objects
        const VertexBuffer<TSpectral, TMeshFloat>& vertices = mesh.getVertexBuffer();
        uint32_t vertexCount = static_cast<uint32_t>(mesh.getVertexCount());

        // Initialize Buffers
        // 1. Vec4Vertex buffer for descriptor binding, inherits vira::Vertex
        // 2. PackedVertex buffer to build the BLAS from, simple structure for vulkan use
        vec4Vertices.clear();
        packedVertices.clear();
        vec4Vertices.reserve(vertexCount);  
        packedVertices.reserve(vertexCount);  
        for (const auto& vertex : vertices) {
           vec4Vertices.emplace_back(vertex); 
           packedVertices.emplace_back(vertex);
        }

        // == Vertex Buffer #1 (Vec4Vertex for Descriptor) ==

        // Create buffer and allocate and bind memory
        vk::MemoryPropertyFlags vec4VertexProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags vec4VertexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                             vk::BufferUsageFlagBits::eShaderDeviceAddress |
                             vk::BufferUsageFlagBits::eTransferDst; 
        
        ViraBuffer* vec4VertexViraBuffer = new ViraBuffer(viraDevice, sizeof(VertexVec4<TSpectral, TMeshFloat>), vertexCount, vec4VertexUsage, vec4VertexProperties, minStorageBufferOffsetAlignment);

        // Create Staging Buffer and write vertices data to it
        vk::DeviceSize vec4VertexDataSize = vertexCount*sizeof(VertexVec4<TSpectral, TMeshFloat>);
        stagingBuffer = new ViraBuffer(viraDevice, sizeof(VertexVec4<TSpectral, TMeshFloat>), vertexCount, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(vec4VertexDataSize, 0);
        stagingBuffer->writeToBuffer(vec4Vertices.data(), vec4VertexDataSize, 0);
        stagingBuffer->unmap();

        // Copy staging buffer to vertex buffer
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), vec4VertexViraBuffer->getBuffer(), vec4VertexDataSize);

        // Add to vertex buffers vector
        vec4VertexViraBuffers.push_back(vec4VertexViraBuffer);

        // == Vertex Buffer #2 (PackedVertex for BLAS Build) ==

        // Create buffer and allocate and bind memory
        vk::MemoryPropertyFlags blasVertexProperties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostVisible;
        vk::BufferUsageFlags blasVertexUsage =  vk::BufferUsageFlagBits::eVertexBuffer |
                                            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                                            vk::BufferUsageFlagBits::eShaderDeviceAddress | 
                                            vk::BufferUsageFlagBits::eStorageBuffer | 
                                            vk::BufferUsageFlagBits::eTransferDst;
        ViraBuffer vertexViraBuffer = ViraBuffer(viraDevice, sizeof(PackedVertex<TSpectral, TMeshFloat>), vertexCount, blasVertexUsage, blasVertexProperties, minStorageBufferOffsetAlignment);
        
        // Copy vertices data into buffer and get device address of buffer
        vk::DeviceSize vertexDataSize = vertexCount * sizeof(PackedVertex<TSpectral, TMeshFloat>);
        vertexViraBuffer.map(vertexDataSize, 0);
        vertexViraBuffer.writeToBuffer(packedVertices.data(), vertexDataSize, 0);
        vertexViraBuffer.unmap();
        vk::DeviceAddress vertexBufferDeviceAddress = vertexViraBuffer.getBufferDeviceAddress();

        // == Extra Albedo Spectral Channels (per-vertex albedo data channels > 4) ==

        // Initialize albedo extra channels storage
        size_t nExtraSpectralSets = nSpectralSets - 1;
        size_t nExtraVec4s = nExtraSpectralSets * vertexCount;
        
        if (nExtraSpectralSets == 0) {
            nExtraVec4s = 1;
        } 
        
        // Create buffer for albedo extra channels
        vk::MemoryPropertyFlags vertexProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags vertexUsage =  vk::BufferUsageFlagBits::eStorageBuffer |
                                            vk::BufferUsageFlagBits::eTransferDst;
        albedoExtraChannelsBuffer = new ViraBuffer(viraDevice, sizeof(glm::vec4), static_cast<uint32_t>(nExtraVec4s), vertexUsage, vertexProperties, minStorageBufferOffsetAlignment);        
        std::vector<vira::vec4<TFloat>> albedoExtraChannels(nExtraVec4s, vira::vec4<TFloat>(0.0f));
        
        if (nExtraSpectralSets > 0) {

            // Write spectral excess albedo data to albedoExtraChannels
            //std::vector<vira::vec4<TFloat>> albedoExtraChannels = splitSpectralToVec4s(mesh.getAlbedos(), true); // TODO Consider removing

        }

        // Write Data to albedoExtraChannelsBuffer via staging buffer
        vk::DeviceSize albedoDataSize = nExtraVec4s * sizeof(glm::vec4);
        ViraBuffer* albedoStagingBuffer = new ViraBuffer(viraDevice, sizeof(glm::vec4), static_cast<uint32_t>(nExtraVec4s), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minStorageBufferOffsetAlignment);

        albedoStagingBuffer->map(albedoDataSize, 0);
        albedoStagingBuffer->writeToBuffer(albedoExtraChannels.data(), albedoDataSize, 0);
        albedoStagingBuffer->unmap();

        viraDevice->copyBuffer(albedoStagingBuffer->getBuffer(), albedoExtraChannelsBuffer->getBuffer(), albedoDataSize);
        
        // == Index Buffer ==

        // Create buffer and allocate and bind memory
        const IndexBuffer indexBuffer = mesh.getIndexBuffer();
        uint32_t indexCount = static_cast<uint32_t>(mesh.getIndexCount());
        vk::MemoryPropertyFlags indexProperties = blasVertexProperties;
        vk::BufferUsageFlags indexUsage =   vk::BufferUsageFlagBits::eIndexBuffer | 
                                            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                                            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
                                            vk::BufferUsageFlagBits::eStorageBuffer | 
                                            vk::BufferUsageFlagBits::eTransferDst;
        ViraBuffer indexViraBuffer = ViraBuffer(viraDevice, sizeof(uint32_t), indexCount, indexUsage, indexProperties, minIndexOffsetAlignment);

        // Copy vertices data into buffer and get device address of buffer
        vk::DeviceSize indexDataSize = indexCount * sizeof(uint32_t);
        indexViraBuffer.map(indexDataSize,0);
        indexViraBuffer.writeToBuffer(indexBuffer.data(), indexDataSize, 0);
        indexViraBuffer.unmap();
        vk::DeviceAddress indexBufferDeviceAddress = indexViraBuffer.getBufferDeviceAddress();
    
        // Add index buffer to vector of index buffers
        vertexIndexBuffers.push_back(indexViraBuffer.getBuffer());
        
        // =======================
        // Build BLAS
        // =======================

        // Create Triangles Data
        uint32_t triCount = static_cast<uint32_t>(mesh.getNumTriangles());
        vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData{};
        trianglesData.setVertexFormat(vk::Format::eR32G32B32Sfloat)
                     .setVertexData(vertexBufferDeviceAddress)
                     .setVertexStride(sizeof(PackedVertex<TSpectral, TMeshFloat>))
                     .setMaxVertex(vertexCount)
                     .setIndexType(vk::IndexType::eUint32)
                     .setIndexData(indexBufferDeviceAddress);  

        // Create AccelerationStructureGeometryKHR
        vk::AccelerationStructureGeometryKHR geometry{};
        geometry.setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setGeometry(trianglesData)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        std::vector<vk::AccelerationStructureGeometryKHR> geometries = {geometry};
        
        // Create build info for acceleration structure
        vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                         .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                         .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                         .setGeometries(geometries)
                         .setGeometryCount(1);

        // Retrieve memory requirements
        vk::AccelerationStructureBuildSizesInfoKHR buildSizes = viraDevice->device()->getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildGeometryInfo, triCount);

        // Create BLAS Buffer
        vk::BufferUsageFlags blasUsage =   vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | 
                                            vk::BufferUsageFlagBits::eTransferDst | 
                                            vk::BufferUsageFlagBits::eShaderDeviceAddress;

        vk::MemoryPropertyFlags blasProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;        
        ViraBuffer blasViraBuffer = ViraBuffer(viraDevice, buildSizes.accelerationStructureSize, 1, blasUsage, blasProperties, minAccelerationStructureScratchOffsetAlignment);
        const vk::Buffer& blasBuffer = blasViraBuffer.getBuffer();

        // Create VK acceleration structure and attach to build geometry info
        vk::AccelerationStructureCreateInfoKHR asCreateInfo{};
        asCreateInfo.setBuffer(blasBuffer)
                    .setSize(buildSizes.accelerationStructureSize)
                    .setType(vk::AccelerationStructureTypeKHR::eBottomLevel);
        vk::AccelerationStructureKHR blas = viraDevice->device()->createAccelerationStructureKHR(asCreateInfo);
        buildGeometryInfo.setDstAccelerationStructure(blas);

        // Create Scratch Buffer for BLAS Build
        vk::BufferUsageFlags scratchUsage =    vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | 
                                                vk::BufferUsageFlagBits::eShaderDeviceAddress;
        vk::MemoryPropertyFlags scratchProperties = blasVertexProperties;
        scratchBLASBuffer = new ViraBuffer(viraDevice, buildSizes.buildScratchSize, 1, scratchUsage, scratchProperties, minAccelerationStructureScratchOffsetAlignment);
        const vk::Buffer& scratchBuffer = scratchBLASBuffer->getBuffer();

        // Get the device address for the scratch buffer
        vk::BufferDeviceAddressInfo scratchBufferAddressInfo{scratchBuffer};
        vk::DeviceAddress scratchBufferAddress = viraDevice->getBufferAddress(scratchBufferAddressInfo);
        buildGeometryInfo.scratchData = vk::DeviceOrHostAddressKHR{ scratchBufferAddress };

        // Create build range info for filling acceleration structure
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(triCount)  // Number of primitives (triangles)
                      .setPrimitiveOffset(0)  // Offset in the index buffer
                      .setFirstVertex(0)
                      .setTransformOffset(0);  // No transforms
        const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &buildRangeInfo;
        std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos = {pBuildRangeInfo};
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = {buildGeometryInfo};

        // Begin command recording
        CommandBufferInterface* commandBuffer = viraDevice->beginSingleTimeCommands();

        // Record BLAS Build Command
        commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfos, pBuildRangeInfos);

        // End command recording and submit for execution
        viraDevice->endSingleTimeCommands(commandBuffer);

        // Retain buffers/memory persistently in a BLASResources structure
        std::unique_ptr<BLASResources> blasResources = std::make_unique<BLASResources>(  blas,
            vertexViraBuffer.moveUniqueBuffer(),
            indexViraBuffer.moveUniqueBuffer(),
            blasViraBuffer.moveUniqueBuffer(),
            matIDFaceViraBuffer.moveUniqueBuffer(),
            vertexViraBuffer.getMemory().moveUniqueMemory(),
            indexViraBuffer.getMemory().moveUniqueMemory(),
            blasViraBuffer.getMemory().moveUniqueMemory(),
            matIDFaceViraBuffer.getMemory().moveUniqueMemory()
        );    
        
        // Return BLAS resources (vertex/index/material buffers and memory)
        return std::move(blasResources);
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createBLASVector() {

        blasResources.reserve(numBLAS);

        // Loop over all scene meshes:
        for (size_t meshIndex = 0; meshIndex < numBLAS; meshIndex++) {
            
            // Retrieve Mesh
            Mesh<TSpectral, TFloat, TMeshFloat>& mesh = *scene->getMeshes()[meshIndex];

            // Create BLAS
            blasResources.push_back(createBLAS(mesh));

            // Add BLAS to vBLAS vector
            vBLAS.push_back(blasResources[meshIndex]->blas);

        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::buildTLAS() {

        // =======================
        // Create Mesh Instances Acceleration Structures
        // =======================

        // Initialize vector of index vectors
        std::vector<std::vector<uint32_t>> indexVecs;

        // Loop over all scene meshes and 
        for (size_t meshIndex = 0; meshIndex < numBLAS; meshIndex++) {
            
            // Retrieve mesh and mesh index buffer
            Mesh<TSpectral, TFloat, TMeshFloat>& mesh = *scene->getMeshes()[meshIndex];
            IndexBuffer indexBuffer = mesh.getIndexBuffer();

            // Add indices and offsets
            indexOffsets.push_back(static_cast<uint32_t>(indexBuffer.size()));
            indexVecs.push_back(mesh.getIndexBuffer());

            // Retrieve Mesh
            const std::vector<Pose<TFloat>>& meshGPs = scene->getMeshGlobalPoses()[meshIndex];

            // Loop over mesh global poses and a mesh instance for each
            for (const Pose<TFloat>& globalPose : meshGPs) {
                    
                // Initialize an ASInstance
                vk::AccelerationStructureInstanceKHR instance;
                
                // Set instance global pose (position/orientation)
                instance.setTransform(getTransformMatrix(globalPose));  // A 3x4 matrix for the instance's transformation
                
                // Set Instance parameters
                instance.setInstanceCustomIndex(static_cast<uint32_t>(meshIndex));  // Custom index, often used in shaders
                instance.setMask(0xFF);  // Visibility mask (all bits set, for visibility in all rays)
                instance.setInstanceShaderBindingTableRecordOffset(0);  // Hit group offset
                instance.setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable);
                
                // Attach BLAS to acceleration structure
                vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
                addressInfo.accelerationStructure = blasResources[meshIndex]->blas;
                vk::DeviceAddress address = viraDevice->device()->getAccelerationStructureAddressKHR(addressInfo);
                instance.setAccelerationStructureReference( address );
                
                // Add acceleration structure to vector
                meshInstances.push_back(instance);

            }
        }

        // =======================
        // Create Index Buffer
        // =======================

        // Flatten the index vectors
        indexVecFlat = joinVectors(indexVecs);
                
        // Index and Index Offset Buffers (for descriptor binding)
        vk::MemoryPropertyFlags indexProps = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags indexUsage = vk::BufferUsageFlagBits::eStorageBuffer |
                            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                            vk::BufferUsageFlagBits::eTransferDst; 
        

        indexBufferFlat = new ViraBuffer(viraDevice, sizeof(uint32_t), static_cast<uint32_t>(indexVecFlat.size()), indexUsage, indexProps, minStorageBufferOffsetAlignment);
        vk::DeviceSize indexDataSize = indexVecFlat.size()*sizeof(uint32_t);
        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, sizeof(uint32_t), static_cast<uint32_t>(indexVecFlat.size()), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(indexDataSize, 0);
        stagingBuffer->writeToBuffer(indexVecFlat.data(), indexDataSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), indexBufferFlat->getBuffer(), indexDataSize);
        
        indexOffsetsBuffer = new ViraBuffer(viraDevice, sizeof(uint32_t), static_cast<uint32_t>(indexOffsets.size()), indexUsage, indexProps, minStorageBufferOffsetAlignment);
        vk::DeviceSize indexOffsetDataSize = indexOffsets.size()*sizeof(uint32_t);
        stagingBuffer = new ViraBuffer(viraDevice, sizeof(uint32_t), static_cast<uint32_t>(indexOffsets.size()), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(indexOffsetDataSize, 0);
        stagingBuffer->writeToBuffer(indexOffsets.data(), indexOffsetDataSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), indexBufferFlat->getBuffer(), indexOffsetDataSize);

        nInstances =  meshInstances.size();

        // =======================
        // Create Instances Buffer
        // =======================

        // Set Instances Buffer usage/properties
        vk::BufferUsageFlags instanceUsage = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eHostVisible;

        // Create Instances Buffer
        instancesViraBuffer = new ViraBuffer(viraDevice, sizeof(vk::AccelerationStructureInstanceKHR), static_cast<uint32_t>(meshInstances.size()), instanceUsage, properties, minStorageBufferOffsetAlignment);

        // Copy instance data into buffer and get device address of buffer
        instancesDataSize = meshInstances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        instancesViraBuffer->map(instancesDataSize, 0);
        instancesViraBuffer->writeToBuffer(meshInstances.data(), instancesDataSize, 0);
        instancesViraBuffer->unmap();        
        vk::DeviceAddress instancesBufferDeviceAddress = instancesViraBuffer->getBufferDeviceAddress();

        // Check BLAS References
        instancesViraBuffer->map(instancesDataSize, 0);
        auto* instanceData = static_cast<vk::AccelerationStructureInstanceKHR*>(instancesViraBuffer->getMappedMemory());
        vk::AccelerationStructureDeviceAddressInfoKHR addrInfo = {};
        addrInfo.accelerationStructure = blasResources[0]->blas;
        vk::DeviceAddress expectedAddress = viraDevice->device()->getAccelerationStructureAddressKHR(addrInfo);
        if (expectedAddress != instanceData[0].accelerationStructureReference) {
            std::cerr << "ERROR: BLAS address mismatch! Expected " 
                      << expectedAddress << " but got " 
                      << instanceData[0].accelerationStructureReference << std::endl;
        }
        instancesViraBuffer->unmap();

        // =======================
        // Create TLAS
        // =======================

        // == Initialize TLAS ==

        // Set TLAS Geometries
        tlasGeometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
        tlasGeometry.geometry.instances = vk::AccelerationStructureGeometryInstancesDataKHR()
            .setArrayOfPointers(VK_FALSE)  // Instance buffer contains data directly
            .setData(instancesBufferDeviceAddress);

        buildGeometryInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        buildGeometryInfo.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate);
        buildGeometryInfo.setMode(vk::BuildAccelerationStructureModeKHR::eBuild);
        buildGeometryInfo.setGeometries(tlasGeometry);

        // Check TLAS Size for validity
        uint32_t instanceCount = static_cast<uint32_t>(meshInstances.size());
        std::vector<uint32_t> instanceCounts{instanceCount};
        vk::AccelerationStructureBuildSizesInfoKHR buildSizes = viraDevice->device()->getAccelerationStructureBuildSizesKHR(
            vk::AccelerationStructureBuildTypeKHR::eDevice,
            buildGeometryInfo,
            static_cast<uint32_t>(nInstances));

        if (buildSizes.accelerationStructureSize == 0) {
            std::cerr << "ERROR: TLAS size is 0!" << std::endl;
        }

        // Create TLAS Buffer
        vk::BufferUsageFlags tlasUsage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        ViraBuffer tlasViraBuffer = ViraBuffer(viraDevice, buildSizes.accelerationStructureSize, 1, tlasUsage, properties, minStorageBufferOffsetAlignment);
        const vk::Buffer& tlasBuffer = tlasViraBuffer.getBuffer();

        // Create TLAS accel structure
        vk::AccelerationStructureCreateInfoKHR tlasCreateInfo{};
        tlasCreateInfo.setBuffer(tlasBuffer);
        tlasCreateInfo.setSize(buildSizes.accelerationStructureSize);
        tlasCreateInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel);

        tlas = viraDevice->device()->createAccelerationStructureKHR(tlasCreateInfo);

        // Create Scratch Buffer for Build and attach to build info
        vk::BufferUsageFlags scratchUsage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress;        
        scratchTLASBuffer = new ViraBuffer(viraDevice, buildSizes.buildScratchSize, 1, scratchUsage, properties);
        const vk::Buffer& scratchBuffer = scratchTLASBuffer->getBuffer();
        vk::BufferDeviceAddressInfo scratchBufferAddressInfo{scratchBuffer};
        vk::DeviceAddress scratchBufferAddress = viraDevice->getBufferAddress(scratchBufferAddressInfo);
        buildGeometryInfo.scratchData = vk::DeviceOrHostAddressKHR{ scratchBufferAddress };
        buildGeometryInfo.setDstAccelerationStructure(tlas);

        // Create build range info for filling acceleration structure
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(instanceCount);
        const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &buildRangeInfo;
        std::vector<const vk::AccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos = {pBuildRangeInfo};

        // Add build geometry info to vector
        std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos = {buildGeometryInfo};


        // == Build TLAS ==

        // Check Build Sizes
        if (buildSizes.accelerationStructureSize == 0) {
            std::cerr << "ERROR: Acceleration structure size is 0!" << std::endl;
        }

        // Begin recording commands
        CommandBufferInterface* commandBuffer = viraDevice->beginSingleTimeCommands();

        // Record Build TLAS Command
        commandBuffer->buildAccelerationStructuresKHR(buildGeometryInfos, pBuildRangeInfos);

        // End command recording and submit for execution
        try {
            viraDevice->endSingleTimeCommands(commandBuffer);
        } catch (const std::exception& ex){
            std::cout << "\n" << ex.what() << "\n";
        };

        // Wait for TLAS Build to complete
        viraDevice->device()->waitIdle();

        // Retrieve device address info for TLAS
        vk::AccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.accelerationStructure = tlas;  
        //vk::DeviceAddress tlasDeviceAddress = viraDevice->device()->getAccelerationStructureAddressKHR(addressInfo); // TODO Consider removing

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createTLAS() {

        // Create a BLAS for each mesh
        createBLASVector();

        // Create Mesh Instances and Build TLAS
        buildTLAS();

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::updateTLAS() {

        // Rebuild Mesh Instances Buffer
        uint32_t instanceIndex = 0;
        for (size_t blasIndex = 0; blasIndex < vBLAS.size(); blasIndex++) {

            for (const Pose<TFloat>& globalPose : scene->getMeshGlobalPoses()[blasIndex]) {

                meshInstances[instanceIndex].setTransform(getTransformMatrix(globalPose));  // A 3x4 matrix for the instance's transformation
                instanceIndex++;

            }    
        }
        instancesViraBuffer->map(instancesDataSize, 0);
        instancesViraBuffer->writeToBuffer(meshInstances.data(), instancesDataSize, 0);
        instancesViraBuffer->unmap();

        // Get Instances Buffer Device Address
        vk::DeviceAddress instancesBufferDeviceAddress = instancesViraBuffer->getBufferDeviceAddress();
        tlasGeometry.geometry.instances.setData(instancesBufferDeviceAddress);
        
        // Create Scratch Buffer for TLAS Build
        const vk::Buffer& scratchBuffer = scratchTLASBuffer->getBuffer();
        vk::BufferDeviceAddressInfo scratchBufferAddressInfo{scratchBuffer};
        vk::DeviceAddress scratchBufferAddress = viraDevice->getBufferAddress(scratchBufferAddressInfo);

        // Set TLAS Build Info
        vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
                .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)  // Use update mode
                .setSrcAccelerationStructure(tlas)  // Updating existing TLAS
                .setDstAccelerationStructure(tlas)  // Destination is also existing TLAS
                .setGeometries(tlasGeometry)  // Updated instance buffer reference
                .setScratchData(scratchBufferAddress);

        // Set TLAS Build Ranges
        vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
        buildRangeInfo.setPrimitiveCount(static_cast<uint32_t>(meshInstances.size()));
        const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = { &buildRangeInfo };

        // Begin command recording
        CommandBufferInterface* commandBuffer = viraDevice->beginSingleTimeCommands();
        
        // Record TLAS Build Command
        commandBuffer->buildAccelerationStructuresKHR(buildInfo, pBuildRangeInfos);

        // End command recording and submit for execution
        viraDevice->endSingleTimeCommands(commandBuffer);

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createMaterials() {

        // Get Scene Meshes
        const std::vector<std::shared_ptr<Mesh<TSpectral, TFloat, TMeshFloat>>>& meshes = scene->getMeshes();            

        // Declare material variables
        std::vector<MaterialInfo> materialInfos;
        std::vector<std::shared_ptr<VulkanMaterial<TSpectral>>> vulkanMaterials;
        std::shared_ptr<VulkanMaterial<TSpectral>> vulkanMaterial;
        std::vector<std::shared_ptr<Material<TSpectral>>> meshMaterials;
        std::vector<uint8_t> perFaceMaterialIndices;
        materialOffsets.clear();

        // Loop through meshes and extract materials
        numMaterials=0;
        for (size_t meshIndex = 0; meshIndex < numBLAS; meshIndex++) {

            // Retrieve offset for current mesh into materials vector
            uint8_t meshMaterialOffset = static_cast<uint8_t>(numMaterials);
            materialOffsets.push_back(meshMaterialOffset);

            // Get materials for this mesh
            meshMaterials = meshes[meshIndex]->getMaterials();

            // Populate vulkanMaterials vector with VulkanMaterial objects created from mesh materials
            for (size_t materialIndex = 0; materialIndex < meshMaterials.size(); materialIndex++) {
                vulkanMaterial = std::make_shared<VulkanMaterial<TSpectral>>(meshMaterials[materialIndex].get());
                materials.push_back(*vulkanMaterial);    
                numMaterials++;

                // Create MaterialInfo for this material
                MaterialInfo materialInfo;
                materialInfo.bsdfType =  materials[materialIndex].bsdfType;
                materialInfo.samplingMethod = materials[materialIndex].samplingMethod;
                materialInfos.push_back(materialInfo);

            }

        }

        // Write materialInfos to materialBuffer
        vk::MemoryPropertyFlags matInfoProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags matInfoUsage =  vk::BufferUsageFlagBits::eStorageBuffer |
                                            vk::BufferUsageFlagBits::eTransferDst;
        vk::DeviceSize matInfoSize = numMaterials * sizeof(MaterialInfo);
        materialBuffer = new ViraBuffer(viraDevice, sizeof(MaterialInfo), static_cast<uint32_t>(numMaterials), matInfoUsage, matInfoProperties, minUniformBufferOffsetAlignment);

        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, sizeof(MaterialInfo), static_cast<uint32_t>(numMaterials), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(matInfoSize, 0);
        stagingBuffer->writeToBuffer(materialInfos.data(), matInfoSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), materialBuffer->getBuffer(), matInfoSize);

        // Write material offsets to materialOffsetsBuffer
        vk::MemoryPropertyFlags matOFFProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags matOFFUsage =  vk::BufferUsageFlagBits::eStorageBuffer |
                                            vk::BufferUsageFlagBits::eTransferDst;
        vk::DeviceSize matOFFSize = materialOffsets.size() * sizeof(uint8_t);
        materialOffsetsBuffer = new ViraBuffer(viraDevice, sizeof(uint8_t), static_cast<uint32_t>(materialOffsets.size()), matOFFUsage, matOFFProperties, minUniformBufferOffsetAlignment);

        stagingBuffer = new ViraBuffer(viraDevice, sizeof(uint8_t), static_cast<uint32_t>(materialOffsets.size()), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(matOFFSize, 0);
        stagingBuffer->writeToBuffer(materialOffsets.data(), matOFFSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), materialOffsetsBuffer->getBuffer(), matOFFSize);

        // Create Texture Arrays
        normalTextures.clear();
        roughnessTextures.clear();
        metalnessTextures.clear();
        transmissionTextures.clear();
        albedoTextures.clear();
        emissionTextures.clear();
        for (size_t materialIndex = 0; materialIndex < materials.size(); materialIndex++) {
            
            normalTextures.push_back( createTextureImage(materials[materialIndex].normalMap, vk::Format::eR8G8B8A8Unorm));
            roughnessTextures.push_back( createTextureImage(materials[materialIndex].roughnessMap,      vk::Format::eR16Sfloat));
            metalnessTextures.push_back( createTextureImage(materials[materialIndex].metalnessMap,      vk::Format::eR16Sfloat));
            transmissionTextures.push_back( createTextureImage(materials[materialIndex].transmissionMap,    vk::Format::eR16Sfloat));    

            std::vector<Image<glm::vec4>> albedoImages = splitSpectralImageToVec4Images(materials[materialIndex].albedoMap); 
            std::vector<Image<glm::vec4>> emissionImages = splitSpectralImageToVec4Images(materials[materialIndex].emissionMap); 

            for (size_t spectralSetIndex = 0; spectralSetIndex < nSpectralSets; ++spectralSetIndex) {

                albedoTextures.push_back( createTextureImage(albedoImages[spectralSetIndex], vk::Format::eR8G8B8A8Unorm));
                emissionTextures.push_back( createTextureImage(emissionImages[spectralSetIndex],    vk::Format::eR16Sfloat));
            }
        }
        Image<float> rawpixelSolidAngleImage = viewportCamera->getPixelSolidAngle();
        Image<float> pixelSolidAngleImage = Image<float>(rawpixelSolidAngleImage.resolution(), float(1.0));
        pixelSolidAngleTexture = createTextureImage(rawpixelSolidAngleImage, vk::Format::eR32Sfloat);

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsPixel TPixel>
    ImageTexture VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createTextureImage(Image<TPixel> image, vk::Format format, bool useNearestTexel) {

        // Get Format Info
        FormatInfo formatInfo = getFormatInfo(format);

        // Flatten and pad image if necessary
        std::vector<float> flattenedImage = flattenWithPadding(image.getVector(),  0.0f);

        // Calculate Image Size
        Resolution resolution = image.resolution();        
        vk::DeviceSize imageSize = image.size() * formatInfo.bytesPerPixel;

        // Create Texture Image and allocate memory
        vk::Image textureImage;      
        std::unique_ptr<MemoryInterface> textureImageMemory;
        TextureImageInfo textureImageInfo(static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), format);
        this->viraDevice->createImageWithInfo(textureImageInfo.info,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         textureImage,
                                         textureImageMemory);

        // Create Staging Buffer
        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, imageSize, 1,    vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);        
        
        // Write pixel data to staging buffer
        stagingBuffer->map(imageSize, 0);
        if (formatInfo.isNormalized) {

            // Convert data if it is a normalized type 
            if(formatInfo.isSigned) {
                std::vector<int8_t> convertedPixels;
                convertedPixels.reserve(flattenedImage.size());
                for (size_t i = 0; i < flattenedImage.size(); i++) {
                    convertedPixels.push_back(static_cast<int8_t>(convertNormalized(flattenedImage[i], formatInfo.isSigned)));
                }
                stagingBuffer->writeToBuffer(convertedPixels.data(), imageSize, 0);
    
            } else {
                std::vector<uint8_t> convertedPixels;
                convertedPixels.reserve(flattenedImage.size());
                for (size_t i = 0; i < flattenedImage.size(); i++) {
                    convertedPixels.push_back(static_cast<int8_t>(convertNormalized(flattenedImage[i], formatInfo.isSigned)));
                }
                stagingBuffer->writeToBuffer(convertedPixels.data(), imageSize, 0);
    
            }
 
        } else {
            
            stagingBuffer->writeToBuffer(image.data(), imageSize, 0);
        
        }
        stagingBuffer->unmap();

        // Transition and copy the staging buffer to the image
        viraDevice->transitionImageLayout(
            textureImage,                       // Image to transition
            vk::ImageLayout::eUndefined,        // Old layout
            vk::ImageLayout::eTransferDstOptimal, // New layout
            vk::PipelineStageFlagBits::eTopOfPipe,  // Source pipeline stage
            vk::PipelineStageFlagBits::eTransfer   // Destination pipeline stage
        );        
        viraDevice->copyBufferToImage(stagingBuffer->getBuffer(), textureImage, static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), 1);
        viraDevice->transitionImageLayout(
            textureImage,
            vk::ImageLayout::eTransferDstOptimal, // Old layout
            vk::ImageLayout::eShaderReadOnlyOptimal, // New layout
            vk::PipelineStageFlagBits::eTransfer, // Source pipeline stage
            vk::PipelineStageFlagBits::eFragmentShader // Destination pipeline stage
        );

        // Create Texture Image View
        TextureViewInfo textureViewInfo(textureImage,format);
        vk::ImageView textureImageView = viraDevice->device()->createImageView(textureViewInfo.info, nullptr);

        // Create Texture Sampler
        SamplerInfo samplerInfo{};
        if(useNearestTexel==true) {
            samplerInfo.info.magFilter = vk::Filter::eNearest;
            samplerInfo.info.minFilter = vk::Filter::eNearest;
            samplerInfo.info.mipmapMode = vk::SamplerMipmapMode::eNearest;
            samplerInfo.info.anisotropyEnable = VK_FALSE;
            samplerInfo.info.maxAnisotropy = 1.0f;
        }
        vk::Sampler textureSampler = viraDevice->device()->createSampler(samplerInfo.info, nullptr);

        // Store texture image memory in persistent variable
        textureMemoryArray.push_back(std::move(textureImageMemory));

        // Return the constructed ImageTexture
        return ImageTexture{ textureImage, textureImageView, textureSampler };

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::updateVulkanScene() {
        
        // Update vulkan scene lights
        updateLights();

        // Update vulkan scene camera
        updateCamera();

        // Rebuild the TLAS
        buildTLAS();
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createCamera() {
    
        // Create Vulkan Camera Struct from the ViewportCamera
        vulkanCamera = new VulkanCamera(*viewportCamera);

        // Create a Vulkan Camera Buffer
        vk::DeviceSize cameraDataSize = sizeof(VulkanCamera<TSpectral, TFloat>);
        cameraBuffer = new ViraBuffer(viraDevice, cameraDataSize, 1, 
                                        vk::BufferUsageFlagBits::eUniformBuffer, 
                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
                                        minUniformBufferOffsetAlignment);

        // Write Vulkan Camera Struct to Buffer
        vk::DeviceSize alignmentSize = ViraBuffer::getAlignment(cameraDataSize, minUniformBufferOffsetAlignment);
        cameraBuffer->map(cameraDataSize, 0);
        cameraBuffer->writeToBuffer(vulkanCamera, alignmentSize, 0);
        cameraBuffer->unmap();

        // Retrieve optical efficiency
        opticalEfficiency = viewportCamera->getOpticalEfficiency();
        void* optEffData  = static_cast<void*>(opticalEfficiency.values.data());

        // Create Optical Efficiency Buffer
        optEffBuffer = new ViraBuffer(viraDevice, sizeof(TFloat), static_cast<uint32_t>(nSpectralBands), 
                                        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, 
                                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                                        minStorageBufferOffsetAlignment);
        size_t optEffSize = sizeof(TFloat) * nSpectralBands;

        // Write Optical Efficiency Data to Buffer
        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, optEffSize, 1, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(optEffSize, 0);
        stagingBuffer->writeToBuffer(optEffData, optEffSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), optEffBuffer->getBuffer(), optEffSize);

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::updateCamera() {

        // Reset VulkanCamera from the updated Viewport Camera
        vk::DeviceSize cameraDataSize = sizeof(VulkanCamera<TSpectral, TFloat>);
        vulkanCamera->resetFromViewportCamera(*viewportCamera);

        // Write VulkanCamera to Buffer
        cameraBuffer->map(cameraDataSize, 0);
        cameraBuffer->writeToBuffer(vulkanCamera, cameraDataSize, 0);
        cameraBuffer->unmap();

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::createLights() {

        // Get Scene Lights
        numLights = scene->getLights().size(); 

        // Create VulkanLights Buffer
        lights.reserve(numLights);          
        lightBuffer = new ViraBuffer(viraDevice, sizeof(VulkanLight<TSpectral, TFloat>), numLights, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minUniformBufferOffsetAlignment);
        
        // Create VulkanLights
        lightDataSize = sizeof(VulkanLight<TSpectral, TFloat>) * numLights;        
        for (const auto& lightPtr : scene->getLights()) {
            lights.emplace_back(lightPtr.get());  // Uses the VulkanLight constructor
        }

        // Write VulkanLights to Buffer
        lightBuffer->map(lightDataSize, 0);
        lightBuffer->writeToBuffer(lights.data(), VK_WHOLE_SIZE, 0);
        lightBuffer->unmap();        

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::updateLights() {

        // Retrieve updated lights and update VulkanLights
        for (size_t i = 0; i < numLights; ++i) {
            
            Light<TSpectral, TFloat>* lightRef = scene->getLights()[i].get(); 
            lights[i].resetFromLight(lightRef);  // Uses the VulkanLight constructor
        }

        // Write updated Vulkan Lights to buffer
        lightBuffer->map(lightDataSize, 0);
        lightBuffer->writeToBuffer(lights.data(), VK_WHOLE_SIZE, 0);
        lightBuffer->unmap();

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vk::TransformMatrixKHR VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::getTransformMatrix(const Pose<TFloat>& pose) {

        // Get Model Matrix
        mat4<TFloat> glmMatrix = pose.getModelMatrix();

        // Build 3x4 Transform Matrix from model matrix 
        // Convert each element from double to float explicitly
        vk::TransformMatrixKHR transformMatrixKHR{
            std::array<std::array<float, 4>, 3>{{
                {static_cast<float>(glmMatrix[0][0]), static_cast<float>(glmMatrix[1][0]), static_cast<float>(glmMatrix[2][0]), static_cast<float>(glmMatrix[3][0])},
                {static_cast<float>(glmMatrix[0][1]), static_cast<float>(glmMatrix[1][1]), static_cast<float>(glmMatrix[2][1]), static_cast<float>(glmMatrix[3][1])},
                {static_cast<float>(glmMatrix[0][2]), static_cast<float>(glmMatrix[1][2]), static_cast<float>(glmMatrix[2][2]), static_cast<float>(glmMatrix[3][2])}
            }}
        };

        // Return 3x4 Transform Matrix
        return transformMatrixKHR;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanPathTracer<TSpectral, TFloat, TMeshFloat>::getPhysicalDeviceProperties2() {

        // Query Physical Device Properties
        vk::PhysicalDeviceProperties deviceProperties = viraDevice->getPhysicalDeviceProperties();
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR accelProperties = viraDevice->getAccelStructProperties();
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTraceProperties = viraDevice->getRayTracingProperties();

        // Get Alignments
        minStorageBufferOffsetAlignment = static_cast<uint32_t>(deviceProperties.limits.minStorageBufferOffsetAlignment);
        minUniformBufferOffsetAlignment = static_cast<uint32_t>(deviceProperties.limits.minUniformBufferOffsetAlignment);
        minIndexOffsetAlignment = sizeof(uint32_t); // Adjust based on index type (2 for uint16_t, 4 for uint32_t)
        minAccelerationStructureScratchOffsetAlignment = accelProperties.minAccelerationStructureScratchOffsetAlignment;

        // Retrieve Shader Group Handle Size and Alignments
        shaderGroupHandleSize = rayTraceProperties.shaderGroupHandleSize;
        shaderGroupBaseAlignment = rayTraceProperties.shaderGroupBaseAlignment;
        shaderGroupHandleAlignment = rayTraceProperties.shaderGroupHandleAlignment;

    }

};