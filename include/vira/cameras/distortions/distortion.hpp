#ifndef VIRA_CAMERAS_DISTORTIONS_DISTORTION_HPP
#define VIRA_CAMERAS_DISTORTIONS_DISTORTION_HPP

#include <variant>
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image.hpp"
#include "vira/rendering/ray.hpp"
#include <string>

namespace vira::cameras {

    /**
     * @brief Base class for lens distortion coefficients
     */
    struct DistortionCoefficients {
        DistortionCoefficients() = default;
        DistortionCoefficients(const DistortionCoefficients&) = default;
        virtual ~DistortionCoefficients() = default;
    };

    /**
     * @brief Abstract base class for lens distortion models
     * @tparam TSpectral Spectral data type
     * @tparam TFloat Floating point precision type
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class Distortion {
    public:
        Distortion() = default;

        virtual ~Distortion() = default;

        /**
         * @brief Applies lens distortion to normalized image coordinates
         * @param homogeneous_coords Normalized homogeneous coordinates (undistorted)
         * @return Distorted pixel coordinates
         */
        virtual Pixel distort(Pixel homogeneous_coords) const = 0;

        /**
         * @brief Removes lens distortion from image coordinates
         * @param homogeneous_coords Distorted pixel coordinates
         * @return Undistorted normalized homogeneous coordinates
         */
        virtual Pixel undistort(Pixel homogeneous_coords) const = 0;

        /**
         * @brief Gets the distortion coefficients for this model
         * @return Pointer to the distortion coefficients
         */
        virtual DistortionCoefficients* getCoefficients() = 0;

        /**
         * @brief Gets the distortion model type as a string
         * @return String identifier for the distortion model type
         */
        virtual std::string getType() const = 0;
    };
}

#endif