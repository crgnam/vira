#include <cmath>
#include <cstddef>
#include <stdexcept>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/math.hpp"
#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/psfs/gaussian_psf.hpp"

namespace vira::cameras {
    /**
     * @brief Constructs an Airy disk PSF with specified optical configuration
     * @param config Configuration containing focal length, aperture diameter, and pixel size
     * @throws std::runtime_error if required configuration parameters are NaN
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    AiryDiskPSF<TSpectral, TFloat>::AiryDiskPSF(AiryDiskPSFConfig config) :
        config_(config)
    {
        // Validate required configuration parameters
        if (std::isnan(config_.focal_length) || std::isnan(config_.aperture_diameter) || std::isnan(config_.pixel_size.x)) {
            throw std::runtime_error("AiryDiskPSFConfig requires focal_length, aperture_diameter, and pixel_size to be set");
        }

        this->supersample_step_ = 10;
        this->init();
    }

    /**
     * @brief Initializes the Airy disk PSF parameters
     * @details Computes the diffraction coefficient based on aperture diameter and focal length
     *          for wavelength-dependent Airy disk calculation
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void AiryDiskPSF<TSpectral, TFloat>::init()
    {
        this->coefficient_ = PI<double>() * static_cast<double>(config_.aperture_diameter / config_.focal_length);

        // TODO: Determine if additional normalizing factor is needed
        // for absolute radiometric accuracy
        this->normalizer_ = TSpectral{ 1.f };
    }

    /**
     * @brief Evaluates the Airy disk PSF at a specific pixel coordinate
     * @param point Coordinate relative to PSF center in pixel units
     * @return Spectral PSF value at the given point
     * @details Converts pixel coordinates to physical distance and evaluates
     *          the radially-symmetric Airy disk function
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    TSpectral AiryDiskPSF<TSpectral, TFloat>::evaluate(Pixel point)
    {
        // Convert pixel coordinates to physical distance
        double physical_radius = length(point * config_.pixel_size);
        return this->evaluate(physical_radius);
    }

    /**
     * @brief Evaluates the Airy disk PSF at a specific radial distance
     * @param radius Radial distance from PSF center in physical units
     * @return Spectral PSF value at the given radius
     * @details Computes the Airy disk function using the formula:
     *          \f$\text{PSF}(r) = \left(\frac{2J_1(x)}{x}\right)^2\f$
     *          where \f$x = \pi \frac{D}{f\lambda} r\f$, \f$J_1\f$ is the first-order Bessel function,
     *          \f$D\f$ is aperture diameter, \f$f\f$ is focal length, and \f$\lambda\f$ is wavelength
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    TSpectral AiryDiskPSF<TSpectral, TFloat>::evaluate(double radius)
    {
        // Handle central point (avoid division by zero)
        if (radius < 1e-20) {
            return TSpectral{ 1.f };
        }

        TSpectral airy_values;
        for (size_t i = 0; i < TSpectral::size(); ++i) {
            // Calculate argument for Bessel function:
            double x = this->coefficient_ * radius / static_cast<double>(TSpectral::bands[i].wavelength);

            // Compute first-order Bessel function J2(x)
            double bessel_j1 = vira_cyl_bessel_j<double>(1., x);

            // Apply Airy disk formula: (2*J2(x)/x)^2
            airy_values[i] = static_cast<float>(2. * bessel_j1 / x);
        }

        return this->normalizer_ * airy_values * airy_values;
    }
}