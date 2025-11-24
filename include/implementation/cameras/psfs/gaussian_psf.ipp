#include <cmath>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/psfs/psf.hpp"

namespace vira::cameras {
    /**
     * @brief Constructs a circular Gaussian PSF with uniform sigma
     * @param sigma_x Standard deviation in both X and Y directions
     * @param angle Rotation angle in degrees (default: 0)
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    GaussianPSF<TSpectral, TFloat>::GaussianPSF(TSpectral sigma_x, float angle) :
        sigma_x_(sigma_x), sigma_y_(sigma_x)
    {
        initRotation(angle);
    }

    /**
     * @brief Constructs an elliptical Gaussian PSF with different X and Y sigmas
     * @param sigma_x Standard deviation in X direction
     * @param sigma_y Standard deviation in Y direction
     * @param angle Rotation angle in degrees (default: 0)
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    GaussianPSF<TSpectral, TFloat>::GaussianPSF(TSpectral sigma_x, TSpectral sigma_y, float angle) :
        sigma_x_(sigma_x), sigma_y_(sigma_y)
    {
        initRotation(angle);
    }

    /**
     * @brief Constructs a Gaussian PSF with scalar sigma values
     * @param sigma_x Standard deviation in X direction (scalar)
     * @param sigma_y Standard deviation in Y direction (scalar)
     * @param angle Rotation angle in degrees (default: 0)
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    GaussianPSF<TSpectral, TFloat>::GaussianPSF(float sigma_x, float sigma_y, float angle) :
        sigma_x_(TSpectral{ sigma_x }), sigma_y_(TSpectral{ sigma_y })
    {
        initRotation(angle);
    }

    /**
     * @brief Evaluates the Gaussian PSF at a specific point
     * @param point Coordinate relative to PSF center
     * @return Spectral PSF value at the given point
     * @details Computes the 2D Gaussian function with rotation applied,
     *          using the formula: \f$\frac{1}{2\pi\sigma_x\sigma_y} \exp\left(-\frac{1}{2}\left(\frac{x'}{\sigma_x}\right)^2 + \left(\frac{y'}{\sigma_y}\right)^2\right)\f$
     *          where \f$(x', y')\f$ are the rotated coordinates
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    TSpectral GaussianPSF<TSpectral, TFloat>::evaluate(Pixel point)
    {
        // Apply rotation to the point
        point = rotation_ * point;

        // Compute normalized distances
        TSpectral x_normalized = point.x / sigma_x_;
        TSpectral y_normalized = point.y / sigma_y_;

        // Calculate Gaussian exponent
        TSpectral exponent = -0.5f * ((x_normalized * x_normalized) + (y_normalized * y_normalized));

        // Calculate normalization coefficient
        TSpectral coefficient = 1.f / (2.f * PI<float>() * sigma_x_ * sigma_y_);

        // Compute exponential for each spectral component
        TSpectral exponential_term;
        for (size_t i = 0; i < TSpectral::size(); ++i) {
            exponential_term[i] = std::exp(exponent[i]);
        }

        return coefficient * exponential_term;
    }

    /**
     * @brief Initializes the rotation matrix for the Gaussian PSF
     * @param angle Rotation angle in degrees
     * @details Creates a 2D rotation matrix to orient the elliptical Gaussian
     *          at the specified angle, allowing for realistic modeling of
     *          optical aberrations and astigmatism
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void GaussianPSF<TSpectral, TFloat>::initRotation(float angle)
    {
        float cos_angle = std::cos(DEG2RAD<float>() * angle);
        float sin_angle = std::sin(DEG2RAD<float>() * angle);

        rotation_[0][0] = cos_angle;
        rotation_[1][0] = sin_angle;
        rotation_[0][1] = -sin_angle;
        rotation_[1][1] = cos_angle;
    }
}