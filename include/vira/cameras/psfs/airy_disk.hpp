#ifndef VIRA_CAMERAS_PSFS_AIRY_DISK_HPP
#define VIRA_CAMERAS_PSFS_AIRY_DISK_HPP

#include <cstddef>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/psfs/psf.hpp"

namespace vira::cameras {
    /**
     * @brief Configuration structure for Airy disk PSF parameters
     *
     * Contains the optical system parameters needed to compute the diffraction-limited
     * Airy disk point spread function for realistic camera simulation.
     */
    struct AiryDiskPSFConfig {
        float focal_length;           ///< Focal length
        float aperture_diameter;      ///< Aperture Diameter
        vira::vec2<float> pixel_size; ///< Pixel Size

        size_t super_sampling = 10;   ///< Super sampling (for generating the PSF kernel)
    };

    /**
     * @brief Diffraction-limited Airy disk point spread function implementation
     * @tparam TSpectral Spectral data type for wavelength-dependent diffraction modeling
     * @tparam TFloat Floating point precision type
     *
     * Implements the theoretical diffraction-limited PSF for a circular aperture,
     * producing the characteristic Airy disk pattern. Uses Bessel function calculations
     * to provide physically accurate wavelength-dependent diffraction effects for
     * high-fidelity optical system simulation.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class AiryDiskPSF : public PointSpreadFunction<TSpectral, TFloat> {
    public:
        AiryDiskPSF() = default;
        AiryDiskPSF(AiryDiskPSFConfig config);

    private:
        AiryDiskPSFConfig config_;
        void init();

        double coefficient_ = 0.;
        TSpectral normalizer_;

        TSpectral evaluate(Pixel point) override;
        TSpectral evaluate(double radius);
    };
}

#include "implementation/cameras/psfs/airy_disk.ipp"

#endif