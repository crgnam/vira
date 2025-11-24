#ifndef VIRA_CAMERAS_NOISE_MODELS_NOISE_MODEL_HPP
#define VIRA_CAMERAS_NOISE_MODELS_NOISE_MODEL_HPP

#include <cstddef>
#include <cmath>
#include <random>
#include <array>

namespace vira::cameras {
    /**
     * @brief Configuration structure for camera noise model parameters
     *
     * Contains all the parameters needed to configure realistic camera sensor
     * noise including dark current, readout noise, and fixed pattern noise.
     */
    struct NoiseModelConfig {
        float dark_current = 0;                    ///< Dark current noise level (electrons/second)
        float readout_noise_mean = 0;              ///< Mean of readout noise distribution
        float readout_noise_std = 0;               ///< Standard deviation of readout noise distribution

        float horizontal_scale = 0;                ///< Scale factor for horizontal fixed pattern noise
        float vertical_scale = 0;                  ///< Scale factor for vertical fixed pattern noise
        size_t horizontal_pattern_period = 0;      ///< Period of horizontal pattern noise (pixels)
        size_t vertical_pattern_period = 0;        ///< Period of vertical pattern noise (pixels)
    };

    /**
     * @brief Camera sensor noise model for realistic image simulation
     *
     * Simulates various types of camera sensor noise including dark current,
     * readout noise, and fixed pattern noise to create realistic sensor
     * behavior for image rendering applications.
     */
    class NoiseModel {
    public:
        NoiseModel() = default;
        NoiseModel(NoiseModelConfig config);

        virtual ~NoiseModel() = default;

        // Delete copy operations
        NoiseModel(const NoiseModel&) = delete;
        NoiseModel& operator=(const NoiseModel&) = delete;

        // Allow move operations
        NoiseModel(NoiseModel&&) = default;
        NoiseModel& operator=(NoiseModel&&) = default;


        virtual float simulate(std::mt19937& rng, size_t i, size_t j, float exposure_time);
        
        virtual float getFixedPatternFactor(std::mt19937& rng, size_t i, size_t j);

    private:
        NoiseModelConfig config_;

        float horizontal_pattern_start_;
        float vertical_pattern_start_;

        std::poisson_distribution<int> poisson_dist_;
        std::normal_distribution<float> normal_dist_;
    };
}

#include "implementation/cameras/noise_models/noise_model.ipp"

#endif