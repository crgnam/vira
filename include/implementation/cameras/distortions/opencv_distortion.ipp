#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::cameras {
    /**
     * @brief Constructs an OpenCV distortion model with specified coefficients
     * @param coefficients The OpenCV distortion coefficients
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    OpenCVDistortion<TSpectral, TFloat>::OpenCVDistortion(OpenCVCoefficients coefficients) :
        coefficients_(coefficients)
    {

    }

    /**
     * @brief Computes the distortion delta vector for given coordinates
     * @param homogeneous_coords Normalized homogeneous coordinates
     * @return The distortion offset between distorted and undistorted coordinates
     * @details This is a helper method that computes the difference between
     *          the input coordinates and their distorted version
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel OpenCVDistortion<TSpectral, TFloat>::computeDelta(Pixel homogeneous_coords) const
    {
        return homogeneous_coords - distort(homogeneous_coords);
    }

    /**
     * @brief Applies OpenCV distortion model to normalized coordinates
     * @param homogeneous_coords Undistorted normalized homogeneous coordinates
     * @return Distorted pixel coordinates
     * @details Implements the complete OpenCV distortion model including:
     *          - Rational radial distortion (k1-k6 coefficients)
     *          - Tangential distortion (p1, p2 coefficients)
     *          - Thin prism distortion (s1-s4 coefficients)
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel OpenCVDistortion<TSpectral, TFloat>::distort(Pixel homogeneous_coords) const
    {
        float x = homogeneous_coords[0];
        float y = homogeneous_coords[1];

        float r = std::sqrt(x * x + y * y);
        float r2 = r * r;
        float r4 = r2 * r2;
        float r6 = r2 * r2 * r2;

        // Rational radial distortion factor
        float numerator = 1 + coefficients_.k1 * r2 + coefficients_.k2 * r4 + coefficients_.k3 * r6;
        float denominator = 1 + coefficients_.k4 * r2 + coefficients_.k5 * r4 + coefficients_.k6 * r6;
        float radial_factor = numerator / denominator;

        // Tangential and thin prism distortion components
        Pixel tangential_and_prism{
            (2 * coefficients_.p1 * x * y) + (coefficients_.p2 * (r2 + 2 * x * x)) + (coefficients_.s1 * r2) + (coefficients_.s2 * r4),
            (coefficients_.p1 * (r2 + 2 * y * y)) + (2 * coefficients_.p2 * x * y) + (coefficients_.s3 * r2) + (coefficients_.s4 * r4)
        };

        return (radial_factor * homogeneous_coords) + tangential_and_prism;
    }

    /**
     * @brief Removes OpenCV distortion using iterative method
     * @param homogeneous_coords Distorted pixel coordinates
     * @return Undistorted normalized homogeneous coordinates
     * @details Uses iterative Newton-Raphson-like method to solve the inverse distortion
     * @todo Implement convergence tolerance checking for early termination
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel OpenCVDistortion<TSpectral, TFloat>::undistort(Pixel homogeneous_coords) const
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