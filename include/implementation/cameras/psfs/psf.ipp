#include <stdexcept>
#include <cstddef>
#include <algorithm>
#include <vector>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"
#include "vira/images/resolution.hpp"
#include "vira/images/compositing.hpp"
#include "vira/images/image_pixel.hpp"

namespace vira::cameras {
    /**
     * @brief Creates a PSF kernel with specified size and supersampling
     * @param kernel_size Kernel size (must be odd number >= 3)
     * @param supersample_factor Supersampling factor for anti-aliasing (0 uses default)
     * @return Anti-aliased PSF kernel for convolution operations
     * @details Uses supersampling to reduce aliasing artifacts when discretizing
     *          the continuous PSF function onto a pixel grid
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    vira::images::Image<TSpectral> PointSpreadFunction<TSpectral, TFloat>::makeKernel(int kernel_size, int supersample_factor)
    {
        if (kernel_size % 2 == 0 || kernel_size <= 2) {
            throw std::runtime_error("Kernel must have an odd resolution >= 3");
        }

        if (supersample_factor < 0) {
            throw std::runtime_error("Supersample factor must be non-negative");
        }

        if (supersample_factor == 0) {
            supersample_factor = this->supersample_step_;
        }

        const float center_coord = (static_cast<float>(kernel_size) - 1.f) / 2.f;
        const Pixel center{ center_coord, center_coord };

        vira::images::Image<TSpectral> new_kernel(vira::images::Resolution{ kernel_size, kernel_size }, 0.f);

        // Parallel kernel generation with supersampling
        vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)
        tbb::parallel_for(
            tbb::blocked_range2d<int>(0, kernel_size, 0, kernel_size),
            [&](const tbb::blocked_range2d<int>& range) {
                for (int i = range.rows().begin(); i < range.rows().end(); i++) {
                    for (int j = range.cols().begin(); j < range.cols().end(); j++) {
                        // Supersample within this pixel for anti-aliasing
                        TSpectral accumulator{ 0 };
                        for (int sample_i = 0; sample_i < supersample_factor; ++sample_i) {
                            for (int sample_j = 0; sample_j < supersample_factor; ++sample_j) {
                                // Calculate sub-pixel offset in range [0,1]
                                float offset_x = (static_cast<float>(sample_i) + 0.5f) / static_cast<float>(supersample_factor);
                                float offset_y = (static_cast<float>(sample_j) + 0.5f) / static_cast<float>(supersample_factor);

                                // Create sub-pixel coordinate relative to pixel center
                                Pixel sub_pixel{
                                    static_cast<float>(i) + offset_x - 0.5f,
                                    static_cast<float>(j) + offset_y - 0.5f
                                };

                                // Evaluate PSF at this sub-pixel location relative to kernel center
                                accumulator += this->evaluate(sub_pixel - center);
                            }
                        }

                        // Store the average of all supersamples for this pixel
                        new_kernel(i, j) = accumulator / (supersample_factor * supersample_factor);
                    }
                }
            }
        );

        return new_kernel;
    }

    /**
     * @brief Gets appropriate kernel size based on received power and minimum threshold
     * @param received_power Spectral power received by the detector
     * @param minimum_power Minimum power threshold for kernel truncation
     * @return PSF kernel appropriately sized for the power level
     * @details Selects smallest kernel where edge power exceeds minimum threshold,
     *          preventing unnecessary computation for low-power sources
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    vira::images::Image<TSpectral> PointSpreadFunction<TSpectral, TFloat>::getKernel(TSpectral received_power, float minimum_power)
    {
        if (!initialized_kernels_) {
            this->initKernels();
        }

        // If no minimum power specified, return largest kernel
        if (minimum_power == 0.f) {
            return kernels_[kernels_.size() - 1];
        }

        // Select smallest kernel where edge power exceeds threshold
        size_t selected_index = 0;
        for (size_t i = 0; i < kernels_.size(); ++i) {
            selected_index = i;
            if ((edge_max_values_[i] * received_power).magnitude() < minimum_power) {
                break;
            }
        }

        return kernels_[selected_index];
    }

    /**
     * @brief Gets PSF response scaled by received power
     * @param received_power Spectral power received by the detector
     * @param minimum_power Minimum power threshold for kernel selection
     * @return Power-scaled PSF response for realistic image formation
     * @details Combines adaptive kernel selection with power scaling to simulate
     *          realistic point source imaging including power-dependent blur extent
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    vira::images::Image<TSpectral> PointSpreadFunction<TSpectral, TFloat>::getResponse(TSpectral received_power, float minimum_power)
    {
        auto kernel = this->getKernel(received_power, minimum_power);
        return received_power * kernel;
    }

    /**
     * @brief Initializes pre-computed kernels of various sizes for adaptive selection
     * @param kernel_sizes Vector of kernel sizes to pre-compute (must be odd)
     * @details Pre-computes kernels and calculates edge maximum values for each size
     *          to enable adaptive kernel selection based on power thresholds
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void PointSpreadFunction<TSpectral, TFloat>::initKernels(std::vector<int> kernel_sizes)
    {
        kernels_.resize(kernel_sizes.size());
        edge_max_values_.resize(kernel_sizes.size());

        for (size_t i = 0; i < kernel_sizes.size(); ++i) {
            kernels_[i] = this->makeKernel(kernel_sizes[i], 20);

            // Calculate maximum value along kernel edge for adaptive sizing
            auto& kernel = kernels_[i];
            int size = kernel_sizes[i];
            TSpectral max_edge_value(0);

            // Check top and bottom edges
            for (int x = 0; x < size; ++x) {
                max_edge_value = std::max(max_edge_value, kernel(0, x));         // Top edge
                max_edge_value = std::max(max_edge_value, kernel(size - 1, x));  // Bottom edge
            }

            // Check left and right edges (excluding corners already checked)
            for (int y = 1; y < size - 1; ++y) {
                max_edge_value = std::max(max_edge_value, kernel(y, 0));         // Left edge
                max_edge_value = std::max(max_edge_value, kernel(y, size - 1));  // Right edge
            }

            edge_max_values_[i] = max_edge_value;
        }

        initialized_kernels_ = true;
    }
}