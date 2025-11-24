#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::cameras {
    /**
     * @brief Constructs an Owen distortion model with specified coefficients
     * @param coefficients The Owen distortion coefficients
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    OwenDistortion<TSpectral, TFloat>::OwenDistortion(OwenCoefficients coefficients) :
        coefficients_(coefficients)
    {

    }

    /**
     * @brief Computes the Owen distortion delta vector for given coordinates
     * @param homogeneous_coords Normalized homogeneous coordinates
     * @return The distortion offset to be applied to the coordinates
     * @details Calculates distortion using Owen's unique formulation that combines:
     *          - Radial terms with both even (e2, e4) and odd (e1, e3) power components
     *          - Linear directional terms (e5, e6) applied to rotated coordinates
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel OwenDistortion<TSpectral, TFloat>::computeDelta(Pixel homogeneous_coords) const
    {
        float x = homogeneous_coords[0];
        float y = homogeneous_coords[1];

        float r = std::sqrt(x * x + y * y);
        float r2 = r * r;
        float r3 = r * r * r;

        // Radial distortion factor for coordinate-aligned terms
        float radial_factor = (coefficients_.e2 * r2) + (coefficients_.e4 * r2 * r2) + (coefficients_.e5 * y) + (coefficients_.e6 * x);

        // Radial distortion factor for rotated coordinate terms
        float rotated_factor = (coefficients_.e1 * r) + (coefficients_.e3 * r3);

        // Apply distortion to original and 90-degree rotated coordinates
        Pixel rotated_coords{ -y, x };
        Pixel delta = (radial_factor * homogeneous_coords) + (rotated_factor * rotated_coords);

        return delta;
    }

    /**
     * @brief Applies Owen distortion to normalized coordinates
     * @param homogeneous_coords Undistorted normalized homogeneous coordinates
     * @return Distorted pixel coordinates
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel OwenDistortion<TSpectral, TFloat>::distort(Pixel homogeneous_coords) const
    {
        return homogeneous_coords + computeDelta(homogeneous_coords);
    }

    /**
     * @brief Removes Owen distortion using iterative method
     * @param homogeneous_coords Distorted pixel coordinates
     * @return Undistorted normalized homogeneous coordinates
     * @details Uses iterative Newton-Raphson-like method to solve the inverse distortion
     * @todo Implement convergence tolerance checking for early termination
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    Pixel OwenDistortion<TSpectral, TFloat>::undistort(Pixel homogeneous_coords) const
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