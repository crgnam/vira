#ifndef VIRA_RENDERING_VULKAN_VIEWPORT_CAMERA_HPP
#define VIRA_RENDERING_VULKAN_VIEWPORT_CAMERA_HPP

#include <filesystem>

#include "vulkan/vulkan.hpp"

#include "vira/math.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/cameras/distortion_models/opencv_distortion.hpp"

namespace vira {

    /**
     * @class ViewportCamera
     * @brief Extends a Vira Camera for Vulkan usage.
     * 
     * Extends a Vira Camera to provide additional set/get methods and camera parameters. 
     * Defines the near and far planes used in the vulkan pipeline. 
     * Collects sensor data and other miscellaneous parameters to be written to a VulkanCamera object for use in the Vulkan pipeline.
     *
     *
     * @tparam TFloat The float type used by Vira, which obeys the "IsFloat" concept.
     * @tparam TSpectral The spectral type of light used in the render, such as the material albedos, the light irradiance, etc.
     * This parameter obeys the "IsSpectral" concept.
     * @param[in] cameraConfig (CameraConfig<TSpectral, TFloat>) Configuration struct defining camera parameters.
     * 
     *
     * @see Camera, VulkanCamera
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class ViewportCamera : public vira::Camera<TSpectral, TFloat> {
    public:
        ViewportCamera(CameraConfig<TSpectral, TFloat> cameraConfig) : Camera<TSpectral, TFloat>(cameraConfig) {};

        // Set/Get Methods
        void setNearPlane(TFloat newNear) { nearPlane = newNear; };
        const TFloat getNearPlane() { return nearPlane; };

        void setFarPlane(TFloat newFar) { farPlane = newFar; };
        const TFloat getFarPlane() { return farPlane; };

        Sensor<TSpectral, TFloat> getSensor() { return this->sensor; };

        DistortionCoefficients* getDistortionCoefficients() {
            return this->getDistortion()->getCoefficients();
        };
        std::string getDistortionType() { return this->getDistortion()->getType(); };

        mat4<TFloat> getProjectionMatrix() {
            mat4<TFloat> projectionMatrix{ 0.0f };

            // Intrinsic parameters from K
            const TFloat fx = this->K[0][0];  // Focal length in x
            const TFloat fy = this->K[1][1];  // Focal length in y
            const TFloat px = this->K[2][0];  // Principal point x
            const TFloat py = this->K[2][1];  // Principal point y

            // Construct projection matrix (column-major order)
            projectionMatrix[0][0] = 2 * fx / this->resolution.x;   // Scale x by focal length and resolution
            projectionMatrix[1][1] = 2 * fy / this->resolution.y;   // Scale y by focal length and resolution
            projectionMatrix[2][0] = 1 - 2 * px / this->resolution.x; // Translate x to normalized device coordinates
            projectionMatrix[2][1] = 1 - 2 * py / this->resolution.y; // Translate y to normalized device coordinates
            projectionMatrix[2][2] = 1.0; // Perspective depth scaling
            projectionMatrix[2][3] = 1.0; // Set perspective transform
            projectionMatrix[3][2] = 0.0; // Perspective depth offset

            return projectionMatrix;
        }        

    private:
        const TFloat nearPlane = 0.1f; // Example near plane (can be adjusted)
        const TFloat farPlane = 1.0e8; // Example far plane (large for space applications)
        const TFloat zRange = farPlane - nearPlane;

    };
}

#endif