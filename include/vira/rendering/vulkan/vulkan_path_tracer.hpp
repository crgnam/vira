#ifndef VIRA_RENDERING_VULKAN_VULKAN_PATH_TRACER_HPP
#define VIRA_RENDERING_VULKAN_VULKAN_PATH_TRACER_HPP

#include "vulkan/vulkan.hpp"

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/scene.hpp"

#include "vira/rendering/vulkan_private/vulkan_render_resources.hpp"
#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/render_loop.hpp"
#include "vira/rendering/vulkan_private/pipeline.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"
#include "vira/rendering/vulkan_private/descriptors.hpp"
#include "vira/rendering/vulkan_private/vulkan_light.hpp"
#include "vira/rendering/vulkan_private/vulkan_camera.hpp"
#include "vira/rendering/vulkan_private/vulkan_material.hpp"

#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/instance_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/instance/vulkan_instance_factory.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/device/device_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/physical_device/physical_device_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/queue/queue_factory_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/memory/memory_interface.hpp"

namespace vira::vulkan {    

    /**
     * @brief Stores and manages buffers and memory for the Bottom Level Acceleration Structures (meshes). This structure also ensures all BLAS resources are kept alive and in scope.
     * 
     * @details  This struct stores and manages buffers and memory for the Bottom Level Acceleration Structures, the base objects that define meshes in the vulkan renderer. This structure also ensures all BLAS resources are kept alive and in scope.
     * @see VulkanPathTracer
    */
    struct BLASResources {
        vk::AccelerationStructureKHR blas;                        // The BLAS handle
        vk::UniqueBuffer vertexBuffer;                            // Unique Vulkan buffer for vertices
        vk::UniqueBuffer indexBuffer;                             // Unique Vulkan buffer for indices
        vk::UniqueBuffer blasBuffer;                              // Unique Vulkan buffer for BLAS storage
        vk::UniqueBuffer matIDFaceBuffer;             
        
        vk::UniqueDeviceMemory vertexMemory;                      // Device memory for vertex buffer
        vk::UniqueDeviceMemory indexMemory;                       // Device memory for index buffer
        vk::UniqueDeviceMemory blasMemory;                        // Device memory for BLAS storage
        vk::UniqueDeviceMemory matIDFaceMemory;                        // Device memory for matID storage

        // Constructor
        BLASResources(
            vk::AccelerationStructureKHR blasHandle,
            vk::UniqueBuffer vBuffer,
            vk::UniqueBuffer iBuffer,
            vk::UniqueBuffer bBuffer,
            vk::UniqueBuffer mBuffer,
            vk::UniqueDeviceMemory vMemory,
            vk::UniqueDeviceMemory iMemory,
            vk::UniqueDeviceMemory bMemory,
            vk::UniqueDeviceMemory mMemory
        ) 
        : blas{blasHandle},
        vertexBuffer{std::move(vBuffer)},
        indexBuffer{std::move(iBuffer)},
        blasBuffer{std::move(bBuffer)},
        matIDFaceBuffer{std::move(mBuffer)},
        vertexMemory{std::move(vMemory)},
        indexMemory{std::move(iMemory)},
        blasMemory{std::move(bMemory)},
        matIDFaceMemory{std::move(mMemory)} {}

        // Destructor
        ~BLASResources() {
        }

        // Disable copying
        BLASResources(const BLASResources&) = delete;
        BLASResources& operator=(const BLASResources&) = delete;

        // Enable moving
        BLASResources(BLASResources&&) noexcept = default;
        BLASResources& operator=(BLASResources&&) noexcept = default;
    };

    /**
     * @brief Defines the shader binding table regions, making the region device addresses available to the pathtracer
     * 
     * @see VulkanPathTracer
    */
    struct ShaderBindingTableRegions {

        vk::StridedDeviceAddressRegionKHR raygen;
        vk::StridedDeviceAddressRegionKHR miss;
        vk::StridedDeviceAddressRegionKHR hit;
        vk::StridedDeviceAddressRegionKHR callable;
    };            


    /**
     * @class VulkanPathTracer
     * @brief Performs Vulkan GPU Pathtracing
     *
     * @details The VulkanPathTracer class creates and manages the vulkan rendering system for a pathtracing workflow, 
     * as well as records vulkan rendering commands to command buffers.
     * This class is designed to match as best as possible the inputs, outputs, and functionality of the CPUPathTracer class.
     *
     * @tparam TFloat The float type used by Vira, which obeys the "IsFloat" concept.
     * @tparam TSpectral The spectral type of light used in the render, such as the material albedos, the light irradiance, etc.
     * This parameter obeys the "IsSpectral" concept.
     * @tparam TMeshFloat The float type used in the meshes.
     * 
     * @param[in] viewportCamera (ViewportCamera) The viewport camera holds and manages camera attributes and methods. It inherits
     * the Vira::Camera class, and is used to construct a VulkanCamera to write to the GPU buffers.
     * @param[in] scene (Scene) The Vira scene to be rendered.
     * @param[in] viraDevice (ViraDevice) The ViraDevice manages the physical and logical Vulkan devices that manage all Vulkan functionality.
     * @param[in] viraSwapchain (ViraSwapchain) The ViraSwapchain manages images/image-frames as render targets, as well as the necessary
     * syncronization objects.
     * @param[in] options (VulkanRendererOptions) The options struct conveys user set options for the render.
     * 
     * ## Member Variables
     *
     * ### Public:
     * - `VulkanCamera<TSpectral, TFloat>* vulkanCamera` : Pointer to the Vulkan camera instance.
     * - `uint32_t minStorageBufferOffsetAlignment` : Minimum required storage buffer offset alignment.
     * - `size_t nPixels` : Total number of pixels in the scene.
     * - `vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo` : Geometry build info for acceleration structures.
     * - `size_t nInstances` : Number of mesh instances in the scene.
     * - `vk::Extent2D useExtent` : The rendering extent being used.
     * - `size_t nSpectralSets` : Number of vec4 images required to hold the spectral data.
     *
     * #### Debug:
     * - `ViraBuffer* debugCounterBuffer` : Debug buffer for performance counters.
     * - `ViraBuffer* debugCounterBuffer2` : Additional debug buffer.
     * - `ViraBuffer* debugCounterBuffer3` : Additional debug buffer.
     * - `ViraBuffer* debugVec4Buffer` : Debug buffer for vec4 values.
     * - `std::unique_ptr<ViraDescriptorSetLayout> debugDescriptorSetLayout` : Descriptor set layout for debugging.
     *
     * #### Descriptors:
     * - `std::unique_ptr<ViraDescriptorSetLayout> geometryDescriptorSetLayout` : Descriptor set layout for geometry data.
     * - `vk::DescriptorSet geometryDescriptorSet` : Descriptor set for geometry.
     * - `std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts` : Descriptor set layouts.
     * - `std::unique_ptr<ViraDescriptorPool> debugDescriptorPool` : Descriptor pool for debugging.
     * - `vk::DescriptorSet debugDescriptorSet` : Descriptor set for debugging.
     *
     * #### Shaders:
     * - `ShaderBindingTableRegions sbtRegions` : Shader binding table regions.
     * - `vk::UniqueShaderModule raygenShaderModule` : Ray generation shader module.
     * - `vk::UniqueShaderModule missShaderModule` : Miss shader module.
     * - `vk::UniqueShaderModule hitRadianceShaderModule` : Closest hit shader module.
     * - `std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups` : List of shader groups for ray tracing.
     * - `std::vector<vk::PipelineShaderStageCreateInfo> shaderStages` : List of pipeline shader stage creation info.
     * - `ViraBuffer* sbtViraBuffer` : Shader binding table buffer.
     * - `VkStridedDeviceAddressRegionKHR raygenSBT` : Strided device address region for ray generation.
     * - `VkStridedDeviceAddressRegionKHR missSBT` : Strided device address region for miss shaders.
     * - `VkStridedDeviceAddressRegionKHR hitSBT` : Strided device address region for hit shaders.
     * - `VkStridedDeviceAddressRegionKHR callableSBT` : Strided device address region for callable shaders.
     *
     * #### Alignments:
     * - `uint32_t minUniformBufferOffsetAlignment` : Minimum alignment for uniform buffers.
     * - `uint32_t minIndexOffsetAlignment` : Minimum index offset alignment.
     * - `uint32_t minAccelerationStructureScratchOffsetAlignment` : Minimum scratch buffer offset alignment for acceleration structures.
     * - `uint32_t shaderGroupBaseAlignment` : Base alignment for shader groups.
     * - `uint32_t shaderGroupHandleAlignment` : Alignment requirement for shader group handles.
     * - `uint32_t shaderGroupHandleSize` : Size of a single shader group handle.
     * - `std::vector<uint8_t> shaderGroupHandles` : Vector storing shader group handle data. 
     *
     * ### Private:
     * - `VulkanRendererOptions options` : Path tracer configuration options.
     * - `ViraDevice* viraDevice` : Pointer to the Vulkan device.
     * - `ViraPipeline<TSpectral, TMeshFloat>* viraPipeline` : Pointer to the rendering pipeline.
     * - `ViraSwapchain* viraSwapchain` : Pointer to the swapchain.
     * - `Scene<TSpectral, TFloat, TMeshFloat>* scene` : Pointer to the scene being rendered.
     * - `ViewportCamera<TSpectral, TFloat>* viewportCamera` : Pointer to the viewport camera.
     * - `std::vector<VulkanLight<TSpectral, TFloat>> lights` : List of lights in the scene.
     *
     * #### Counts/Sizes:
     * - `vk::DeviceSize numBLAS` : Number of bottom-level acceleration structures.
     * - `vk::DeviceSize numLights` : Number of lights in the scene.
     * - `vk::DeviceSize numMaterials` : Total number of materials used across all BLAS.
     * - `size_t nSpectralBands` : Number of spectral bands in rendering.
     * - `size_t nSpectralPad` : Padding for spectral data.
     * - `vk::DeviceSize lightDataSize` : Total size of light data.
     * - `vk::DeviceSize instancesDataSize` : Total size of instance data.
     *
     * #### Camera:
     * - `vk::Extent2D extent` : The camera/image resolution.
     * - `ImageTexture pixelSolidAngleTexture` : Precomputed texture for pixel solid angles.
     * - `TSpectral opticalEfficiency` : Optical efficiency factor.
     * - `ViraBuffer* optEffBuffer` : Buffer for optical efficiency data.
     * - `vk::ImageView outputImageView` : Output image view.
     *
     * #### Geometry:
     * - `std::vector<std::unique_ptr<BLASResources>> blasResources` : List of BLAS resources.
     * - `std::vector<vk::AccelerationStructureKHR> vBLAS` : Vector of bottom-level acceleration structures.
     * - `ViraBuffer* albedoExtraChannelsBuffer` : Buffer for additional vertex albedo channels (for future use for spectral types with nBands > 4).
     * - `std::vector<vk::AccelerationStructureInstanceKHR> meshInstances` : List of mesh instances.
     * - `vk::AccelerationStructureGeometryKHR tlasGeometry` : Top-level acceleration structure geometry.
     * - `vk::AccelerationStructureKHR tlas` : Top-level acceleration structure.
     *
     * #### Reusable Objects
     * - `std::vector<VertexVec4<TSpectral, TMeshFloat>> vec4Vertices` :  Vector of vec4 vertex data (used for BLAS creation).
     * - `std::vector<PackedVertex<TSpectral, TMeshFloat>> packedVertices` : Vector of packed vertex data (used for BLAS creation).
     * 
     * #### Descriptor Sets & Pools, Pipeline:
     * - `std::array<vk::DescriptorSet, 7> descriptorSets` : Descriptor sets.
     * - `std::unique_ptr<ViraDescriptorSetLayout> meshDescriptorSetLayout` : Mesh descriptor set layout.
     * - `vk::DescriptorSet meshDescriptorSet` : Mesh descriptor set.
     * - `std::unique_ptr<ViraDescriptorSetLayout> cameraDescriptorSetLayout` : Camera descriptor set layout.
     * - `vk::DescriptorSet cameraDescriptorSet` : Camera descriptor set.
     * - `std::unique_ptr<ViraDescriptorSetLayout> materialDescriptorSetLayout` : Material descriptor set layout.
     * - `vk::DescriptorSet materialDescriptorSet` : Material descriptor set.
     * - `std::unique_ptr<ViraDescriptorSetLayout> lightDescriptorSetLayout` : Light descriptor set layout.
     * - `vk::DescriptorSet lightDescriptorSet` : Light descriptor set.
     * - `std::unique_ptr<ViraDescriptorSetLayout> imageDescriptorSetLayout` : Image descriptor set layout.
     * - `vk::DescriptorSet imageDescriptorSet` : Image descriptor set.
     * - `std::unique_ptr<ViraDescriptorPool> cameraDescriptorPool` : Camera descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> meshDescriptorPool` : Mesh descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> materialDescriptorPool` : Material descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> lightDescriptorPool` : Light descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> geometryDescriptorPool` : Geometry descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> imageDescriptorPool` : Image descriptor pool.
     * - `vk::PipelineLayout pipelineLayout` : Pipeline layout.
     *
     * #### Materials:
     * - `std::vector<ImageTexture> normalTextures` : List of all normal textures used.
     * - `std::vector<ImageTexture> roughnessTextures` : List of all roughness textures used.
     * - `std::vector<ImageTexture> metalnessTextures` : List of all metalness textures used.
     * - `std::vector<ImageTexture> transmissionTextures` : List of all transmission textures used.
     * - `std::vector<ImageTexture> albedoTextures` : List of all albedo textures used. If nSpectral > 4, the multiple textures objects of a single spectral texture are contiguous.
     * - `std::vector<ImageTexture> emissionTextures` : List of all emission textures used. If nSpectral > 4, the multiple textures objects of a single spectral texture are contiguous.
     * - `std::vector<std::unique_ptr<MemoryInterface>> textureMemoryArray` : Memory storage for textures.
     *
     * #### Buffers:
     * - `ViraBuffer* lightBuffer` : Buffer storing light data.
     * - `ViraBuffer* cameraBuffer` : Buffer storing camera data.
     * - `ViraBuffer* materialBuffer` : Buffer storing material data.
     * - `ViraBuffer* materialOffsetsBuffer` : Buffer for material offsets.
     * - `std::vector<ViraBuffer*> debugBuffers` : List of debug buffers.
     *
     * @see VulkanRenderLoop, CPUPathtracer, VulkanCamera
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class VulkanPathTracer {

        public:

            VulkanPathTracer(ViewportCamera<TSpectral, TFloat>* viewportCamera, Scene<TSpectral, TFloat, TMeshFloat>* scene, ViraDevice* viraDevice, ViraSwapchain* viraSwapchain, VulkanRendererOptions options = VulkanRendererOptions{});

            void initialize();

            /**
             * @brief Creates descriptor sets and layouts to bind rendering resources to the rendering pipeline.
             * 
             * This method creates descriptor sets, layouts, and pools to properly bind the following rendering resources to the pipeline. 
             * 
             *  Camera
             *      Camera parameters
             *  Materials and Textures
             *      Stores material and texture data for per face material responses
             *  Light Resources
             *      Scene light sources
             *  Geometry Resources
             *      Top Level Acceleration Structure (TLAS)
             *  Mesh Resources  
             *      vertex and index buffers for mesh instances
             *  Image resources
             *      Images to write data to
             *  Debug Buffers  
             *      Buffers to store debug information from shaders
             * 
             * Descriptor set bindings make these rendering resources available to use in the pipeline for both reading and writing data.
             * 
             */
            void setupDescriptors();


            /**
             * @brief Creates ray tracing pipeline and binds shaders to it
             *
             * This method creates the vulkan render pipeline, loads shaders and organizes them into shader groups/stages to be bound to the pipeline.
             */            
            void setupRayTracingPipeline();

            /**
             * @brief Creates Shader Binding Table (SBT) to map ray tracing shader groups (ray generation, closest hit, miss, calculation, etc.) to GPU memory for efficient access by the pipeline.
             */            
            void createShaderBindingTable(vk::Pipeline& inputPipeline);

            /**
             * @brief Records the ray tracing commands for the current render pass to the command buffer for execution.
             */            
            void recordRayTracingCommands(vk::CommandBuffer commandBuffer);

            std::vector<vk::UniquePipeline> pipelines;

            /**
             * @brief Creates a bottom level accelleration structure from a mesh. Mesh instances are then created for each object in the scene using this mesh referencing this BLAS.
             */   
            std::unique_ptr<BLASResources>  createBLAS(Mesh<TSpectral, TFloat, TMeshFloat>& mesh);
            
            /**
             * @brief Creates all BLASes for the scene and stores them in a vector.
             */   
            void createBLASVector();

            /**
             * @brief Calls createBLASVector and buildTLAS in order to build the Vira scene in vulkan.
             */   
            void createTLAS();

            /**
             * @brief Updates the TLAS after the scene has been updated in Vira.
             */               
            void updateTLAS();
            
            /**
             * @brief Builds the top level accelleration structure (TLAS) that manages the scene geometry in Vulkan.
             */   
            void buildTLAS();

            /**
             * @brief Creates vulkan materials and textures for use in vulkan rendering.
             * 
             * Extracts materials and textures from the Vira scene and writes them to vulkan buffers. Creates and manages indexing and offsets of materials to allow for per-mesh per-facet material indexing.
            */   
            void createMaterials();

            /**
             * @brief Creates a 2D texture sampler from a Vira Image for texture sampling in Vulkan shaders.
             * 
             * Creates a ImageTexture struct from a Vira Image, containing a vulkan image, imageview, and sampler. Manages different image/pixel formats, and does the necessary data padding and conversion for 3 element and normalized formats.
            */ 
            template <IsPixel TImage>
            ImageTexture createTextureImage(Image<TImage> image, 
                                            vk::Format inputFormat = vk::Format::eUndefined, 
                                            bool useNearestTexel = false);            

            /**
             * @brief Updates scene objects and rebuilds the TLAS
             * 
             * Updates the light and camera resources, and then rebuilds the TLAS based on an updated Vira scene.
            */   
            void updateVulkanScene();

            /**
             * @brief Creates vulkan lights for each light in the Vira scene and writes them to a vulkan buffer for use in descriptor bindings.
            */   
            void createLights();
            /**
             * @brief Updates (recreates) lights based on changed Vira scene.
            */   
            void updateLights();
            
            /**
             * @brief Creates Vulkan camera from a ViewportCamera created from the Vira scene camera. Writes Vulkan Camera and optical efficiency to vulkan buffers.
             * 
             * @note Optical efficiency is written to a separate buffer than the other camera parameters because it is of type TSpectral and has a variable number of elements.
            */   
            void createCamera();
            void updateCamera();

            // Get Methods
            vk::DescriptorSet getGeometryDescriptorSet() {return geometryDescriptorSet;};
            const ViraDescriptorSetLayout* getGeometryDescriptorSetLayout() {
                return geometryDescriptorSetLayout.get();
            };
            std::array<vk::DescriptorSet, 7> getDescriptorSets() {return descriptorSets;};
            vk::TransformMatrixKHR getTransformMatrix(const Pose<TFloat>& pose);
            void getPhysicalDeviceProperties2();
            vk::PipelineLayout getPipelineLayout() {return pipelineLayout;};  
            vk::Pipeline& getPipeline() { return viraPipeline->getPipeline();};
            std::vector<vk::AccelerationStructureKHR> getVBLAS() {return vBLAS;};
            ViewportCamera<TSpectral, TFloat>* getCamera() {return viewportCamera;};
            vk::AccelerationStructureKHR getTLAS() {return tlas;};
            vk::AccelerationStructureKHR* getpTLAS() {return &tlas;};
            VkStridedDeviceAddressRegionKHR getRaygenSBT() {return raygenSBT;};
            Scene<TSpectral, TFloat, TMeshFloat>* getScene() {return scene;};
            VulkanRendererOptions getOptions() {return options;};

            // Public Member Variables

            VulkanCamera<TSpectral, TFloat>* vulkanCamera;
            uint32_t minStorageBufferOffsetAlignment;
            size_t nPixels;
            vk::AccelerationStructureBuildGeometryInfoKHR buildGeometryInfo;
            size_t nInstances;
            vk::Extent2D useExtent;
            size_t nSpectralSets;

            // Debug
            ViraBuffer* debugCounterBuffer;
            ViraBuffer* debugCounterBuffer2;
            ViraBuffer* debugCounterBuffer3;
            ViraBuffer* debugVec4Buffer;
            std::unique_ptr<ViraDescriptorSetLayout> debugDescriptorSetLayout; 

            // Descriptors
            std::unique_ptr<ViraDescriptorSetLayout> geometryDescriptorSetLayout;
            vk::DescriptorSet geometryDescriptorSet;
            std::array<vk::DescriptorSetLayout, 1> descriptorSetLayouts;
            std::unique_ptr<ViraDescriptorPool> debugDescriptorPool;
            vk::DescriptorSet debugDescriptorSet;

            // Shaders          
            ShaderBindingTableRegions sbtRegions;
            vk::UniqueShaderModule raygenShaderModule;
            vk::UniqueShaderModule missShaderModule;
            vk::UniqueShaderModule missShadowShaderModule;
            vk::UniqueShaderModule hitRadianceShaderModule;
            vk::UniqueShaderModule shadowHitShaderModule;
            
            std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
            ShaderBindingTableRegions getSBTRegions() {return sbtRegions;};
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
            ViraBuffer* sbtViraBuffer;
            VkStridedDeviceAddressRegionKHR raygenSBT;
            VkStridedDeviceAddressRegionKHR missSBT, hitSBT, callableSBT;
            
            // Alignments
            uint32_t minUniformBufferOffsetAlignment;
            uint32_t minIndexOffsetAlignment;
            uint32_t minAccelerationStructureScratchOffsetAlignment;
            uint32_t shaderGroupBaseAlignment;
            uint32_t shaderGroupHandleAlignment;
            uint32_t shaderGroupHandleSize;
            std::vector<uint8_t> shaderGroupHandles;

        void render(vira::cameras::Camera<TSpectral, TFloat>& camera, vira::Scene<TSpectral, TFloat, float>& Scene);

        private:

            /// Private Member Variables

            // General
            ViewportCamera<TSpectral, TFloat>* viewportCamera;
            Scene<TSpectral, TFloat, TMeshFloat>* scene;
            ViraDevice* viraDevice;
            ViraSwapchain* viraSwapchain;
            VulkanRendererOptions options;
            
            ViraPipeline<TSpectral, TMeshFloat>* viraPipeline;
            std::vector<VulkanLight<TSpectral, TFloat>> lights; 

            // Counts/Sizes
            vk::DeviceSize numBLAS;
            vk::DeviceSize numLights;
            vk::DeviceSize numMaterials;
            
            size_t nSpectralBands;
            size_t nSpectralPad;
            vk::DeviceSize lightDataSize;
            vk::DeviceSize instancesDataSize;

            // Camera
            vk::Extent2D extent; 
            ImageTexture pixelSolidAngleTexture;
            TSpectral opticalEfficiency;
            ViraBuffer* optEffBuffer;
            vk::ImageView outputImageView;            
            
            // Geometry
            std::vector<std::unique_ptr<BLASResources>> blasResources;
            std::vector<vk::AccelerationStructureKHR> vBLAS;
            ViraBuffer* albedoExtraChannelsBuffer;

            std::vector<vk::AccelerationStructureInstanceKHR> meshInstances;
            vk::AccelerationStructureGeometryKHR tlasGeometry;
            vk::AccelerationStructureKHR tlas;
            
            // Reusable Objects
            std::vector<VertexVec4<TSpectral, TMeshFloat>> vec4Vertices;
            std::vector<PackedVertex<TSpectral, TMeshFloat>> packedVertices;

            // Descriptor Sets & Pools , Pipeline
            std::array<vk::DescriptorSet, 7> descriptorSets;

            std::unique_ptr<ViraDescriptorSetLayout> meshDescriptorSetLayout;
            vk::DescriptorSet meshDescriptorSet;

            std::unique_ptr<ViraDescriptorSetLayout> cameraDescriptorSetLayout;
            vk::DescriptorSet cameraDescriptorSet;

            std::unique_ptr<ViraDescriptorSetLayout> materialDescriptorSetLayout;
            vk::DescriptorSet materialDescriptorSet;

            std::unique_ptr<ViraDescriptorSetLayout> lightDescriptorSetLayout;
            vk::DescriptorSet lightDescriptorSet;

            std::unique_ptr<ViraDescriptorSetLayout> imageDescriptorSetLayout;
            vk::DescriptorSet imageDescriptorSet;

            std::unique_ptr<ViraDescriptorPool> cameraDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> meshDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> materialDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> lightDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> geometryDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> imageDescriptorPool;

            vk::PipelineLayout pipelineLayout;  

            // Materials
            std::vector<ImageTexture> normalTextures; 
            std::vector<ImageTexture> roughnessTextures; 
            std::vector<ImageTexture> metalnessTextures; 
            std::vector<ImageTexture> transmissionTextures; 
            std::vector<ImageTexture> albedoTextures; 
            std::vector<ImageTexture> emissionTextures; 
            std::vector<std::unique_ptr<MemoryInterface>> textureMemoryArray; // Persistent storage for texture memory

            std::vector<MaterialInfo> materialInfos;
            std::vector<std::vector<uint32_t>> meshLinearMaterialIndices;
            std::vector<VulkanMaterial<TSpectral>> materials;
            std::vector<std::vector<std::shared_ptr<VulkanMaterial<TSpectral>>>> vMaterials;
            std::vector<uint8_t> materialOffsets;

            // Buffers
            ViraBuffer* lightBuffer;
            ViraBuffer* cameraBuffer;

            ViraBuffer* materialBuffer;
            ViraBuffer* materialOffsetsBuffer;

            std::vector<ViraBuffer*> vec4VertexViraBuffers;
            std::vector<vk::Buffer> vertexIndexBuffers;
            std::vector<vk::DescriptorBufferInfo> vertexBufferInfos;
            std::vector<vk::DescriptorBufferInfo> vertexIndicesBufferInfos;

            // Index Buffers for descriptor binding (flat and offsets)
            std::vector<uint32_t> indexVecFlat;
            std::vector<uint32_t> indexOffsets;

            ViraBuffer* instancesViraBuffer;
            ViraBuffer* scratchBLASBuffer;
            ViraBuffer* scratchTLASBuffer;
            ViraBuffer* indexBufferFlat;
            ViraBuffer* indexOffsetsBuffer;
          
            std::vector<ViraBuffer*> debugBuffers;

    };

};

#include "implementation/rendering/vulkan/vulkan_path_tracer.ipp"

#endif