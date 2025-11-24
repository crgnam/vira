#ifndef VULKAN_MATERIAL_HPP
#define VULKAN_MATERIAL_HPP

#include "vulkan/vulkan.hpp"
#include "glm/vec3.hpp"

#include "vira/rendering/vulkan/viewport_camera.hpp"
#include "vira/math.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/materials/material.hpp"
#include "vira/images/image_utils.hpp"

namespace vira::vulkan {

   
    /**
     * @struct VulkanMaterial 
     * @brief Holds material parameters and textures in format writable to vulkan buffer
	 * 
     *
     * @details This struct stores various texture maps and parameters used 
     * for shading calculations, including albedo, normal, roughness, 
     * metalness, and transmission properties.
     *
     * ### Member Variables:
     * - **Albedo Map (`albedoMap`)**: Spectral reflectance of the surface.
     * - **Normal Map (`normalMap`)**: Surface normals in tangent space.
     * - **Roughness Map (`roughnessMap`)**: Defines surface microfacet roughness.
     * - **Metalness Map (`metalnessMap`)**: Indicates metallic vs. dielectric material.
     * - **Transmission Map (`transmissionMap`)**: Controls transparency or transmissivity.
     * - **Emission Map (`emissionMap`)**: Defines self-emission color and intensity.
     * - **BSDF Type (`bsdfType`)**: Specifies the type of BRDF/BSDF used for shading.
     * - **Sampling Method (`samplingMethod`)**: Defines how samples are distributed for rendering.\
     * @see Material
     */
    
    template <IsSpectral TSpectral>
    struct VulkanMaterial {

        Image<TSpectral> albedoMap;
        Image<vec4<float>> normalMap;
        Image<float> roughnessMap;
        Image<float> metalnessMap;
        Image<TSpectral> transmissionMap;
        Image<TSpectral> emissionMap;
        int bsdfType; 
        vira::SamplingMethod samplingMethod;

        // Constructor from vira::Material
        VulkanMaterial(vira::Material<TSpectral>* material) {
            resetFromMaterial(material);
        }

        /**
         * @brief Fills struct with values from a vira::Material<TSpectral>
         */   
        void resetFromMaterial(vira::Material<TSpectral>* material) {        

            albedoMap = material->getAlbedoMap();
            emissionMap = material->getEmissionMap();

            Image<Normal> inputNormalMap = material->getNormalMap();
            std::vector<Normal> normalVec = inputNormalMap.getVector();
            std::vector<vec4<float>> paddedNormalVec(normalVec.size(), vec4<float>(0.0f));
            for(size_t i = 0; i < normalVec.size(); ++i) {
                for(int j = 0; j < 3; ++j) {
                    paddedNormalVec[i][j] = normalVec[i][j]*0.5f + 0.5f;
                }
            }
            normalMap = Image(inputNormalMap.resolution(), paddedNormalVec);

            roughnessMap = material->getRoughnessMap();
            metalnessMap = material->getMetalnessMap();
            transmissionMap = material->getTransmissionMap();

            bsdfType = material->getBSDFID();  // Access bsdfType for the selected material
            samplingMethod = material->getSamplingMethod();

        }

    };       
}

#endif // VULKAN_MATERIAL_HPP