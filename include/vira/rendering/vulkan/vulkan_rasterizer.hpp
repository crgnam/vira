#ifndef VIRA_RENDERING_VULKAN_VULKAN_RASTERIZER_HPP
#define VIRA_RENDERING_VULKAN_VULKAN_RASTERIZER_HPP

#include "vulkan/vulkan.hpp"

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/images/image.hpp"
#include "vira/scene.hpp"

#include "vira/rendering/vulkan_private/device.hpp"
#include "vira/rendering/vulkan_private/render_loop.hpp"
#include "vira/rendering/vulkan_private/pipeline.hpp"
#include "vira/rendering/vulkan_private/buffer.hpp"
#include "vira/rendering/vulkan_private/descriptors.hpp"
#include "vira/rendering/vulkan_private/vulkan_render_resources.hpp"
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
     * @brief Aggregates Vulkan buffer and memory resources associated with a mesh.
     *
     * Members:
     * - vertexBuffer: Unique Vulkan buffer for vertex data.
     * - indexBuffer: Unique Vulkan buffer for index data.
     * - instancesBuffer: Unique Vulkan buffer for per-instance data.
     * - vertexMemory: Device memory backing the vertex buffer.
     * - indexMemory: Device memory backing the index buffer.
     * - instancesMemory: Device memory backing the instance buffer.
     * - nInstances: Number of mesh instances.
     * - indexCount: Number of indices in the mesh.
     */
    struct MeshResources {
        vk::UniqueBuffer vertexBuffer;                            // Unique Vulkan buffer for vertices
        vk::UniqueBuffer indexBuffer;                             // Unique Vulkan buffer for indices
        vk::UniqueBuffer instancesBuffer;                         // Vira buffer for instances data

        vk::UniqueDeviceMemory vertexMemory;                      // Device memory for vertex buffer
        vk::UniqueDeviceMemory indexMemory;                       // Device memory for index buffer       
        vk::UniqueDeviceMemory instancesMemory;                       // Device memory for index buffer       

        size_t nInstances = 0;
        size_t indexCount = 0;


        // Constructor
        MeshResources(
            vk::UniqueBuffer vBuffer,
            vk::UniqueBuffer iBuffer,
            vk::UniqueDeviceMemory vMemory,
            vk::UniqueDeviceMemory iMemory //,
        ) 
        : 
        vertexBuffer{std::move(vBuffer)},
        indexBuffer{std::move(iBuffer)},

        vertexMemory{std::move(vMemory)},
        indexMemory{std::move(iMemory)} {}; 

        // Disable copying
        MeshResources(const MeshResources&) = delete;
        MeshResources& operator=(const MeshResources&) = delete;

        // Enable moving
        MeshResources(MeshResources&&) noexcept = default;
        MeshResources& operator=(MeshResources&&) noexcept = default;
        
        // Get Methods
        vk::Buffer& getVertexBuffer() {return vertexBuffer.get();}
        vk::Buffer& getIndexBuffer() {return indexBuffer.get();}
        vk::Buffer& getInstancesBuffer() {return instancesBuffer.get();}
    };


    /**
     * @class VulkanRasterizer
     * @brief Performs Vulkan GPU Rasterization Rendering
     *
     * @details The VulkanRasterizer class creates and manages all vulkan gpu related resources, 
     * as well as records vulkan rendering commands to command buffers.
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
     * - `uint32_t minStorageBufferOffsetAlignment` : Minimum required storage buffer offset alignment.
     * - `uint32_t minUniformBufferOffsetAlignment` : Minimum alignment for uniform buffers.
     * - `uint32_t minIndexOffsetAlignment` : Minimum index offset alignment.
     * - `uint32_t vertexCount` : Total number of vertices in the scene.
     * - `std::vector<std::unique_ptr<MeshResources>> meshResources` : List of mesh resources.
     * - `std::vector<ViraBuffer*> debugBuffers` : Buffers used for debugging.
     * - `size_t nPixels` : Total number of pixels in the scene.
     * - `size_t nSpectralSets` : Number of spectral images used.
     *
     * ### Private:
     * - `VulkanRendererOptions options` : Rasterizer configuration options.
     * - `ViraDevice* viraDevice` : Pointer to the Vulkan device.
     * - `ViraPipeline<TSpectral, TMeshFloat>* viraPipeline` : Pointer to the rendering pipeline.
     * - `ViraSwapchain* viraSwapchain` : Pointer to the swapchain.
     * - `Scene<TSpectral, TFloat, TMeshFloat>* scene` : Pointer to the scene being rendered.
     * - `vk::Extent2D extent` : The rendering resolution.
     * - `ViewportCamera<TSpectral, TFloat>* viewportCamera` : Pointer to the viewport camera.
     * - `VulkanCamera<TSpectral, TFloat>* vulkanCamera` : Pointer to the Vulkan camera instance.
     * - `ImageTexture pixelSolidAngleTexture` : Texture for pixel solid angles.
     * - `TSpectral opticalEfficiency` : Optical efficiency factor.
     *
     * #### Lighting & Geometry:
     * - `std::vector<VulkanLight<TSpectral, TFloat>> lights` : List of lights in the scene.
     * - `std::vector<std::vector<InstanceData>> meshInstances` : List of mesh instance data.
     * - `std::vector<vk::DescriptorBufferInfo> vertexBufferInfos` : Buffer info for vertex buffers.
     * - `std::vector<vk::DescriptorBufferInfo> vertexIndicesBufferInfos` : Buffer info for vertex indices.
     * - `std::vector<vk::DescriptorBufferInfo> albedoBufferInfos` : Buffer info for albedo textures.
     * - `std::vector<vk::DescriptorBufferInfo> instancesBufferInfos` : Buffer info for instance data.
     *
     * #### Materials:
     * - `std::vector<VulkanMaterial<TSpectral>> materials` : List of materials used in the scene.
     * - `std::vector<MaterialInfo> materialInfos` : Material information list.
     * - `std::vector<ImageTexture> normalTextures` : List of normal textures.
     * - `std::vector<ImageTexture> roughnessTextures` : List of roughness textures.
     * - `std::vector<ImageTexture> metalnessTextures` : List of metalness textures.
     * - `std::vector<ImageTexture> transmissionTextures` : List of transmission textures.
     * - `std::vector<ImageTexture> albedoTextures` : List of albedo textures.
     * - `std::vector<ImageTexture> emissionTextures` : List of emission textures.
     * - `std::vector<std::unique_ptr<MemoryInterface>> textureMemoryArray` : Persistent storage for texture memory.
     *
     * #### Descriptor Sets & Pools:
     * - `std::vector<vk::DescriptorSet> meshDescriptorSets` : Descriptor sets for meshes.
     * - `vk::DescriptorSet cameraDescriptorSet` : Descriptor set for camera data.
     * - `vk::DescriptorSet materialDescriptorSet` : Descriptor set for material data.
     * - `vk::DescriptorSet lightDescriptorSet` : Descriptor set for lighting data.
     * - `vk::DescriptorSet debugDescriptorSet` : Descriptor set for debugging.
     * - `std::unique_ptr<ViraDescriptorPool> cameraDescriptorPool` : Camera descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> meshDescriptorPool` : Mesh descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> materialDescriptorPool` : Material descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> lightDescriptorPool` : Light descriptor pool.
     * - `std::unique_ptr<ViraDescriptorPool> debugDescriptorPool` : Debug descriptor pool.
     * - `vk::PipelineLayout pipelineLayout` : Pipeline layout for rasterization.
     *
     * #### Buffers:
     * - `ViraBuffer* lightBuffer` : Buffer storing light data.
     * - `ViraBuffer* cameraBuffer` : Buffer storing camera data.
     * - `ViraBuffer* optEffBuffer` : Buffer for optical efficiency data.
     * - `ViraBuffer* albedoExtraChannelsBuffer` : Buffer for additional albedo channels.
     * - `std::vector<ViraBuffer*> albedoExtraChannelsBuffers` : Vector of buffers for additional albedo channels.
     * - `ViraBuffer* materialBuffer` : Buffer storing material data.
     * - `ViraBuffer* materialOffsetBuffer` : Buffer storing material offsets.
     * - `ViraBuffer* materialIndexBuffer` : Buffer storing material indices.
     * - `std::vector<ViraBuffer*> matOffsetBuffers` : Vector of buffers for material offsets.
     * - `std::vector<ViraBuffer*> matIndexBuffers` : Vector of buffers for material indices.
     *
     * #### Counts & Sizes:
     * - `size_t nSpectralBands` : Number of spectral bands in rendering.
     * - `size_t nSpectralPad` : Padding for spectral data.
     * - `size_t nSpectralSets` : Number of spectral sets.
     * - `vk::DeviceSize numMesh` : Number of meshes in the scene.
     * - `vk::DeviceSize numLights` : Number of lights in the scene.
     * - `vk::DeviceSize numMaterials` : Number of materials used.
     * - `vk::DeviceSize lightDataSize` : Total size of light data.
     * - `vk::DeviceSize instancesDataSize` : Total size of instance data.
     * 
     * @note Currently this class does not support color data with a spectral number greater than 4. In order
     * to develop this capability, all spectral data should be split into bins of 4 bands and represented
     * by vec4 arrays. In the shader, this bin indexing needs to be managed, and on the CPU side these vec4s
     * need to be rejoined into spectral data classes.
     * It would also be appropriate to create a VulkanRenderer parent class for the VulkanPathTracer and VulkanRasterizer
     * as they share a good deal of functionality. 
     *
     * @see VulkanRenderLoop, CPURasterizer, VulkanCamera
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class VulkanRasterizer {

        public:

            VulkanRasterizer( ViewportCamera<TSpectral, TFloat>* viewportCamera, 
                              Scene<TSpectral, TFloat, TMeshFloat>* scene, 
                              ViraDevice* viraDevice, 
                              ViraSwapchain* viraSwapchain, 
                              VulkanRendererOptions options = VulkanRendererOptions{} );

            /**
             * @brief Builds vulkan scene and pipeline
             * 
             * This method calls other methods to build the vulkan scene, including the light sources, the camera, and the scene target objects. 
             * This method also calls other methods to create the vulkan render pipeline and the descriptor bindings, 
             * loads shaders, and binds them and the render pass to the pipeline.
             */  
            void initialize();

            /**
             * @brief Queries Vulkan for the physical device properties
             *
             */  
            void getPhysicalDeviceProperties2();

            /**
             * @brief Creates rasterizer pipeline and binds shaders to it
             *
             * This method creates the vulkan render pipeline, loads shaders and binds them and the render pass to the pipeline.
             */  
            void setupRasterizationPipeline();

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
             *  Mesh Resources (material indexing/offsets)
             *  Debug Buffers  
             *      Buffers to store debug information from shaders
             * 
             * Descriptor set bindings make these rendering resources available to use in the pipeline for both reading and writing data.
             * @throws ExceptionType Description if the method can throw.
             * @note Any additional information or limitations.
             * @warning Any important warnings or edge cases.
             * @see RelatedClass or RelatedMethod
             */            
            void setupDescriptors();
 
            /**
             * @brief Calls methods to update the vulkan scene geometries, lights, and camera following a 
             * Vira scene update.
             */
            void updateVulkanScene();
 
            /**
             * @brief Calls createMesh and update Instances to create vertex, index, and instance buffers for the vulkan scene
             */
            void createGeometries();

            /**
             * @brief Calls createMesh and update Instances to create vertex, index, and instance buffers for the vulkan scene
             */
            void updateGeometries();

            /**
             * @brief Creates Vertex and Index Buffers for a given Vira mesh
             */
            std::unique_ptr<MeshResources>  createMesh(Mesh<TSpectral, TFloat, TMeshFloat>& mesh);

            /**
             * @brief Creates/Updates Instances Buffer. Is called after a Vira Scene update
             * To update the global poses of the mesh instances.
             */
            void updateInstances(size_t meshIndex, const std::vector<Pose<TFloat>>& meshGPsUpdate);

            /**
             * @brief Creates VulkanLights for the Vulkan scene
             */
            void createLights();

            /**
             * @brief Retrieves updated Vira Lights and rewrites VulkanLights with updated values
             */
            void updateLights();

            /**
             * @brief Creates VulkanCamera Buffer and Optical Efficiency Buffer
             */
            void createCamera();

            /**
             * @brief Retrieves Vira Camera updated parameters and updates VulkanCamera / Camera Buffer
             */
            void updateCamera();

            /**
             * @brief Creates the material buffer, including texture arrays, for each mesh instance
             */
            void createMaterials();

            /**
             * @brief Records commands to draw mesh instances to render scene
             */
            void recordDrawCommands(vk::CommandBuffer commandBuffer);

            /**
             * @brief Creates a texture image that can be used for texture
             * sampling in the vulkan shaders when bound to a descriptor
             */
            template <IsPixel TImage>
            ImageTexture createTextureImage(Image<TImage> image, 
                                            vk::Format inputFormat = vk::Format::eUndefined, 
                                            bool useNearestTexel = false );  

            // =======================
            // Get Methods
            // =======================
            vk::TransformMatrixKHR getTransformMatrix(const Pose<TFloat>& pose);
            ViewportCamera<TSpectral, TFloat>* getCamera() {return viewportCamera;};
            Scene<TSpectral, TFloat, TMeshFloat>* getScene() {return scene;};
            VulkanRendererOptions getOptions() {return options;};

            
            // =======================
            // Alignments
            // =======================
            uint32_t minStorageBufferOffsetAlignment;
            uint32_t minUniformBufferOffsetAlignment;
            uint32_t minIndexOffsetAlignment;

            std::vector<std::unique_ptr<MeshResources>> meshResources;  
            std::vector<ViraBuffer*> debugBuffers;

            size_t nPixels;
            size_t nSpectralSets;

        void render(vira::cameras::Camera<TSpectral, TFloat>& camera, vira::Scene<TSpectral, TFloat, float>& Scene);

        private:
            
            ViewportCamera<TSpectral, TFloat>* viewportCamera;
            Scene<TSpectral, TFloat, TMeshFloat>* scene;
            ViraDevice* viraDevice;
            ViraSwapchain* viraSwapchain;
            VulkanRendererOptions options;

            // =======================
            // Vulkan Helper Objects and Options
            // =======================
            ViraPipeline<TSpectral, TMeshFloat>* viraPipeline;
            

            // =======================
            // Vulkan Scene
            // =======================
            std::vector<VulkanLight<TSpectral, TFloat>> lights;   
            std::vector<std::vector<InstanceData>> meshInstances;
            std::vector<vk::DescriptorBufferInfo> vertexBufferInfos;
            std::vector<vk::DescriptorBufferInfo> vertexIndicesBufferInfos;
            std::vector<vk::DescriptorBufferInfo> albedoBufferInfos;
            std::vector<vk::DescriptorBufferInfo> instancesBufferInfos;

            // =======================
            // Vulkan Camera
            // =======================
            vk::Extent2D extent; 
            VulkanCamera<TSpectral, TFloat>* vulkanCamera;
            ImageTexture pixelSolidAngleTexture;
            TSpectral opticalEfficiency;


            // =======================
            // Materials
            // =======================
            std::vector<VulkanMaterial<TSpectral>> materials;
            std::vector<MaterialInfo> materialInfos;

            std::vector<ImageTexture> normalTextures;
            std::vector<ImageTexture> roughnessTextures; 
            std::vector<ImageTexture> metalnessTextures; 
            std::vector<ImageTexture> transmissionTextures; 
            std::vector<ImageTexture> albedoTextures; 
            std::vector<ImageTexture> emissionTextures; 
            std::vector<std::unique_ptr<MemoryInterface>> textureMemoryArray; // Persistent storage for texture memory


            // =======================
            // Descriptor Sets
            // =======================
            std::vector<vk::DescriptorSet> meshDescriptorSets;
            vk::DescriptorSet cameraDescriptorSet;
            vk::DescriptorSet materialDescriptorSet;
            vk::DescriptorSet lightDescriptorSet;
            vk::DescriptorSet debugDescriptorSet;

            std::unique_ptr<ViraDescriptorPool> cameraDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> meshDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> materialDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> lightDescriptorPool;
            std::unique_ptr<ViraDescriptorPool> debugDescriptorPool;

            vk::PipelineLayout pipelineLayout;

            // =======================
            // Buffers
            // =======================
            /// Scene Object
            ViraBuffer* lightBuffer;
            ViraBuffer* cameraBuffer;
            ViraBuffer* optEffBuffer;

            /// Outputs
            ViraBuffer* albedoExtraChannelsBuffer;
            std::vector<ViraBuffer*> albedoExtraChannelsBuffers;

            /// Materials
            ViraBuffer* materialBuffer;
            ViraBuffer* materialOffsetBuffer;
            ViraBuffer* materialIndexBuffer;
            std::vector<ViraBuffer*> matOffsetBuffers;
            std::vector<ViraBuffer*> matIndexBuffers;

            // =======================
            // Counts/Sizes
            // =======================
            size_t nSpectralBands;
            size_t nSpectralPad;

            vk::DeviceSize numMesh;
            vk::DeviceSize numLights;
            vk::DeviceSize numMaterials;
            vk::DeviceSize lightDataSize;
            vk::DeviceSize instancesDataSize;

    };

};

#include "implementation/rendering/vulkan/vulkan_rasterizer.ipp"

#endif