#include <cstddef>
#include <random>
#include <cmath>

#include "vira/math.hpp"

namespace vira::cameras {
    /**
     * @brief Constructs a noise model with specified configuration
     * @param config The noise model configuration parameters
     * @details Initializes the Poisson distribution for dark current and
     *          normal distribution for readout noise based on config parameters
     */
    inline NoiseModel::NoiseModel(NoiseModelConfig config) :
        config_(config)
    {
        poisson_dist_ = std::poisson_distribution<int>(static_cast<double>(config_.dark_current));
        normal_dist_ = std::normal_distribution<float>(config_.readout_noise_mean, config_.readout_noise_std);

        horizontal_pattern_start_ = 1.f - config_.horizontal_scale;
        vertical_pattern_start_ = 1.f - config_.vertical_scale;
    }

    /**
     * @brief Simulates total noise for a pixel at given coordinates and exposure
     * @param rng Random number generator for noise sampling
     * @param i Horizontal pixel coordinate
     * @param j Vertical pixel coordinate
     * @param exposure_time Exposure duration in seconds
     * @return Total noise count in electrons
     * @details Combines dark current noise (Poisson-distributed), readout noise
     *          (Gaussian-distributed), and fixed pattern noise for realistic
     *          camera sensor simulation
     */
    inline float NoiseModel::simulate(std::mt19937& rng, size_t i, size_t j, float exposure_time)
    {
        float dark_counts = static_cast<float>(poisson_dist_(rng)) * exposure_time;
        float readout_counts = normal_dist_(rng);

        float total_noise = this->getFixedPatternFactor(rng, i, j) * (dark_counts + readout_counts);

        return total_noise;
    }

    /**
     * @brief Computes the fixed pattern noise scaling factor for a pixel
     * @param rng Random number generator (unused in this implementation)
     * @param i Horizontal pixel coordinate
     * @param j Vertical pixel coordinate
     * @return Multiplicative scaling factor for fixed pattern noise
     * @details Multiplicative fixed pattern noise factor
     */
    inline float NoiseModel::getFixedPatternFactor(std::mt19937& rng, size_t i, size_t j)
    {
        (void)rng;

        float scale = 1.f;

        // Apply horizontal pattern noise if configured
        if (config_.horizontal_pattern_period != 0) {
            scale *= horizontal_pattern_start_ + (config_.horizontal_scale *
                (1.f + std::sin(2.f * PI<float>() * static_cast<float>(i) /
                    (2.f * static_cast<float>(config_.horizontal_pattern_period)))));
        }

        // Apply vertical pattern noise if configured
        if (config_.vertical_pattern_period != 0) {
            scale *= vertical_pattern_start_ + (config_.vertical_scale *
                (1.f + std::sin(2.f * PI<float>() * static_cast<float>(j) /
                    (2.f * static_cast<float>(config_.vertical_pattern_period)))));
        }

        return scale;
    }
}