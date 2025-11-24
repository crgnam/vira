#ifndef VULKAN_LIGHT_HPP
#define VULKAN_LIGHT_HPP

#include "vulkan/vulkan.hpp"
#include "glm/vec3.hpp"

#include "vira/lights/light.hpp"
#include "vira/lights/sphere_light.hpp"

namespace vira::vulkan {

    /**
     * @struct VulkanLight 
     * @brief Holds light parameters in format writable to vulkan buffer
     * 
     * ### Member Variables:
     * - **Position (`position`)**: World-space position of the light.
     * - **Type (`type`)**: Light type identifier (`0 = point light`, `1 = spherical light`).
     * - **Color (`color`)**: Spectral intensity or radiance of the light (for point lights).
     * - **Radius (`radius`)**: Radius of a spherical light (`0` for point lights).
     * 
	 * @see Light
	 * 
     */	
    template <typename TFloat, typename TSpectral>        
    struct VulkanLight {
    
        alignas(16) vec3<TFloat> position;      // Position of the light 12 Bytes, aligned to 16
        alignas(4) float type;                  // Light type identifier (0 = point, 1 = sphere) 4 Bytes, aligned to 4, completes block of 16 with position

        alignas(16) vec4<TFloat> color{1};      // Spectral intensity or radiance of the light for point lights
        alignas(4) float radius;                // Radius for spherical lights (0 for point lights)

        // Constructor to initialize VulkanLight from a scene Light
        VulkanLight(Light<TSpectral, TFloat>* light) {

            resetFromLight(light);
        }

        /**
         * @brief Fills struct with values from a vira::Light<TSpectral, TFloat>
         */   
        void resetFromLight(Light<TSpectral, TFloat>* light) {

            // Set the light type based on the Light instance
            position = light->getPosition();  // Assuming getPosition() returns glm::vec3
            type = TFloat(light->getType());
            radius = light->getRadius();  // Sphere light radius

            TSpectral spectralOutput;
            if (light->getType() == POINT_LIGHT) {
                spectralOutput = light->getSpectralIntensity();
            } else if (light->getType() == SPHERE_LIGHT) {
                spectralOutput = light->getSpectralRadiance();
            }

            for (int i = 0; i < 3; ++i) {
                color[i] = spectralOutput.values[i];
            }

        }
    };
}

#endif // VULKAN_LIGHT_HPP