#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::cameras {
    /**
     * @brief Constructs a Brown distortion model with specified coefficients
     * @param coefficients The Brown-Conrady distortion coefficients
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    BrownDistortion<TSpectral, TFloat>::BrownDistortion(BrownCoefficients coefficients) :
        coefficients_(coefficients)
    {

    }

    /**
     * @brief Computes the distortion delta vector for given coordinates
     * @param homogeneous_coords Normalized homogeneous coordinates
     * @return The distortion offset to be applied to the coordinates
     * @details Calculates both radial and tangential distortion components
     *          using the Brown-Conrady model equations
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel BrownDistortion<TSpectral, TFloat>::computeDelta(Pixel homogeneous_coords) const
    {
        float x = homogeneous_coords[0];
        float y = homogeneous_coords[1];

        float r = std::sqrt(x * x + y * y);
        float r2 = r * r;
        float r4 = r2 * r2;
        float r6 = r2 * r2 * r2;

        // Radial distortion component
        Pixel radial_distortion = (coefficients_.k1 * r2 + coefficients_.k2 * r4 + coefficients_.k3 * r6) * homogeneous_coords;

        // Tangential distortion component
        Pixel tangential_distortion{
            (2 * coefficients_.p1 * x * y) + (coefficients_.p2 * (r2 + 2 * x * x)),
            (coefficients_.p1 * (r2 + 2 * y * y)) + (2 * coefficients_.p2 * x * y)
        };

        return radial_distortion + tangential_distortion;
    }

    /**
     * @brief Applies Brown-Conrady distortion to normalized coordinates
     * @param homogeneous_coords Undistorted normalized homogeneous coordinates
     * @return Distorted pixel coordinates
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel BrownDistortion<TSpectral, TFloat>::distort(Pixel homogeneous_coords) const
    {
        return homogeneous_coords + computeDelta(homogeneous_coords);
    }

    /**
     * @brief Removes Brown-Conrady distortion using iterative method
     * @param homogeneous_coords Distorted pixel coordinates
     * @return Undistorted normalized homogeneous coordinates
     * @details Uses iterative Newton-Raphson-like method to solve the inverse distortion
     * @todo Implement convergence tolerance checking for early termination
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel BrownDistortion<TSpectral, TFloat>::undistort(Pixel homogeneous_coords) const
    {
        Pixel undistorted_coords = homogeneous_coords;
        for (size_t i = 0; i < 20; ++i) {
            Pixel delta = computeDelta(undistorted_coords);
            undistorted_coords = homogeneous_coords - delta;

            // TODO: Implement tolerance checking for convergence
        }

        return undistorted_coords;
    }
}