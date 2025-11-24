#ifndef VIRA_CAMERAS_PSFS_GAUSSIAN_PSF_HPP
#define VIRA_CAMERAS_PSFS_GAUSSIAN_PSF_HPP

#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/psfs/psf.hpp"

namespace vira::cameras {
    /**
     * @brief Gaussian point spread function implementation
     * @tparam TSpectral Spectral data type for wavelength-dependent PSF modeling
     * @tparam TFloat Floating point precision type
     *
     * Implements a 2D Gaussian PSF that can model various optical aberrations
     * including astigmatism through elliptical shapes and arbitrary rotation angles.
     * Commonly used as an approximation to diffraction-limited performance or
     * to model specific optical system characteristics.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class GaussianPSF : public PointSpreadFunction<TSpectral, TFloat> {
    public:
        GaussianPSF(TSpectral sigma_x, float angle = 0.f);
        GaussianPSF(TSpectral sigma_x, TSpectral sigma_y, float angle = 0.f);
        GaussianPSF(float sigma_x, float sigma_y, float angle = 0.f);

    private:
        TSpectral sigma_x_{ 1.f };
        TSpectral sigma_y_{ 1.f };
        mat2<float> rotation_;

        void initRotation(float angle);

        TSpectral evaluate(Pixel point) override;
    };
}

#include "implementation/cameras/psfs/gaussian_psf.ipp"

#endif