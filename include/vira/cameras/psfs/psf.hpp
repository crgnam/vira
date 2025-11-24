#ifndef VIRA_CAMERAS_PSFS_PSF_HPP
#define VIRA_CAMERAS_PSFS_PSF_HPP

#include <vector>
#include <cstddef>

#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"

namespace vira::cameras {
    /**
     * @brief Abstract base class for point spread function implementations
     * @tparam TSpectral Spectral data type for wavelength-dependent PSF modeling
     * @tparam TFloat Floating point precision type
     *
     * Defines the interface for camera point spread functions that model optical
     * blur and diffraction effects. PSFs are used to simulate realistic image
     * formation including optical aberrations and diffraction-limited performance.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class PointSpreadFunction {
    public:
        PointSpreadFunction() = default;
        virtual ~PointSpreadFunction() = default;

        // Delete copy operations
        PointSpreadFunction(const PointSpreadFunction&) = default;
        PointSpreadFunction& operator=(const PointSpreadFunction&) = default;

        // Allow move operations
        PointSpreadFunction(PointSpreadFunction&&) = default;
        PointSpreadFunction& operator=(PointSpreadFunction&&) = default;


        vira::images::Image<TSpectral> makeKernel(int kernel_size, int supersample_factor = 0);

        vira::images::Image<TSpectral> getKernel(TSpectral received_power = TSpectral{ 0 }, float minimum_power = 0);

        vira::images::Image<TSpectral> getResponse(TSpectral received_power, float minimum_power);

    protected:
        int supersample_step_ = 1;
        std::vector<TSpectral> edge_max_values_;
        std::vector<vira::images::Image<TSpectral>> kernels_;

        /**
         * @brief Evaluates the PSF at a specific point
         * @param point Coordinate relative to PSF center
         * @return Spectral PSF value at the given point
         */
        virtual TSpectral evaluate(Pixel point) = 0;

        bool initialized_kernels_ = false;

        void initKernels(std::vector<int> kernel_sizes = std::vector<int>{ 3, 9, 27, 81 });
    };
}

#include "implementation/cameras/psfs/psf.ipp"

#endif