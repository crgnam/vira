#include <stdexcept>
#include <cstddef>
#include <vector>

#include "vulkan/vulkan.hpp"
#include "glm/gtx/string_cast.hpp"

#include "vira/vec.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/scene/scene.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/geometry/vertex.hpp"

#include "vira/rendering/vulkan/viewport_camera.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/render_loop.hpp"
#include "vira/rendering/vulkan_private/vulkan_render_resources.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/memory/vulkan_memory.hpp"

namespace vira::vulkan {

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::VulkanRasterizer(
        ViewportCamera<TSpectral, TFloat>* viewportCamera, 
        Scene<TSpectral, TFloat, TMeshFloat>* scene, 
        ViraDevice* viraDevice, 
        ViraSwapchain* viraSwapchain,
        VulkanRendererOptions options) : viewportCamera{viewportCamera}, scene{scene}, viraDevice{viraDevice}, viraSwapchain{viraSwapchain}, options{options} {

        // Query Physical Device Properties
        getPhysicalDeviceProperties2();

        // Retrieve Camera & Scene Parameters
        Resolution resolution = viewportCamera->getResolution();
        nPixels = resolution.x * resolution.y;
        numMesh = scene->getMeshes().size();

        // Initialize spectral number related objects
        nSpectralBands = TSpectral::size();
        nSpectralSets = static_cast<size_t>(std::ceil(static_cast<float>(nSpectralBands)/4.f));
        nSpectralPad = ( 4 * nSpectralSets ) - nSpectralBands;

        // Initialize / Build Rasterizer
        std::cout << "\nInitializing Vulkan Rasterizer Renderer" << std::endl;
        initialize();        
        std::cout << "Vulkan Rasterizer Renderer Created" << std::endl;

    }

   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::initialize() {
        createCamera();
        createLights();
        createMaterials();
        createGeometries();
        setupDescriptors();
        setupRasterizationPipeline();
    }

   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::createMaterials() {

        // Get Scene Meshes
        const std::vector<std::shared_ptr<Mesh<TSpectral, TFloat, TMeshFloat>>>& meshes = scene->getMeshes();            

        // Declare material variables
        std::vector<std::shared_ptr<VulkanMaterial<TSpectral>>> vulkanMaterials;
        std::vector<std::shared_ptr<Material<TSpectral>>> meshMaterials;
        std::shared_ptr<VulkanMaterial<TSpectral>> vulkanMaterial;
        std::vector<uint8_t> perFaceMaterialIndices;

        std::vector<MaterialInfo> materialInfos;

        // Loop through meshes and extract materials
        numMaterials=0;
        for (size_t meshIndex = 0; meshIndex < numMesh; meshIndex++) {

            uint32_t meshMaterialOffset = static_cast<uint32_t>(numMaterials);

            // Get materials for this mesh
            meshMaterials = meshes[meshIndex]->getMaterials();
            
            vk::MemoryPropertyFlags offsetProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
            vk::BufferUsageFlags offsetUsage =  vk::BufferUsageFlagBits::eUniformBuffer |
                                                vk::BufferUsageFlagBits::eTransferDst;
            materialOffsetBuffer = new ViraBuffer(viraDevice, sizeof(uint8_t), 1, offsetUsage, offsetProperties, minUniformBufferOffsetAlignment);
            materialOffsetBuffer->map(sizeof(uint32_t), 0);
            materialOffsetBuffer->writeToBuffer(&meshMaterialOffset, sizeof(uint32_t), 0);
            materialOffsetBuffer->unmap();

            // Create Per Face Material Indices Buffer for this mesh
            perFaceMaterialIndices = meshes[meshIndex]->getMaterialIDs();

            vk::MemoryPropertyFlags matIDProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
            vk::BufferUsageFlags matIDUsage =  vk::BufferUsageFlagBits::eStorageBuffer |
                                                vk::BufferUsageFlagBits::eTransferDst;

            materialIndexBuffer = new ViraBuffer(viraDevice, sizeof(uint8_t), static_cast<uint32_t>(perFaceMaterialIndices.size()), matIDUsage, matIDProperties, minUniformBufferOffsetAlignment);

            vk::DeviceSize matIDSize = perFaceMaterialIndices.size() * sizeof(uint8_t);

            ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, sizeof(uint8_t), static_cast<uint32_t>(perFaceMaterialIndices.size()), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
            stagingBuffer->map(matIDSize, 0);
            stagingBuffer->writeToBuffer(perFaceMaterialIndices.data(), matIDSize, 0);
            stagingBuffer->unmap();
            viraDevice->copyBuffer(stagingBuffer->getBuffer(), materialIndexBuffer->getBuffer(), matIDSize);

            // Store buffers for offset and index
            matOffsetBuffers.push_back(materialOffsetBuffer);
            matIndexBuffers.push_back(materialIndexBuffer);

            // Create Vulkan Materials and populate materials vector

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
        vk::BufferUsageFlags matInfoUsage =  vk::BufferUsageFlagBits::eUniformBuffer |
                                            vk::BufferUsageFlagBits::eTransferDst;
        vk::DeviceSize matInfoSize = numMaterials * sizeof(MaterialInfo);
        materialBuffer = new ViraBuffer(viraDevice, sizeof(MaterialInfo), static_cast<uint32_t>(numMaterials), matInfoUsage, matInfoProperties, minUniformBufferOffsetAlignment);

        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, sizeof(MaterialInfo), static_cast<uint32_t>(numMaterials), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(matInfoSize, 0);
        stagingBuffer->writeToBuffer(materialInfos.data(), matInfoSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), materialBuffer->getBuffer(), matInfoSize);

        // Create Texture Arrays
        normalTextures.clear();
        roughnessTextures.clear();
        metalnessTextures.clear();
        transmissionTextures.clear();
        albedoTextures.clear();
        emissionTextures.clear();

        // Create texture image, view, and sampler for each texture
        for (size_t materialIndex = 0; materialIndex < materials.size(); materialIndex++) {

            // Split color type data into image data vectors  with vec4<TFloat> pixel type 
            std::vector<Image<vira::vec4<TFloat>>> albedoImages = vira::ImageUtils::splitToVec4Images(materials[materialIndex].albedoMap, TFloat(0.0));
            std::vector<Image<vira::vec4<TFloat>>> emissionImages = vira::ImageUtils::splitToVec4Images(materials[materialIndex].emissionMap, TFloat(0.0));

            // Create a texture image from each spectral set of the color type images
            for ( size_t i = 0; i < albedoImages.size(); ++i) {
                albedoTextures.push_back(createTextureImage(albedoImages[i]));
                emissionTextures.push_back(createTextureImage(emissionImages[i]));
            }

            // Create Non-Color (Scalar and Vec3) texture images
            normalTextures.push_back(createTextureImage(materials[materialIndex].normalMap));
            roughnessTextures.push_back(createTextureImage(materials[materialIndex].roughnessMap));
            metalnessTextures.push_back(createTextureImage(materials[materialIndex].metalnessMap));
            transmissionTextures.push_back(createTextureImage(materials[materialIndex].transmissionMap));

        }

        // Create Precomputed Pixel Solid Angle texture image
        Image<float> rawpixelSolidAngleImage = viewportCamera->getPixelSolidAngle();
        Image<float> pixelSolidAngleImage = Image<float>(rawpixelSolidAngleImage.resolution(), float(1.0));
        pixelSolidAngleTexture = createTextureImage(rawpixelSolidAngleImage, vk::Format::eR32Sfloat);

    }

   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::createGeometries() {

        // Get Scene Meshes
        numMesh = scene->getMeshes().size();
        meshResources.clear();
        meshResources.reserve(numMesh);

        // For each mesh, create a vulkan mesh resources (vertex & index buffers)
        // and update instances buffer with mesh instance global poses
        for (size_t meshIndex = 0; meshIndex < numMesh; meshIndex++) {
            
            Mesh<TSpectral, TFloat, TMeshFloat>& mesh = *scene->getMeshes()[meshIndex];
            const std::vector<Pose<TFloat>>& meshGPs = scene->getMeshGlobalPoses()[meshIndex];

            // Create Mesh Resources
            meshResources.push_back(createMesh(mesh));

            // Update / Create Mesh Instances in Instance Buffer
            updateInstances(meshIndex, meshGPs);

        }

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::updateGeometries() {

        // Update Mesh Instances Global Poses for each Mesh
        for (size_t meshIndex = 0; meshIndex < numMesh; meshIndex++) {

            // Retrieve updated global poses
            const std::vector<Pose<TFloat>>& meshGPs = scene->getMeshGlobalPoses()[meshIndex];

            // Write updated global poses to instance buffer
            updateInstances(meshIndex, meshGPs);

        }
    }


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::unique_ptr<MeshResources>  VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::createMesh(Mesh<TSpectral, TFloat, TMeshFloat>& mesh) {

        // =======================
        // VERTEX BUFFER
        // =======================

        // Retrieve vertices from mesh
        const VertexBuffer<TSpectral, TMeshFloat>& vertices = mesh.getVertexBuffer();
        uint32_t vertexCount = static_cast<uint32_t>(mesh.getVertexCount());

        // Create a vector of Vulkan vec4 vertex for each mesh vertex
        std::vector<VertexVec4<TSpectral, TMeshFloat>> vec4Vertices;
        vec4Vertices.reserve(vertexCount);  
        for (const auto& vertex : vertices) {
           vec4Vertices.emplace_back(vertex);
        }

        // Create buffer and allocate and bind memory
        vk::MemoryPropertyFlags vertexProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags vertexUsage =  vk::BufferUsageFlagBits::eVertexBuffer |
                                            vk::BufferUsageFlagBits::eTransferDst;

        // Create ViraBuffer / vk::Buffer
        ViraBuffer vertexViraBuffer = ViraBuffer(viraDevice, sizeof(VertexVec4<TSpectral, TMeshFloat>), vertexCount, vertexUsage, vertexProperties, minStorageBufferOffsetAlignment);

        vk::DeviceSize vertexDataSize = vertexCount * sizeof(VertexVec4<TSpectral, TMeshFloat>);
        ViraBuffer* vertexStagingBuffer = new ViraBuffer(viraDevice, sizeof(VertexVec4<TSpectral, TMeshFloat>),vertexCount, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minStorageBufferOffsetAlignment);
        vertexStagingBuffer->map(vertexDataSize, 0);
        vertexStagingBuffer->writeToBuffer(vec4Vertices.data(), vertexDataSize, 0);
        vertexStagingBuffer->unmap();
        viraDevice->copyBuffer(vertexStagingBuffer->getBuffer(), vertexViraBuffer.getBuffer(), vertexDataSize);

        std::vector<vira::vec4<TFloat>> albedoExtraChannels;
        if ( nSpectralBands > 4 ) {
            albedoExtraChannels.reserve(vertexCount * (nSpectralSets - 1) );
            //std::vector<vira::vec4<TFloat>> albedoExtraChannels = splitSpectralToVec4s(mesh.getAlbedos(), true); // TODO Consider removing

            // TODO Consider removing
            //vk::MemoryPropertyFlags vertexProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
            //vk::BufferUsageFlags vertexUsage =  vk::BufferUsageFlagBits::eStorageBuffer |
            //                                    vk::BufferUsageFlagBits::eTransferDst;

            // Create ViraBuffer / vk::Buffer
            albedoExtraChannelsBuffer = new ViraBuffer(viraDevice, sizeof(VertexVec4<TSpectral, TMeshFloat>), vertexCount, vertexUsage, vertexProperties, minStorageBufferOffsetAlignment);

            vk::DeviceSize albedoDataSize = vertexCount * sizeof(VertexVec4<TSpectral, TMeshFloat>);

            // TODO Consider removing
            //ViraBuffer* albedoStagingBuffer = new ViraBuffer(viraDevice, sizeof(VertexVec4<TSpectral, TMeshFloat>),vertexCount, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minStorageBufferOffsetAlignment);
            vertexStagingBuffer->map(albedoDataSize, 0);
            vertexStagingBuffer->writeToBuffer(albedoExtraChannels.data(), albedoDataSize, 0);
            vertexStagingBuffer->unmap();
            viraDevice->copyBuffer(vertexStagingBuffer->getBuffer(), albedoExtraChannelsBuffer->getBuffer(), albedoDataSize);
            albedoExtraChannelsBuffers.push_back(albedoExtraChannelsBuffer);
        }


        // =======================
        // INDEX BUFFER
        // =======================

        // Create Buffer
        const IndexBuffer indexBuffer = mesh.getIndexBuffer();
        
        // Set usage, size, and properties of Index Buffer
        uint32_t indexCount = static_cast<uint32_t>(mesh.getIndexCount());
        vk::MemoryPropertyFlags indexProperties = vertexProperties;
        vk::BufferUsageFlags indexUsage =   vk::BufferUsageFlagBits::eIndexBuffer | 
                                            vk::BufferUsageFlagBits::eTransferDst;
        ViraBuffer indexViraBuffer = ViraBuffer(viraDevice, sizeof(uint32_t), indexCount, indexUsage, indexProperties, minIndexOffsetAlignment);
        vk::DeviceSize indexDataSize = indexCount * sizeof(uint32_t);

        // Write Index data to staging buffer and copy to Index Buffer
        ViraBuffer* indexStagingBuffer = new ViraBuffer(viraDevice, sizeof(uint32_t),indexCount, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minIndexOffsetAlignment);
        indexStagingBuffer->map(indexDataSize, 0);
        indexStagingBuffer->writeToBuffer(indexBuffer.data(), indexDataSize, 0);
        indexStagingBuffer->unmap();
        viraDevice->copyBuffer(indexStagingBuffer->getBuffer(), indexViraBuffer.getBuffer(), indexDataSize);


        // =======================
        // DEBUG BUFFERS
        // =======================

        vk::BufferUsageFlags bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        vk::MemoryPropertyFlags memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        //vk::DeviceSize totalSize = sizeof(vec4<TFloat>) * vertexCount; // TODO Consider removing

        debugBuffers.push_back(new ViraBuffer(viraDevice, sizeof(vec4<TFloat>) * nPixels, 1, bufferUsage, memoryProperties, minStorageBufferOffsetAlignment));
        debugBuffers.push_back(new ViraBuffer(viraDevice, sizeof(TFloat) * nPixels, 1, bufferUsage, memoryProperties, minStorageBufferOffsetAlignment));


        std::unique_ptr<MeshResources> thisMeshResources = std::make_unique<MeshResources>(  
            vertexViraBuffer.moveUniqueBuffer(),
            indexViraBuffer.moveUniqueBuffer(),
            vertexViraBuffer.getMemory().moveUniqueMemory(),
            indexViraBuffer.getMemory().moveUniqueMemory()//,

        );        
        thisMeshResources->indexCount = indexCount;
        
        delete vertexStagingBuffer;
        delete indexStagingBuffer;

        // Return Mesh Resources
        return std::move(thisMeshResources);
    }

   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::updateInstances(size_t meshIndex, const std::vector<Pose<TFloat>>& meshGPsUpdate) {

        // Retrieve current mesh resources
        MeshResources* mResources = meshResources[meshIndex].get();

        // Create vector of updated InstanceData
        size_t nPoses = meshGPsUpdate.size();
        std::vector<InstanceData> instances;
        instances.reserve(nPoses);

        // Write updated global poses to instance data vector
        for (size_t poseIndex = 0; poseIndex < nPoses; poseIndex++) {
            instances.push_back(InstanceData{meshGPsUpdate[poseIndex].getModelMatrix(), meshGPsUpdate[poseIndex].getNormalMatrix()});
        }

        // Initialize instances buffer
        vk::MemoryPropertyFlags instanceProperties = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::BufferUsageFlags instanceUsage = 
            vk::BufferUsageFlagBits::eVertexBuffer |         // Required for vertex buffers
            vk::BufferUsageFlagBits::eTransferDst;           // For data upload via staging buffer

        std::unique_ptr<ViraBuffer> instancesViraBuffer = std::make_unique<ViraBuffer>(viraDevice, sizeof(InstanceData), nPoses, instanceUsage, instanceProperties);

        // Write instances data (global poses) to staging buffer
        vk::DeviceSize instancesDataSize = nPoses * sizeof(InstanceData);
        std::unique_ptr<ViraBuffer> stagingBuffer = std::make_unique<ViraBuffer>(
            viraDevice,
            instancesDataSize,
            1,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        stagingBuffer->map(instancesDataSize, 0);
        stagingBuffer->writeToBuffer(instances.data(), instancesDataSize, 0);
        stagingBuffer->unmap();

        // Copy Staging Buffer to Instances Buffer
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), instancesViraBuffer->getBuffer(), instancesDataSize);

        // Update Mesh Resources
        mResources->nInstances = nPoses;    
        mResources->instancesBuffer = std::move(instancesViraBuffer->moveUniqueBuffer());
        mResources->instancesMemory = instancesViraBuffer->getMemory().moveUniqueMemory();

    }

   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::updateVulkanScene() {
        updateGeometries();
        updateLights();
        updateCamera();
    }

    
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::setupDescriptors() {

        // =======================
        // DESCRIPTOR SET 0: Camera 
        // =======================
    
        // Layout    
        std::unique_ptr<ViraDescriptorSetLayout> cameraDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) // viewportCamera parameters
                .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
                .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) // viewportCamera parameters
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
            .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment, 1)
            .addBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(numMaterials))
            .addBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(numMaterials))
            .addBinding(3, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(numMaterials))
            .addBinding(4, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(numMaterials))
            .addBinding(5, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(numMaterials * nSpectralSets))
            .addBinding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, static_cast<uint32_t>(numMaterials * nSpectralSets))
            .build();
            
        // Create Descriptor Pool
        size_t texturePoolSize =( 4 + (2 * nSpectralSets) ) * numMaterials;
        materialDescriptorPool =ViraDescriptorPool::Builder(viraDevice)
            .addPoolSize(vk::DescriptorType::eUniformBuffer, 1)
            .addPoolSize(vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(texturePoolSize))
            .setMaxSets(1)
            .build();            

        // Create Descriptor Set
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
                .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex)       // Light data buffer
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

        if (!lightDescriptorWriter.build(lightDescriptorSet)) {
            throw std::runtime_error("Failed to build light descriptor set");
        }


        // =======================
        // DESCRIPTOR SET 3: Mesh Descriptor Set (Per mesh data e.g. material indexing/offsets)
        // =======================

        // Create Layout    
        std::unique_ptr<ViraDescriptorSetLayout> meshDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) // Global Material Offset
                .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment) // Per Face Material Indices
                .build();

        // Create Descriptor Pool
        meshDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(numMesh)) 
                .addPoolSize(vk::DescriptorType::eStorageBuffer, static_cast<uint32_t>(numMesh)) 
                .setMaxSets(static_cast<uint32_t>(numMesh))
                .build();

        // Create Descriptor Sets (one for each mesh)
        meshDescriptorSets.reserve(numMesh);
        for (uint32_t i = 0; i < numMesh; ++i) {
            ViraDescriptorWriter meshDescriptorWriter = ViraDescriptorWriter(*meshDescriptorSetLayout, *meshDescriptorPool);

            // Binding 0 : Material Offsets
            vk::DescriptorBufferInfo materialOffsetsBufferInfo = vk::DescriptorBufferInfo()
                .setBuffer(matOffsetBuffers[i]->getBuffer()) 
                .setOffset(0)                                  
                .setRange(matOffsetBuffers[i]->getInstanceCount() * matOffsetBuffers[i]->getInstanceSize());
            meshDescriptorWriter.write(0, &materialOffsetsBufferInfo); 

            // Binding 1 : Material Indices
            vk::DescriptorBufferInfo materialIndicesBufferInfo = vk::DescriptorBufferInfo()
                .setBuffer(matIndexBuffers[i]->getBuffer()) 
                .setOffset(0)                                  
                .setRange(matIndexBuffers[i]->getInstanceCount() * matIndexBuffers[i]->getInstanceSize());
            meshDescriptorWriter.write(1, &materialIndicesBufferInfo); 

            // Binding 2 (optional): Extra Albedo Channels (for nSpectralBands > 4)
            if (nSpectralBands > 4) {
                    vk::DescriptorBufferInfo albedoBufferInfo = vk::DescriptorBufferInfo()
                .setBuffer(albedoExtraChannelsBuffers[i]->getBuffer()) 
                .setOffset(0)                                  
                .setRange(albedoExtraChannelsBuffers[i]->getInstanceCount() * albedoExtraChannelsBuffers[i]->getInstanceSize());
                meshDescriptorWriter.write(2, &materialIndicesBufferInfo); 
            }

            // Build Descriptor Set for this Material
            if (!meshDescriptorWriter.build(meshDescriptorSets[i])) {
                throw std::runtime_error("Failed to build global descriptor set");
            }
        }


        // =======================
        // DESCRIPTOR SET 4 (DEBUG BUFFERS): Special storage buffers to capture gpu calculations and compare to CPU calcs. For Debug Purposes.
        // =======================

        // Layout    
        std::unique_ptr<ViraDescriptorSetLayout> debugDescriptorSetLayout = 
            ViraDescriptorSetLayout::Builder(viraDevice)
                .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .addBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                .build();

        // Create and build Descriptor Pool
        debugDescriptorPool = 
            ViraDescriptorPool::Builder(viraDevice)
                .addPoolSize(vk::DescriptorType::eStorageBuffer, 4) 
                .setMaxSets(1)
                .build();

        // Create Descriptor Writer
        ViraDescriptorWriter debugDescriptorWriter = ViraDescriptorWriter(*debugDescriptorSetLayout, *debugDescriptorPool);

        // Write Binding 0: Debug
        vk::DescriptorBufferInfo debugVec4Info = vk::DescriptorBufferInfo()
            .setBuffer(debugBuffers[0]->getBuffer())
            .setOffset(0)
            .setRange(sizeof(vec4<TMeshFloat>)*nPixels);

        debugDescriptorWriter.write(0, &debugVec4Info);

        // Write Binding 1: Debug
        vk::DescriptorBufferInfo debugFloatInfo = vk::DescriptorBufferInfo()
            .setBuffer(debugBuffers[1]->getBuffer())
            .setOffset(0)
            .setRange(sizeof(TMeshFloat)*nPixels);

        debugDescriptorWriter.write(1, &debugFloatInfo);

        // Build Descriptor Set
        if (!debugDescriptorWriter.build(debugDescriptorSet)) {
            throw std::runtime_error("Failed to build global descriptor set");
        }


        // =======================
        // CREATE PIPELINE LAYOUT
        // =======================

        // Set Pipeline Descriptor Sets
        std::array<vk::DescriptorSetLayout, 5> descriptorSetLayouts = {
            cameraDescriptorSetLayout->getDescriptorSetLayout(),
            materialDescriptorSetLayout->getDescriptorSetLayout(),
            lightDescriptorSetLayout->getDescriptorSetLayout(),
            meshDescriptorSetLayout->getDescriptorSetLayout(),
            debugDescriptorSetLayout->getDescriptorSetLayout()
        };
        //uint32_t layoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()); // TODO Consider removing

        // Set Pipeline Push Constants Layout
        vk::PushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex; // Shader stages
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);


        // Set Pipeline Layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
            .setPushConstantRanges(pushConstantRange)
            .setSetLayoutCount(5)
            .setPSetLayouts(descriptorSetLayouts.data());
        pipelineLayout = viraDevice->device()->createPipelineLayout(pipelineLayoutInfo, nullptr);

    }    

   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::setupRasterizationPipeline() {
        
        // Create Vira Pipeline
        viraPipeline = new ViraPipeline<TSpectral, TMeshFloat>(viraDevice);

        // Set Config Info for pipeline
        PipelineConfigInfo* configInfo = new PipelineConfigInfo();
        viraPipeline->defaultPipelineConfigInfo(configInfo);
        configInfo->pipelineLayout = pipelineLayout;
        configInfo->renderPass = viraSwapchain->getRenderPass(); 

        // Build pipeline
        viraPipeline->createRasterizationPipeline(options.vertShader, options.fragShader, configInfo);
        
    }


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::recordDrawCommands(vk::CommandBuffer commandBuffer) {

        // Bind the pipeline
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, viraPipeline->getPipeline(), *viraDevice->dldi);

        // Bind descriptor sets
        std::array<vk::DescriptorSet, 5> globalDescriptorSets = {
            cameraDescriptorSet,
            materialDescriptorSet,
            lightDescriptorSet,
            meshDescriptorSets[0],
            debugDescriptorSet
        };

        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pipelineLayout,
            0, // Starting index of the descriptor sets
            globalDescriptorSets,
            {}, // No dynamic offsets
            *viraDevice->dldi
        );

        // Bind Vertex
        // Loop over meshes
        for (size_t i = 0; i < meshResources.size(); ++i) {
            const auto& meshResource = meshResources[i];

            // Bind the vertex and instances buffer for this mesh
            std::array<vk::Buffer, 2> vertexBuffers = { meshResource->getVertexBuffer(), meshResource->getInstancesBuffer() };
            std::array<vk::DeviceSize, 2> vertexOffsets = { 0, 0 };  // Offsets for vertex and instance buffers
            commandBuffer.bindVertexBuffers(0, vertexBuffers, vertexOffsets, *viraDevice->dldi);

            // Bind the index buffer for this mesh
            commandBuffer.bindIndexBuffer(meshResource->getIndexBuffer(), 0, vk::IndexType::eUint32, *viraDevice->dldi);

            // Set and Bind Push Constants
            PushConstants pushConstants{};
            pushConstants.nSpectralBands = static_cast<uint32_t>(nSpectralBands);
            pushConstants.nLights = static_cast<uint32_t>(numLights);
            commandBuffer.pushConstants(
                pipelineLayout,
                vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(PushConstants),
                &pushConstants,
                *viraDevice->dldi
            );

            // Record the draw call for the current mesh
            commandBuffer.drawIndexed(
                static_cast<uint32_t>(meshResource->indexCount), // Number of indices to draw
                static_cast<uint32_t>(meshResource->nInstances), // Number of instances
                0, // First index
                0, // Vertex offset
                0, // First instance
                *viraDevice->dldi
            );
        }
        
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::createCamera() {

        // Create VulkanCamera from ViewportCamera
        vulkanCamera = new VulkanCamera(*viewportCamera);

        // Write VulkanCamera to camera buffer
        vk::DeviceSize cameraDataSize = sizeof(VulkanCamera<TSpectral, TFloat>);
        cameraBuffer = new ViraBuffer(viraDevice, cameraDataSize, 1, 
                                        vk::BufferUsageFlagBits::eUniformBuffer, 
                                        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, 
                                        minUniformBufferOffsetAlignment);

        vk::DeviceSize alignmentSize = ViraBuffer::getAlignment(cameraDataSize, minUniformBufferOffsetAlignment);
        cameraBuffer->map(cameraDataSize, 0);
        cameraBuffer->writeToBuffer(vulkanCamera, alignmentSize, 0);
        cameraBuffer->unmap();

        // Retrieve Optical Efficiency Data
        opticalEfficiency = viewportCamera->getOpticalEfficiency();
        void* optEffData  = static_cast<void*>(opticalEfficiency.values.data());

        // Write Optical Efficiency to optEffBuffer via Staging Buffer
        optEffBuffer = new ViraBuffer(viraDevice, sizeof(TFloat), static_cast<uint32_t>(nSpectralBands), 
                                        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst, 
                                        vk::MemoryPropertyFlagBits::eDeviceLocal,
                                        minStorageBufferOffsetAlignment);
        size_t optEffSize = sizeof(TFloat) * nSpectralBands;
        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, optEffSize, 1, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(optEffSize, 0);
        stagingBuffer->writeToBuffer(optEffData, optEffSize, 0);
        stagingBuffer->unmap();
        viraDevice->copyBuffer(stagingBuffer->getBuffer(), optEffBuffer->getBuffer(), optEffSize);

    }
   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::updateCamera() {

        // Update VulkanCamera from updated Viewport Camera
        vk::DeviceSize cameraDataSize = sizeof(VulkanCamera<TSpectral, TFloat>);
        vulkanCamera->resetFromViewportCamera(*viewportCamera);

        // Write updated VulkanCamera to camera buffer
        cameraBuffer->map(cameraDataSize, 0);
        cameraBuffer->writeToBuffer(vulkanCamera, cameraDataSize, 0);
        cameraBuffer->unmap();

    }
   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::createLights() {

        // Initialize Vulkan Lights
        numLights = scene->getLights().size(); 
        lights.reserve(numLights);  

        // Create Light Buffer
        lightBuffer = new ViraBuffer(viraDevice, sizeof(VulkanLight<TSpectral, TFloat>), static_cast<uint32_t>(numLights), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, minUniformBufferOffsetAlignment);
        
        // Retrieve Vira Lights and write to Vulkan Lights
        lightDataSize = sizeof(VulkanLight<TSpectral, TFloat>) * numLights;        
        for (const auto& lightPtr : scene->getLights()) {
            lights.emplace_back(lightPtr.get());  // Uses the VulkanLight constructor 
        }

        // Write Vulkan Lights to light buffer
        lightBuffer->map(lightDataSize, 0);
        lightBuffer->writeToBuffer(lights.data(), VK_WHOLE_SIZE, 0);
        lightBuffer->unmap();        

    }
   
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::updateLights() {
        
        // Update Vulkan Lights from updated Vira Lights
        for (size_t i = 0; i < numLights; ++i) {
            
            Light<TSpectral, TFloat>* lightRef = scene->getLights()[i].get(); 
            lights[i].resetFromLight(lightRef);  // Uses the VulkanLight constructor
        }

        // Write Updated Vulkan Lights to light buffer
        lightBuffer->map(lightDataSize, 0);
        lightBuffer->writeToBuffer(lights.data(), VK_WHOLE_SIZE, 0);
        lightBuffer->unmap();

    }

    // Generate transform matrix from model matrix
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vk::TransformMatrixKHR VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::getTransformMatrix(const Pose<TFloat>& pose) {

        // Get Model Matrix
        mat4<TFloat> glmMatrix = pose.getModelMatrix();

        // Construct 3x4 transform matrix from model matrix
        // Converting each element from double to float explicitly
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

    // Texture Image Creation
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsPixel TImage>
    ImageTexture VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::createTextureImage(Image<TImage> image, vk::Format inputFormat, bool useNearestTexel) {

        // Retrieve Image Parameters
        vk::DeviceSize nChannels = static_cast<vk::DeviceSize>(image.numChannels());
        Resolution resolution = image.resolution();

        // Determine if is vec3 image that needs to be padded to vec4 
        bool usePaddedImage = false;
        if(nChannels == 3) {
            nChannels = 4;
            usePaddedImage = true;
        } 

        // Get image size
        vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(resolution.x*resolution.y*nChannels*sizeof(TFloat));

        // Determine Data Format to use on GPU for data
        vk::Format format;
        if (inputFormat == vk::Format::eUndefined) {
            format = determineFormat(static_cast<uint32_t>(nChannels));
        } else {
            format = inputFormat;
        }

        // Initialize texture image and allocate memory
        vk::Image textureImage;      
        std::unique_ptr<MemoryInterface> textureImageMemory;

        // Create Texture Image
        TextureImageInfo textureImageInfo(static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), format);
        this->viraDevice->createImageWithInfo(textureImageInfo.info,
                                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                                         textureImage,
                                         textureImageMemory);
        
        // Write image (or padded image) to staging buffer
        ViraBuffer* stagingBuffer = new ViraBuffer(viraDevice, imageSize, 1, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        stagingBuffer->map(imageSize, 0);
        if(usePaddedImage) {
            Image<vec4<TFloat>> paddedImage = vira::ImageUtils::padImageToVEC4( image, TFloat(0.0));
            stagingBuffer->writeToBuffer(paddedImage.data(), imageSize, 0);
        } else {
            stagingBuffer->writeToBuffer(image.data(), imageSize, 0);
        }
        stagingBuffer->unmap();

        // Transition and copy the staging buffer to a copyable layout
        viraDevice->transitionImageLayout(
            textureImage,                           // Image to transition
            vk::ImageLayout::eUndefined,            // Old layout
            vk::ImageLayout::eTransferDstOptimal,   // New layout
            vk::PipelineStageFlagBits::eTopOfPipe,  // Source pipeline stage
            vk::PipelineStageFlagBits::eTransfer    // Destination pipeline stage
        );        
        
        // Copy Image to Buffer via Staging Buffer
        viraDevice->copyBufferToImage(stagingBuffer->getBuffer(), textureImage, static_cast<uint32_t>(resolution.x), static_cast<uint32_t>(resolution.y), 1);
    
        // Transition Image back to shader readable layout
        viraDevice->transitionImageLayout(
            textureImage,
            vk::ImageLayout::eTransferDstOptimal,       // Old layout
            vk::ImageLayout::eShaderReadOnlyOptimal,    // New layout
            vk::PipelineStageFlagBits::eTransfer,       // Source pipeline stage
            vk::PipelineStageFlagBits::eFragmentShader  // Destination pipeline stage
        );

        // Create texture ImageView
        TextureViewInfo textureViewInfo(textureImage,format);
        vk::ImageView textureImageView = viraDevice->device()->createImageView(textureViewInfo.info, nullptr);

        // Create texture Sampler
        SamplerInfo samplerInfo{};
        if(useNearestTexel==true) {
            samplerInfo.info.magFilter = vk::Filter::eNearest;
            samplerInfo.info.minFilter = vk::Filter::eNearest;
            samplerInfo.info.mipmapMode = vk::SamplerMipmapMode::eNearest;
            samplerInfo.info.anisotropyEnable = VK_FALSE;
            samplerInfo.info.maxAnisotropy = 1.0f;
        }
        vk::Sampler textureSampler = viraDevice->device()->createSampler(samplerInfo.info, nullptr);

        // Store texture memory
        textureMemoryArray.push_back(std::move(textureImageMemory));

        // Return the constructed ImageTexture
        return ImageTexture{ textureImage, textureImageView, textureSampler };

    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void VulkanRasterizer<TSpectral, TFloat, TMeshFloat>::getPhysicalDeviceProperties2() {

        // Retrieve physical device properties
        vk::PhysicalDeviceProperties deviceProperties = viraDevice->getPhysicalDeviceProperties();

        // Extract alignment info and store in member variables
        minStorageBufferOffsetAlignment = static_cast<uint32_t>(deviceProperties.limits.minStorageBufferOffsetAlignment);
        minUniformBufferOffsetAlignment = static_cast<uint32_t>(deviceProperties.limits.minUniformBufferOffsetAlignment);
        minIndexOffsetAlignment = sizeof(uint32_t);
        
    }

};