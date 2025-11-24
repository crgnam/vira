#ifndef VULKAN_CAMERA_HPP
#define VULKAN_CAMERA_HPP

#include "vulkan/vulkan.hpp"
#include "glm/vec3.hpp"

#include "vira/math.hpp"
#include "vira/rendering/vulkan/viewport_camera.hpp"
#include "vira/cameras/distortion_models/opencv_distortion.hpp"

namespace vira::vulkan {

    /**
     * @struct VulkanCamera 
     * 
     * @brief Holds camera parameters and transformation matrices in format writable to vulkan buffer
     *
     * @details This struct contains data members defining the camera's transformation matrices, 
     * position, optical properties, and distortion parameters used in rendering and simulations.
     * It holds this data in a format easy to write to a vulkan buffer.
     *
     * ### Member Variables:
     * - **View and Projection Matrices:**
     *   - `viewMatrix`: View transformation matrix.
     *   - `inverseViewMatrix`: Inverse of the view matrix.
     *   - `projectionMatrix`: Projection transformation matrix.
     *   - `normalMatrix`: Normal transformation matrix.
     *   - `instrinsicMatrix`: Camera intrinsic matrix.
     *   - `inverseInstrinsicMatrix`: Inverse of the intrinsic matrix.
     * 
     * - **Camera Position and Orientation:**
     *   - `position`: Camera’s position in world space.
     *   - `zDir`: Z-axis direction of the camera.
     * 
     * - **Optical Properties:**
     *   - `focalLength`: Camera focal length.
     *   - `aperture`: Camera aperture size.
     *   - `aspectRatio`: Aspect ratio of the camera.
     *   - `nearPlane`: Near clipping plane distance.
     *   - `farPlane`: Far clipping plane distance.
     *   - `focusDistance`: Distance at which objects appear sharp in depth-of-field calculations.
     *   - `exposure`: Exposure control for image brightness.
     *   - `depthOfField`: Enables or disables depth-of-field simulation.
     * 
     * - **Distortion Coefficients:**
     *   - `k1, k2, k3, k4, k5, k6`: Radial distortion coefficients.
     *   - `p1, p2`: Tangential distortion coefficients.
     *   - `s1, s2, s3, s4`: Thin prism distortion coefficients.
     * 
     * - **Miscellaneous:**
     *   - `resolution`: Camera resolution (width & height).
     *   - `padding1`: Padding for 16-byte memory alignment.
     * 
     * @see ViewportCamera, Camera
     */

    template <IsSpectral TSpectral, IsFloat TFloat>
    struct alignas(16) VulkanCamera {

        alignas(16) vira::mat4<TFloat> viewMatrix;       // Camera's view transformation
        alignas(16) vira::mat4<TFloat> inverseViewMatrix;
        alignas(16) vira::mat4<TFloat> projectionMatrix; // Projection transformation
        alignas(16) vira::mat4<TFloat> normalMatrix; // Projection transformation
        alignas(16) vira::mat4<TFloat> instrinsicMatrix;
        alignas(16) vira::mat4<TFloat> inverseInstrinsicMatrix;

        alignas(16) vira::vec3<TFloat> position;    // Camera's world position
        alignas(4) float padding1 = 0.0;                  // Align to 16-byte boundary
        alignas(4) float focalLength;               // Camera focal length (or field of view)
        alignas(4) float aperture;                  // Camera aperture size
        alignas(4) float aspectRatio;               // Aspect ratio
        alignas(4) float nearPlane;                 // Near clipping plane distance
        alignas(4) float farPlane;                  // Far clipping plane distance
        alignas(4) float focusDistance = std::numeric_limits<TFloat>::infinity();
        alignas(4) float exposure;                  // Exposure control for brightness
        alignas(4) float zDir;
        alignas(4) bool depthOfField = false;
        // Distortion coefficients
        alignas(4) float k1;
        alignas(4) float k2;
        alignas(4) float k3;
        alignas(4) float k4;       
        alignas(4) float k5;
        alignas(4) float k6;
        alignas(4) float p1;
        alignas(4) float p2;       
        alignas(4) float s1;
        alignas(4) float s2;
        alignas(4) float s3;
        alignas(4) float s4;       
        alignas(8) Resolution resolution;

        /**
         * @brief Provide data pointer for buffer writing, etc
         */
        const void* data() const {
            return static_cast<const void*>(this);
        }

        /**
         * @brief Pads a 2x3 matrix to a mat4
         */
        vira::mat4<TFloat> padMat23ToMat4(vira::mat23<TFloat>& inputMat, TFloat padValue) {
            vira::mat4<TFloat> paddedMat(padValue); // Initialize all elements with padValue
        
            // Copy the original 3x2 matrix into the upper-left part of the 4x4 matrix
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 2; ++col) {
                    paddedMat[row][col] = inputMat[row][col];
                }
            }
        
            return paddedMat;
        }
        
        // Constructor
        VulkanCamera(ViewportCamera<TSpectral, TFloat>& viewportCamera) {
            resetFromViewportCamera(viewportCamera);
        }

        /**
         * @brief Fills struct with values from a vira::ViewportCamera<TSpectral, TFloat>
         */           
        void resetFromViewportCamera(ViewportCamera<TSpectral, TFloat>& viewportCamera) {        
            viewMatrix = viewportCamera.getViewMatrix();       // Camera's view transformation
            inverseViewMatrix = viewportCamera.getInverseViewMatrix();       // Camera's view transformation
            projectionMatrix = viewportCamera.getProjectionMatrix(); // Projection transformation
            normalMatrix = viewportCamera.getNormalMatrix(); // Normal matrix

            vira::mat23<TFloat> Kmat = viewportCamera.getIntrinsicMatrix();
            vira::mat23<TFloat> Kinv = viewportCamera.getInverseInstrinsicMatrix();
            
            instrinsicMatrix = padMat23ToMat4(Kmat, 0.0); // Normal matrix
            inverseInstrinsicMatrix = padMat23ToMat4(Kinv, 0.0); // Normal matrix
            position = viewportCamera.getPosition();         // Camera's world position
            focalLength = viewportCamera.getFocalLength();          // Camera focal length (or field of view)
            aperture = viewportCamera.getApertureArea();             // Camera aperture size
            nearPlane = viewportCamera.getNearPlane();            // Near clipping plane distance
            farPlane = viewportCamera.getFarPlane();             // Far clipping plane distance
            focusDistance = viewportCamera.getFocusDistance();             // Far clipping plane distance
            resolution = viewportCamera.getResolution();
            aspectRatio = static_cast<float>(resolution.x) / static_cast<float>(resolution.y); // Aspect ratio
            zDir = viewportCamera.getZDir();
            depthOfField = viewportCamera.getDepthOfField();
            
            vira::DistortionCoefficients* coeffs = viewportCamera.getDistortionCoefficients();
            std::string distortionType = viewportCamera.getDistortionType();
            if (distortionType == "OpenCV") {
                vira::OpenCVCoefficients* openCVCoeffs = dynamic_cast<vira::OpenCVCoefficients*>(coeffs);
                k1 = openCVCoeffs->k1;
                k2 = openCVCoeffs->k2;
                k3 = openCVCoeffs->k3;
                k4 = openCVCoeffs->k4;
                k5 = openCVCoeffs->k5;
                k6 = openCVCoeffs->k6;
                p1 = openCVCoeffs->p1;
                p2 = openCVCoeffs->p2;
                s1 = openCVCoeffs->s1;
                s2 = openCVCoeffs->s2;
                s3 = openCVCoeffs->s3;
                s4 = openCVCoeffs->s4;
            }


        }
    };       
}


#endif // VULKAN_CAMERA_HPP