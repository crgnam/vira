#ifndef VIRA_RENDERING_VULKAN_VULKAN_RENDER_RESOURCES_HPP
#define VIRA_RENDERING_VULKAN_VULKAN_RENDER_RESOURCES_HPP

#include <ranges>
#include <vector>
#include <cmath>
#include <cassert>
#include <iostream>
#include <type_traits>

#include "vulkan/vulkan.hpp"
#include "glm/glm.hpp"

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/materials/material.hpp"
#include "vira/rendering/vulkan_private/vulkan_interface/command_buffer/command_buffer_interface.hpp"

namespace vira::vulkan {

    /**
     * @brief Run specific options for a vulkan renderer
     * 
     * @details This struct carries any user defined or data defined options defining the specifics of the data and renderer 
     * Members:
     * - nSpectralBands: Number of bands (N) in the spectral class of the photometric data being used/rendered.
     * - nSamples: Number of rays sampled/generated for each pixel.
     * - numFrames: Number of frames to render.
     * - maxFramesInFlight: The maximum number of frames that can be in flight at once.
     * - smoothShading: Whether to enable smooth shading (vs flat shading).
     * - nRaygenShaders: Number of ray generation shaders used in this render.
     * - nMissShaders: Number of miss shaders used in this render.
     * - nHitShaders: Number of hit shaders used in this render.
     * - rayGenShader: Path to the compiled ray generation shader (SPIR-V).
     * - missRadianceShader: Path to the compiled miss shader (SPIR-V).
     * - hitRadianceShader: Path to the compiled closest-hit shader (SPIR-V).
     * - vertShader: Path to the vertex shader SPIR-V file.
     * - fragShader: Path to the fragment shader SPIR-V file.

     * 
     * @see VulkanPathTracer , VulkanRasterizer
     */
    struct VulkanRendererOptions {

        int nSpectralBands = 4;
        int nSamples = 1;
        int numFrames = 1;

        size_t maxFramesInFlight = 2;
        bool smoothShading = true;

        int nRaygenShaders = 1;
        int nMissShaders = 2;
        int nHitShaders = 2;

        const std::string rayGenShader = "./shaders/ray_generation.spv";
        const std::string missRadianceShader = "./shaders/miss_radiance.spv";
        const std::string missShadowShader = "./shaders/miss_shadow.spv";
        const std::string hitRadianceShader = "./shaders/hit_radiance.spv";
        const std::string shadowHitShader = "./shaders/hit_shadow.spv";

        const std::string& vertShader = "./shaders/pinhole.spv";
        const std::string& fragShader = "./shaders/mcewan.spv";

    };  

    enum RayType : uint32_t {
        RAY_TYPE_RADIANCE = 0,
        RAY_TYPE_SHADOW   = 1,
        RAY_TYPE_COUNT    = 2
    };
        
    /**
     * @brief Joins multiple std::vector<uint32_t> objects into one vector
     */       
    std::vector<uint32_t> joinVectors(const std::vector<std::vector<uint32_t>>& vectors) {
        std::vector<uint32_t> result;
        size_t totalSize = 0;
    
        // Precompute total size to avoid multiple reallocations
        for (const auto& vec : vectors) totalSize += vec.size();
        result.reserve(totalSize);
    
        // Append all vectors
        for (const auto& vec : vectors) {
            result.insert(result.end(), vec.begin(), vec.end());
        }
    
        return result;
    }
    
    /**
     * @brief Loads a shader model .spv file for use in a rendering pipeline
     */       
     vk::UniqueShaderModule loadShaderModule(const std::string& filename, vk::Device device, vk::DispatchLoaderDynamic* dldi) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open shader file: " + filename);
        }
    
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> shaderCode(fileSize / sizeof(uint32_t));
    
        file.seekg(0);
        file.read(reinterpret_cast<char*>(shaderCode.data()), fileSize);
        file.close();
    
        vk::ShaderModuleCreateInfo shaderCreateInfo{};
        shaderCreateInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
        shaderCreateInfo.pCode = shaderCode.data();
    
        return device.createShaderModuleUnique(shaderCreateInfo, nullptr, *dldi);
    }

    /**
     * @brief Splits a vector of TSpectral data into a flattened vectors of vec4<float>s.
     * 
     * This is used when nSpectralBands > 4, as vk::Images do not support arbitrary vector types.
     * This method takes a vector of TSpectral data and converts it to a vector of vec4 data.
     * If nSpectralSets > 1, the multiple vec4s that cover a spectral data type are contiguous.
     * If flagged, this will exclude the first spectral set of each data point (used for constructing
     * extra albedo channels, as first vec4 is stored in Vec4Vertex)
     */       
    template <IsSpectral TSpectral>
    std::vector<glm::vec4> splitSpectralToVec4s(
        const std::vector<TSpectral>& spectralData, 
        bool excludeFirst4 = false) 
    {
        size_t N = TSpectral::size();   // Number of spectral sets
        size_t M = spectralData.size(); // Number of vertices

        size_t totalChannels = excludeFirst4 ? std::max(N - 4, size_t(0)) : N;
        size_t nSets = static_cast<size_t>(std::ceil(totalChannels / 4.0));

        std::vector<glm::vec4> vec4s(M * nSets, glm::vec4(0.0f)); // Initialize with zero padding

        for (size_t vertexIdx = 0; vertexIdx < M; vertexIdx++) {
            int startChannel = excludeFirst4 ? 4 : 0;
            size_t vec4BaseIdx = vertexIdx * nSets; // Ensure contiguous storage per vertex

            for (int i = startChannel; i < static_cast<int>(N); i++) {
                int relativeIdx = i - startChannel; // Re-index for extra channels
                int vec4Index = static_cast<int>(vec4BaseIdx) + (relativeIdx / 4);
                int componentIndex = relativeIdx % 4;

                vec4s[vec4Index][componentIndex] = static_cast<float>(spectralData[vertexIdx][i]);
            }
        }

        return vec4s;
    }

    template <typename T>
    auto make_strided_view(const std::vector<T>& vec, size_t offset, size_t stride) {
        return std::views::iota(offset, vec.size())
             | std::views::filter([=](size_t i) { return (i - offset) % stride == 0; })
             | std::views::transform([&vec](size_t i) -> const T& {
                   return vec[i];
               });
    }

    /**
     * @brief Splits an Image<TSpectral> into a vector of Image<glm::vec4> that each hold the 
     * subsequent 4 channels from the previous one.
     * 
     * This is used when nSpectralBands > 4, as vk::Images do not support arbitrary vector types.
     * This method takes an Image<TSpectral> and converts it to a std::vector<Image<glm::vec4>>.
     * Each output image holds up to 4 channels of the original image.
     */       
    template <IsSpectral TSpectral>
    std::vector<Image<glm::vec4>> splitSpectralImageToVec4Images(
        const Image<TSpectral>& spectralImage) 
    {

        std::vector<Image<glm::vec4>> imagesOut;

        std::vector<TSpectral> spectralImageDataVector = spectralImage.getVector();

        std::vector<glm::vec4> splitDataVector = splitSpectralToVec4s(
            spectralImageDataVector, 
            false); 

        size_t N = (TSpectral::size() + 3) / 4;

        for (size_t spectralSet=0; spectralSet < N; ++spectralSet) {

            auto view = make_strided_view(splitDataVector, spectralSet, N);
            std::vector<glm::vec4> result(view.begin(), view.end());
            Image<glm::vec4> thisImage = Image(spectralImage.resolution(), result);
            imagesOut.push_back(thisImage);
        }

        return imagesOut;
    }   
    


    /**
     * @brief Reads data from file
     */
    std::vector<char> readFile(const std::string& filepath)
	{
		// We seek the end of the file immediately, and read it as a binary
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filepath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg()); // Because we are at the end of the file
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	};

    /**
     * @struct PushConstants
     * 
     * Defines some control parameters for binding to vulkan pipelines.
     * These are used by the vulkan shaders to define miscellaneous run
     * parameters
     */
    struct PushConstants {
       uint32_t nSpectralBands;
       uint32_t nLights;  
       uint32_t nSamples;    
    }; 

    /**
     * @brief Determines the format for a texture image based on the number of channels
     */       
    vk::Format determineFormat(uint32_t nChannels, bool highPrecision = false) {
        (void)highPrecision; // TODO Consider removing

        switch (nChannels) {
            case 1: return vk::Format::eR8Unorm;            // Single channel
            case 2: return vk::Format::eR8G8Unorm;          // Two channels
            case 3: 
                throw std::runtime_error("Vulkan does not support 3-channel formats directly. Use RGBA or pad the data.");
            // case 4: return vk::Format::eR8G8B8A8Srgb;       // Four channels
            case 4: return vk::Format::eR16G16B16A16Sfloat;       // Four channels

            default:
                throw std::runtime_error("Unsupported number of channels. Must be 1, 2, or 4.");
        }
    }

    /**
     * @brief Holds the model matrix and normal matrix that define a mesh/blas instance
     * in the vulkan scene.
     */
    struct InstanceData {
        glm::mat4 modelMatrix; // Instance transformation matrix
        glm::mat4 normalMatrix; // Instance transformation matrix
    };    

    /**
     * @brief PackedVertex buffer to build the BLAS from, simple structure for vulkan use
     */       
    template <IsFloat TMeshFloat, IsSpectral TSpectral>
    struct PackedVertex {
        vec3<TMeshFloat> position;  // 3 floats
        Normal normal;    // 3 floats
        UV uv;        // 2 floats

        PackedVertex(const Vertex<TSpectral, TMeshFloat>& vertex) {
            position = vertex.position;
            normal = vertex.normal;
            uv = vertex.uv;
        }

    };

    /**
     * @brief Vec4Vertex buffer for descriptor binding, inherits vira::Vertex.
     */    
    template <IsFloat TMeshFloat, IsSpectral TSpectral>
    struct alignas(16) VertexVec4 {
        vec3<TMeshFloat> position;              // 12 bytes
        float padding0;                         // 4 bytes (for 16-byte alignment)
        vec4<TMeshFloat> albedo{0, 0, 0, 0};    // 16 bytes (already 16-byte aligned)
        Normal normal;                          // 12 bytes, Vertex Normal
        float padding1;                         // 4 bytes (for 16-byte alignment)
        UV uv;                                  // 8 bytes, Vertex Texture Coordinates
        float padding2[2];                      // 8 bytes (ensuring total struct size is a multiple of 16)

        // Constructor
        VertexVec4(const Vertex<TSpectral, TMeshFloat>& vertex) {
            position = vertex.position;
            normal = vertex.normal;
            uv = vertex.uv;
            // Fill albedo from vertex.color with padding for missing channels
            for (int i = 0; i < 4; ++i) {
                if (i < vertex.albedo.size()) {
                    albedo[i] = static_cast<TMeshFloat>(vertex.albedo[i]);
                } else {
                    albedo[i] = 0;
                }
            }        
        }
        
        /**
         * @brief Returns vertex binding descriptions for vertex and instance buffers
         */
        static std::vector<vk::VertexInputBindingDescription>   getBindingDescriptions() {
            std::vector<vk::VertexInputBindingDescription> bindingDescriptions(2);

            // Binding 0: Vertex Buffer
            bindingDescriptions[0].setBinding(0); // Binding index 0
            bindingDescriptions[0].setStride(sizeof(VertexVec4<TSpectral, TMeshFloat>)); // Size of each vertex
            bindingDescriptions[0].setInputRate(vk::VertexInputRate::eVertex); // Per-vertex data

            // Binding 1: Instance Buffer
            bindingDescriptions[1].setBinding(1); // Binding index 1
            bindingDescriptions[1].setStride(sizeof(InstanceData)); // Size of each instance
            bindingDescriptions[1].setInputRate(vk::VertexInputRate::eInstance); // Per-instance data

            return bindingDescriptions;
        }

        /**
         * @brief Returns vertex attribute descriptions for the vertex and instance buffers
         */
        static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {

            std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = {
                // Attributes for binding 0 (vertex buffer)
                { 0, 0, vk::Format::eR32G32B32Sfloat,   __builtin_offsetof(VertexVec4<TSpectral, TMeshFloat>, position) }, 
                { 1, 0, vk::Format::eR32G32B32Sfloat,   __builtin_offsetof(VertexVec4<TSpectral, TMeshFloat>, albedo) },   
                { 2, 0, vk::Format::eR32G32B32Sfloat,   __builtin_offsetof(VertexVec4<TSpectral, TMeshFloat>, normal) },   
                { 3, 0, vk::Format::eR32G32Sfloat,      __builtin_offsetof(VertexVec4<TSpectral, TMeshFloat>, uv) },       

                // Attributes for binding 1 (instance buffer)
                { 4, 1, vk::Format::eR32G32B32A32Sfloat, 0 }, // mat4 column 0
                { 5, 1, vk::Format::eR32G32B32A32Sfloat, 16 }, // mat4 column 1
                { 6, 1, vk::Format::eR32G32B32A32Sfloat, 32 }, // mat4 column 2
                { 7, 1, vk::Format::eR32G32B32A32Sfloat, 48 }, // mat4 column 3
                { 8, 1, vk::Format::eR32G32B32A32Sfloat, 0 }, // mat4 column 0
                { 9, 1, vk::Format::eR32G32B32A32Sfloat, 16 }, // mat4 column 1
                { 10, 1, vk::Format::eR32G32B32A32Sfloat, 32 }, // mat4 column 2
                { 11, 1, vk::Format::eR32G32B32A32Sfloat, 48 }, // mat4 column 3
            };
            return attributeDescriptions;

        }
    };

    /**
     * @brief Provides struct to enable specializations for different templated classes
     * to return the number of channels.
     */
    template <typename T, typename Enable = void>
    struct NumChannels {
        static constexpr size_t value = 1;  // Default to 1 channel (e.g., float, int)
    };

    /**
     * @brief NumChannels Specialization for Spectral classes
     */     
    template <typename T>
    struct NumChannels<T, std::void_t<decltype(T::size())>> {
        static constexpr size_t value = T::size();
    };

    /**
     * @brief NumChannels Specialization for glm vector classes
     */      
    template <typename T> struct NumChannels<glm::vec2, T> { static constexpr size_t value = 2; };
    template <typename T> struct NumChannels<glm::vec3, T> { static constexpr size_t value = 3; };
    template <typename T> struct NumChannels<glm::vec4, T> { static constexpr size_t value = 4; };

    /**
     * @brief NumChannels helper variable template for convenience
     */
    template <typename T>
    constexpr size_t NumChannels_v = NumChannels<T>::value;

    /**
     * @brief Flattens a std::vector<TPix> of size N with TPix size M to a std::vector<float> of size N * M
     */      
    template <IsIndexable TPix>
    std::vector<float> flattenWithPadding(const std::vector<TPix>& pixels, float paddingValue = 0.0f) {

        constexpr size_t numChannels = NumChannels_v<TPix>;
        constexpr size_t paddedChannels = (numChannels == 3) ? 4 : numChannels;
    
        std::vector<float> flattenedData;
        flattenedData.reserve(pixels.size() * paddedChannels);
    
        for (size_t p = 0; p < pixels.size(); ++p) {

            TPix pixel = pixels[p];

            for (int i = 0; i < static_cast<int>(numChannels); ++i) {
                flattenedData.push_back(pixel[i]);
            }    

            if (numChannels == 3) {
                flattenedData.push_back(paddingValue); // Insert padding only for vec3
            }
        }
    
        return flattenedData;
    }
    
    /**
     * @brief Dummy Function to allow float data to be passed to flattenWithPadding without effect
     */      
    std::vector<float> flattenWithPadding(const std::vector<float>& pixels, float paddingValue = 0.0f) {
        (void)paddingValue; // TODO Consider removing

        std::vector<float> flattenedData;
        flattenedData.reserve(pixels.size());
    
        for (size_t p = 0; p < pixels.size(); ++p) {
            flattenedData.push_back( static_cast<float>(pixels[p]) );
        }
    
        return flattenedData;
    }

    /**
     * @brief converts a float to a uint8_t formated value
     */      
    inline uint8_t convertUNorm(float value) {
        return static_cast<uint8_t>(std::clamp(value * 255.0f, 0.0f, 255.0f));
    }

    /**
     * @brief converts a float to a int8_t formated value
     */      
    inline int8_t convertSNorm(float value) {
        return static_cast<int8_t>(std::clamp(value * 127.0f, -127.0f, 127.0f));
    }

    /**
     * @brief Unified function for UNorm and SNorm conversion
     */      
    inline int convertNormalized(float value, bool isSigned) {
        return isSigned ? convertSNorm(value) : convertUNorm(value);
    }

    /**
     * @brief Stores information about a Vulkan image format.
     *
     * @details This struct provides metadata for a given `vk::Format`, 
     * including the number of channels, byte size per channel, 
     * total pixel size, and whether the format is normalized or signed.
     */
    struct FormatInfo {
        uint32_t numChannels;   // Number of color/depth/stencil channels
        size_t bytesPerChannel; // Bytes per individual channel
        size_t bytesPerPixel;   // Total bytes per element (pixel)
        bool isNormalized;      // True if format is Unorm/Snorm (normalized integer)
        bool isSigned;          // True if format uses signed values (SInt, SNorm, Float)
    };

    // Unified function to get Vulkan format properties
    FormatInfo getFormatInfo(vk::Format format) {
        size_t bytesPerPixel = 0;
        uint32_t numChannels = 0;
        bool isNormalized = false;
        bool isSigned = false;

        switch (format) {
            //  4-Component Formats (RGBA)
            case vk::Format::eR8G8B8A8Unorm:
            case vk::Format::eR8G8B8A8Srgb:
            case vk::Format::eB8G8R8A8Unorm:
                bytesPerPixel = 4 * sizeof(uint8_t); numChannels = 4; isNormalized = true; break;
            case vk::Format::eR8G8B8A8Snorm:
                bytesPerPixel = 4 * sizeof(int8_t); numChannels = 4; isNormalized = true; isSigned = true; break;
            case vk::Format::eR8G8B8A8Uint:
                bytesPerPixel = 4 * sizeof(uint8_t); numChannels = 4; break;
            case vk::Format::eR8G8B8A8Sint:
                bytesPerPixel = 4 * sizeof(int8_t); numChannels = 4; isSigned = true; break;
            case vk::Format::eR16G16B16A16Sfloat:
                bytesPerPixel = 4 * sizeof(uint16_t); numChannels = 4; isSigned = true; break;
            case vk::Format::eR32G32B32A32Sfloat:
                bytesPerPixel = 4 * sizeof(float); numChannels = 4; isSigned = true; break;

            //  3-Component Formats (RGB) [Rare]
            case vk::Format::eR8G8B8Unorm:
            case vk::Format::eB8G8R8Unorm:
                bytesPerPixel = 3 * sizeof(uint8_t); numChannels = 3; isNormalized = true; break;
            case vk::Format::eR8G8B8Snorm:
                bytesPerPixel = 3 * sizeof(int8_t); numChannels = 3; isNormalized = true; isSigned = true; break;
            case vk::Format::eR32G32B32Sfloat:
                bytesPerPixel = 3 * sizeof(float); numChannels = 3; isSigned = true; break;

            //  2-Component Formats (RG)
            case vk::Format::eR8G8Unorm:
                bytesPerPixel = 2 * sizeof(uint8_t); numChannels = 2; isNormalized = true; break;
            case vk::Format::eR8G8Snorm:
                bytesPerPixel = 2 * sizeof(int8_t); numChannels = 2; isNormalized = true; isSigned = true; break;
            case vk::Format::eR8G8Uint:
                bytesPerPixel = 2 * sizeof(uint8_t); numChannels = 2; break;
            case vk::Format::eR8G8Sint:
                bytesPerPixel = 2 * sizeof(int8_t); numChannels = 2; isSigned = true; break;
            case vk::Format::eR16G16Sfloat:
                bytesPerPixel = 2 * sizeof(uint16_t); numChannels = 2; isSigned = true; break;
            case vk::Format::eR32G32Sfloat:
                bytesPerPixel = 2 * sizeof(float); numChannels = 2; isSigned = true; break;

            //  1-Component Formats (R)
            case vk::Format::eR8Unorm:
                bytesPerPixel = sizeof(uint8_t); numChannels = 1; isNormalized = true; break;
            case vk::Format::eR8Snorm:
                bytesPerPixel = sizeof(int8_t); numChannels = 1; isNormalized = true; isSigned = true; break;
            case vk::Format::eR8Uint:
                bytesPerPixel = sizeof(uint8_t); numChannels = 1; break;
            case vk::Format::eR8Sint:
                bytesPerPixel = sizeof(int8_t); numChannels = 1; isSigned = true; break;
            case vk::Format::eR16Sfloat:
                bytesPerPixel = sizeof(uint16_t); numChannels = 1; isSigned = true; break;
            case vk::Format::eR32Sfloat:
                bytesPerPixel = sizeof(float); numChannels = 1; isSigned = true; break;

            //  Depth Formats (1-Channel)
            case vk::Format::eD32Sfloat:
                bytesPerPixel = sizeof(float); numChannels = 1; isSigned = true; break;
            case vk::Format::eD16Unorm:
                bytesPerPixel = sizeof(uint16_t); numChannels = 1; isNormalized = true; break;

            //  Depth+Stencil Formats (2-Channel)
            case vk::Format::eD24UnormS8Uint:
            case vk::Format::eD32SfloatS8Uint:
                bytesPerPixel = 4; numChannels = 2; isNormalized = true; break;
            default:
                bytesPerPixel = 0; numChannels = 0; isNormalized = false; isSigned = false; // Unknown format
        }

        // Compute bytes per channel, avoid division by zero
        size_t bytesPerChannel = numChannels ? bytesPerPixel / numChannels : 0;

        return {numChannels, bytesPerChannel, bytesPerPixel, isNormalized, isSigned};
    }

    /**
     * @brief Holds material properties related to shading and sampling.
     *
     * @details Contains data about the Bidirectional Scattering Distribution Function (BSDF)
     * type and the sampling method used for rendering.
     */
    struct MaterialInfo {
        int bsdfType; 
        vira::SamplingMethod samplingMethod;    
    };

    /**
     * @brief Represents an image-based texture in Vulkan.
     *
     * @details Stores Vulkan objects required for texture sampling, 
     * including an image, image view, and sampler.
     */
     struct ImageTexture {
        vk::Image image;
        vk::ImageView imageView;
        vk::Sampler sampler;
    };

     /**
     * @brief Represents a buffer-based texture in Vulkan.
     *
     * @details Stores a Vulkan buffer and an associated buffer view, 
     * allowing textures to be stored in a buffer rather than an image.
     */
     struct BufferTexture {
        vk::Buffer buffer;
        vk::BufferView bufferView;
    };
    
    /**
     * @brief Stores configuration information for a Vulkan texture image.
     *
     * @details Defines the image creation parameters such as format, 
     * dimensions, tiling mode, and usage flags.
     */
    struct TextureImageInfo {
        vk::ImageCreateInfo info;

        TextureImageInfo(uint32_t textureWidth, uint32_t textureHeight,  vk::Format format) {
            info.imageType = vk::ImageType::e2D;
            info.extent.width = textureWidth;           // Width of the texture
            info.extent.height = textureHeight;         // Height of the texture
            info.extent.depth = 1;                      // Always 1 for 2D textures
            info.mipLevels = 1;                         // No mipmaps for simplicity
            info.arrayLayers = 1;                       // Single-layer image
            info.format = format;                       // Texture format (e.g., RGBA)
            info.tiling = vk::ImageTiling::eOptimal;    // Optimal for GPU access
            info.initialLayout = vk::ImageLayout::eUndefined;
            info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            info.samples = vk::SampleCountFlagBits::e1;
            info.sharingMode = vk::SharingMode::eExclusive;
        }
    };

    /**
     * @brief Defines image view parameters for a Vulkan texture.
     *
     * @details Specifies how a Vulkan image is accessed, including view type, 
     * format, and aspect mask for sampling.
     */
    struct TextureViewInfo {
        vk::ImageViewCreateInfo info;

        TextureViewInfo(vk::Image textureImage, vk::Format format) {
            info.image = textureImage;              // Vulkan image
            info.viewType = vk::ImageViewType::e2D; // 2D texture
            info.format = format;                   // Format determined by nChannels
            info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;   // Single mip level
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;   // Single layer
        }
    };  

    /**
     * @brief Stores configuration parameters for a Vulkan sampler.
     *
     * @details Defines how textures are sampled in shaders, 
     * including filtering modes, mipmap behavior, and address modes.
     */
    struct SamplerInfo {
        vk::SamplerCreateInfo info;

        SamplerInfo() {
            info.magFilter = vk::Filter::eLinear;                           // Linear filtering for magnification
            info.minFilter = vk::Filter::eLinear;                           // Linear filtering for minification
            info.mipmapMode = vk::SamplerMipmapMode::eNearest;              // Linear mipmap filtering
            info.addressModeU = vk::SamplerAddressMode::eClampToEdge;       // Repeat texture in U direction
            info.addressModeV = vk::SamplerAddressMode::eClampToEdge;       // Repeat texture in V direction
            info.addressModeW = vk::SamplerAddressMode::eClampToEdge;       // Repeat texture in W direction
            info.mipLodBias = 0.0f;                                         // No LOD bias
            info.anisotropyEnable = VK_TRUE;                                // Enable anisotropic filtering
            info.maxAnisotropy = 16.0f;                                     // Maximum anisotropy level
            info.minLod = 0.0f;                                             // Minimum LOD
            info.maxLod = VK_LOD_CLAMP_NONE;                                // No maximum LOD clamp
            info.borderColor = vk::BorderColor::eIntOpaqueBlack;            // Border color for clamp modes
            info.unnormalizedCoordinates = VK_FALSE;                        // Use normalized texture coordinates
        }
    }; 

    /**
     * @brief Holds associated objects and parameters for a render frame 
     *
     * @details Defines the command buffer, camera, and frame index/time
     */    
    template <IsSpectral TSpectral, IsFloat TFloat>
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		CommandBufferInterface* commandBuffer;
		ViewportCamera<TSpectral, TFloat>& camera;
		vk::DescriptorSet globalDescriptorSet;
	};    

}      
#endif
