// Standard library headers (alphabetical within each group)
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

// Third-party library headers
#include "tbb/blocked_range2d.h"
#include "tbb/parallel_for.h"

// Project core headers (foundational types first)
#include "vira/constraints.hpp"
#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/vec.hpp"

// Project image/rendering headers
#include "vira/images/image.hpp"
#include "vira/images/interfaces/image_interface.hpp"
#include "vira/rendering/acceleration/frustum.hpp"
#include "vira/rendering/acceleration/obb.hpp"
#include "vira/rendering/ray.hpp"

// Project camera-specific headers (organized by subsystem)
#include "vira/cameras/filter_arrays.hpp"
#include "vira/cameras/apertures/aperture.hpp"
#include "vira/cameras/apertures/circular_aperture.hpp"
#include "vira/cameras/distortions/brown_distortion.hpp"
#include "vira/cameras/distortions/opencv_distortion.hpp"
#include "vira/cameras/distortions/owen_distortion.hpp"
#include "vira/cameras/photosites/photosite.hpp"
#include "vira/cameras/psfs/airy_disk.hpp"
#include "vira/cameras/psfs/gaussian_psf.hpp"

// Project utility headers
#include "vira/utils/print_utils.hpp"
#include "vira/debug.hpp"

namespace vira::cameras {
    /**
     * @brief Default constructor initializing camera with default settings
     * @details Sets up basic camera parameters including aperture diameter calculation
     *          based on default focal length and f-stop values
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Camera<TSpectral, TFloat, TMeshFloat>::Camera()
    {
        aperture_diameter_ = static_cast<float>(focal_length_) / f_stop_;
    }


    // ====================== //
    // === Initialization === //
    // ====================== //

    /**
     * @brief Initializes all camera components and precomputes cached values
     * @details Performs lazy initialization of camera subsystems including photosite,
     *          aperture, PSF, noise model, and filter mosaic. Also precomputes
     *          intrinsic matrices, pixel solid angles, and viewing frustum for
     *          efficient runtime operation.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::initialize()
    {
        if (needs_initialization_) {
            std::chrono::high_resolution_clock::time_point start_time;
            std::chrono::high_resolution_clock::time_point stop_time;
            if (vira::getPrintStatus()) {
                std::cout << "Initializing Camera...\n" << std::flush;
                start_time = std::chrono::high_resolution_clock::now();
            }

            // Initialize photosite if not custom-set
            if (!photosite_) {
                photosite_ = std::make_unique<Photosite<TSpectral>>(default_photosite_config_);
            }

            // Initialize aperture if not custom-set
            if (!aperture_) {
                aperture_ = std::make_unique<CircularAperture<TSpectral, TFloat>>();
                aperture_->setDiameter(focal_length_ / f_stop_);
            }

            // Cache pixel size for efficient access
            pixel_size_.x = static_cast<float>(sensor_size_.x) / static_cast<float>(resolution_.x);
            pixel_size_.y = static_cast<float>(sensor_size_.y) / static_cast<float>(resolution_.y);

            // Initialize PSF if not custom-set
            if (!psf_) {
                if (default_psf_option_ == DefaultPSFOption::AIRY_DISK_PSF) {
                    AiryDiskPSFConfig airy_config;
                    airy_config.focal_length = static_cast<float>(focal_length_);
                    airy_config.aperture_diameter = aperture_->getDiameter();
                    airy_config.pixel_size = pixel_size_;
                    psf_ = std::make_unique<AiryDiskPSF<TSpectral, TFloat>>(airy_config);
                }
                else if (default_psf_option_ == DefaultPSFOption::GAUSSIAN_PSF) {
                    // Calculate sigma for Gaussian approximation to Airy disk for each wavelength
                    TSpectral sigma_x;
                    TSpectral sigma_y;
                    for (size_t i = 0; i < TSpectral::size(); ++i) {
                        // Airy disk first zero radius approximation: r ~ 0.84 * lambda * f / D
                        float wavelength = TSpectral::wavelengths[i];
                        float airy_radius = (0.84f * wavelength * static_cast<float>(focal_length_)) / aperture_->getDiameter();
                        sigma_x[i] = airy_radius / pixel_size_.x;
                        sigma_y[i] = airy_radius / pixel_size_.y;
                    }

                    psf_ = std::make_unique<GaussianPSF<TSpectral, TFloat>>(sigma_x, sigma_y, 0.f);
                }
            }

            // Initialize noise model if configured
            if (!noise_model_ && noise_model_configured_) {
                noise_model_ = std::make_unique<NoiseModel>(default_noise_model_config_);
            }

            // Initialize Bayer filter mosaic if enabled
            if (use_bayer_filter_) {
                if (!custom_filters_) {
                    // Use default RGB to spectral conversion for standard Bayer filters
                    default_bayer_red_ = rgb_to_spectral_(ColorRGB{ 1, 0, 0 });
                    default_bayer_green_ = rgb_to_spectral_(ColorRGB{ 0, 1, 0 });
                    default_bayer_blue_ = rgb_to_spectral_(ColorRGB{ 0, 0, 1 });
                }

                filter_mosaic_ = bayerFilter(resolution_, default_bayer_red_, default_bayer_green_, default_bayer_blue_);
            }

            // Perform precomputation for efficient runtime operation
            this->initializeIntrinsicMatrix();
            this->initializePixelSolidAngle();

            // Configure coordinate system direction
            if (blender_frame_) {
                z_dir_ = -1.f;
            }

            // Determine if direction interpolation should be used
            interpolate_directions_ = false;
            if (interpolate_distortion_directions_ && hasDistortion()) {
                interpolate_directions_ = true;
            }

            // Precompute pixel directions if interpolation is enabled
            if (interpolate_directions_) {
                this->precomputePixelDirections();
            }
            else {
                precomputed_pixel_directions_.clear();
            }

            // Precompute viewing frustum for culling
            this->precomputeFrustum();

            needs_initialization_ = false;

            if (vira::getPrintStatus()) {
                stop_time = std::chrono::high_resolution_clock::now();
                auto duration = duration_cast<std::chrono::milliseconds>(stop_time - start_time);
                std::cout << vira::print::VIRA_INDENT << "Completed (" << duration.count() << " ms)\n" << std::flush;
            }
        }
    }

    // =================================== //
    // === Generic Processing Settings === //
    // =================================== //

    /**
     * @brief Enables or disables parallel initialization of camera components
     * @param parallel_initialization Whether to use parallel processing during initialization
     * @details When enabled, computationally expensive initialization tasks like
     *          pixel solid angle calculation are performed in parallel using TBB
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::enableParallelInitialization(bool parallel_initialization)
    {
        parallel_initialization_ = parallel_initialization;
    }

    /**
     * @brief Enables or disables precomputed interpolation for lens distortion
     * @param interpolation Whether to precompute and interpolate distorted pixel directions
     * @details When enabled, pixel-to-ray direction calculations are precomputed for all
     *          pixels and stored for fast interpolation. Useful for heavy distortion models
     *          but increases memory usage. Triggers camera reinitialization.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::enableDistortionInterpolation(bool interpolation)
    {
        interpolate_distortion_directions_ = interpolation;
        needs_initialization_ = true;
    }

    /**
     * @brief Enables or disables depth of field simulation
     * @param depth_of_field Whether to simulate aperture-based depth of field effects
     * @details When enabled, ray generation includes aperture sampling to create
     *          realistic depth of field blur based on focus distance and aperture size
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::enableDepthOfField(bool depth_of_field)
    {
        depth_of_field_ = depth_of_field;
    }

    /**
     * @brief Enables or disables Blender-compatible coordinate system
     * @param blender_frame Whether to use Blender's coordinate system conventions
     * @details When enabled, adjusts coordinate transformations to match Blender's
     *          camera orientation and projection conventions. Triggers camera reinitialization.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::enableBlenderFrame(bool blender_frame)
    {
        blender_frame_ = blender_frame;
        needs_initialization_ = true;
    }


    // =================================== //
    // === Camera Intrinsic Parameters === //
    // =================================== //

    /**
     * @brief Sets the camera focal length and updates related parameters
     * @param focal_length Focal length in meters (must be positive)
     * @details Updates the focal length and recalculates the f-stop based on current
     *          aperture diameter. Triggers camera reinitialization to update intrinsic matrix.
     * @throws std::invalid_argument if focal_length is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setFocalLength(double focal_length)
    {
        vira::utils::validatePositiveDefinite(focal_length, "Focal Length");
        focal_length_ = static_cast<TFloat>(focal_length);
        f_stop_ = static_cast<float>(focal_length) / aperture_diameter_;
        needs_initialization_ = true;
    }

    /**
     * @brief Sets the camera principal point (optical center) in pixel coordinates
     * @param px Principal point X coordinate in pixels (must be positive)
     * @param py Principal point Y coordinate in pixels (must be positive)
     * @details The principal point is where the optical axis intersects the image plane.
     *          Typically near the image center but may be offset due to manufacturing tolerances.
     *          Triggers camera reinitialization to update intrinsic matrix.
     * @throws std::invalid_argument if px or py is not positive
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setPrincipalPoint(double px, double py)
    {
        vira::utils::validatePositive(px, "Principal Point X");
        vira::utils::validatePositive(py, "Principal Point Y");

        px_ = static_cast<TFloat>(px);
        py_ = static_cast<TFloat>(py);

        needs_initialization_ = true;
    }

    /**
     * @brief Sets the camera skew parameters for non-rectangular pixels
     * @param kxy Skew parameter relating X and Y pixel coordinates
     * @param kyx Skew parameter relating Y and X pixel coordinates
     * @details Skew parameters account for non-orthogonal pixel grid orientations,
     *          typically zero for modern digital cameras but may be non-zero for
     *          specialized sensors or calibration corrections.
     *          Triggers camera reinitialization to update intrinsic matrix.
     * @throws std::invalid_argument if kxy or kyx is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setSkewParameters(double kxy, double kyx)
    {
        vira::utils::validateFinite(kxy, "Skew Kxy");
        vira::utils::validateFinite(kyx, "Skew Kyx");

        kxy_ = static_cast<TFloat>(kxy);
        kyx_ = static_cast<TFloat>(kyx);

        needs_initialization_ = true;
    }

    /**
     * @brief Sets the image resolution and optionally updates sensor size
     * @param resolution Image resolution in pixels (width x height)
     * @details Updates the image resolution and triggers camera reinitialization.
     *          If square pixel mode is enabled, automatically adjusts sensor size
     *          to maintain square pixels with the new resolution.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setResolution(vira::images::Resolution resolution)
    {
        resolution_ = resolution;
        needs_initialization_ = true;
        if (square_pixel_) {
            setSensorSize(sensor_size_.x);
        }
    }

    /**
     * @brief Sets the image resolution using separate width and height values
     * @param x Image width in pixels
     * @param y Image height in pixels
     * @details Convenience overload that constructs a Resolution object and calls
     *          the main setResolution method.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setResolution(size_t x, size_t y)
    {
        this->setResolution(vira::images::Resolution{ x, y });
    }

    /**
     * @brief Sets the physical sensor size with independent X and Y dimensions
     * @param sensor_size Physical sensor dimensions in meters (width x height)
     * @details Sets the physical sensor size and disables square pixel mode,
     *          allowing for non-square pixels. Triggers camera reinitialization
     *          to update pixel size calculations and intrinsic matrix.
     * @throws std::invalid_argument if sensor_size components are not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setSensorSize(vec2<TFloat> sensor_size)
    {
        vira::utils::validatePositiveDefinite(sensor_size.x, "Sensor Size X");
        vira::utils::validatePositiveDefinite(sensor_size.y, "Sensor Size Y");
        sensor_size_ = sensor_size;
        needs_initialization_ = true;
        square_pixel_ = false;
    }

    /**
     * @brief Sets the physical sensor size using separate width and height values
     * @param x Sensor width in meters (must be positive)
     * @param y Sensor height in meters (must be positive)
     * @details Convenience overload that constructs a vec2 and calls the main
     *          setSensorSize method. Disables square pixel mode.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setSensorSize(double x, double y)
    {
        this->setSensorSize(vec2<TFloat>{x, y});
    }

    /**
     * @brief Sets the sensor width and calculates height for square pixels
     * @param x Sensor width in meters (must be positive)
     * @details Calculates the sensor height based on the aspect ratio of the current
     *          resolution to maintain square pixels. Enables square pixel mode and
     *          triggers camera reinitialization.
     * @throws std::invalid_argument if x is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setSensorSize(double x)
    {
        vira::utils::validatePositiveDefinite(x, "Sensor Size");
        double y = static_cast<double>(resolution_.y) * (x / static_cast<double>(resolution_.x));
        sensor_size_ = vec2<TFloat>{ x, y };
        needs_initialization_ = true;
        square_pixel_ = true;
    }

    /**
     * @brief Sets uniform optical efficiency across all wavelengths
     * @param optical_efficiency Optical transmission efficiency (0-1 range)
     * @details Sets a constant optical efficiency for all spectral bands,
     *          representing the fraction of incident light that reaches the sensor
     *          after passing through the optical system (lenses, filters, etc.).
     * @throws std::invalid_argument if optical_efficiency is not in normalized range
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setOpticalEfficiency(double optical_efficiency)
    {
        vira::utils::validateNormalized(optical_efficiency, "Optical Efficiency");
        optical_efficiency_ = TSpectral{ static_cast<float>(optical_efficiency) };
    }

    /**
     * @brief Sets wavelength-dependent optical efficiency
     * @param optical_efficiency Spectral optical transmission efficiency curve
     * @details Sets wavelength-dependent optical efficiency, allowing for realistic
     *          modeling of wavelength-selective losses in the optical system such as
     *          lens coatings, filters, and material absorption.
     * @throws std::invalid_argument if any optical_efficiency values are not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setOpticalEfficiency(TSpectral optical_efficiency)
    {
        vira::utils::validatePositiveDefinite(optical_efficiency, "Optical Efficiency");
        optical_efficiency_ = optical_efficiency;
    }


    // ======================= //
    // === Camera Settings === //
    // ======================= //

    /**
     * @brief Sets the camera exposure time
     * @param exposure_time Exposure duration in seconds (must be positive)
     * @details Controls how long the sensor is exposed to light, affecting
     *          image brightness and motion blur characteristics.
     * @throws std::invalid_argument if exposure_time is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setExposureTime(double exposure_time)
    {
        vira::utils::validatePositiveDefinite(exposure_time, "Exposure Time");
        exposure_time_ = static_cast<float>(exposure_time);
    }

    /**
     * @brief Sets the camera focus distance for depth of field calculations
     * @param focus_distance Distance to the focus plane in meters (must be finite, may be infinite)
     * @details Controls the distance at which objects appear in sharp focus.
     *          Used for depth of field calculations when DOF is enabled.
     *          Set to infinity for hyperfocal focusing.
     * @throws std::invalid_argument if focus_distance is NaN
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setFocusDistance(double focus_distance)
    {
        vira::utils::validateNotNaN(focus_distance, "Focus Distance");
        focus_distance_ = static_cast<TFloat>(focus_distance);
    }

    /**
     * @brief Sets the camera f-stop and updates aperture diameter
     * @param f_stop F-number (focal length / aperture diameter, must be positive)
     * @details Controls depth of field and exposure. Lower f-stop values create
     *          shallower depth of field and allow more light. Updates the physical
     *          aperture diameter and notifies existing aperture objects.
     * @throws std::invalid_argument if f_stop is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setFStop(double f_stop)
    {
        vira::utils::validatePositiveDefinite(f_stop, "F-Stop");
        f_stop_ = static_cast<float>(f_stop);
        aperture_diameter_ = static_cast<float>(focal_length_) / f_stop_;

        if (hasAperture()) {
            aperture_->setDiameter(aperture_diameter_);
        }
    }

    /**
     * @brief Sets the physical aperture diameter and updates f-stop
     * @param aperture_diameter Physical aperture diameter in meters (must be positive)
     * @details Directly controls the physical aperture size and automatically
     *          calculates the corresponding f-stop. Updates existing aperture objects.
     * @throws std::invalid_argument if aperture_diameter is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setApertureDiameter(double aperture_diameter)
    {
        vira::utils::validatePositiveDefinite(aperture_diameter, "Aperture Diameter");
        aperture_diameter_ = static_cast<float>(aperture_diameter);
        f_stop_ = static_cast<float>(focal_length_) / aperture_diameter_;

        if (hasAperture()) {
            aperture_->setDiameter(aperture_diameter_);
        }
    }

    /**
     * @brief Sets the sensor gain in linear units (ADU/electron)
     * @param gain Linear gain value (must be positive)
     * @details Controls the amplification of the photosite signal before digitization.
     *          Higher gain increases sensitivity but also amplifies noise.
     *          Applies to default photosite config if no custom photosite is set.
     * @throws std::invalid_argument if gain is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setGain(double gain)
    {
        vira::utils::validatePositiveDefinite(gain, "Gain");
        if (!photosite_) {
            default_photosite_config_.gain = static_cast<float>(gain);
        }
        else {
            photosite_->setGain(gain);
        }
    }

    /**
     * @brief Sets the sensor gain in decibels
     * @param gain_db Gain value in decibels
     * @param new_unity_gain_db Unity gain reference in dB (default: 0)
     * @details Sets gain using decibel notation, which is common in camera specifications.
     *          Applies to default photosite config if no custom photosite is set.
     * @throws std::invalid_argument if gain_db or new_unity_gain_db is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setGainDB(double gain_db, double new_unity_gain_db)
    {
        vira::utils::validateFinite(gain_db, "Gain (dB)");
        vira::utils::validateFinite(new_unity_gain_db, "Unity Gain (dB)");

        if (!photosite_) {
            default_photosite_config_.setGainDB(gain_db, new_unity_gain_db);
        }
        else {
            photosite_->setGainDB(gain_db, new_unity_gain_db);
        }
    }

    // =============================== //
    // === Photosite Configuration === //
    // =============================== //
    /**
    * @brief Sets a custom photosite implementation
    * @param custom_photosite Unique pointer to custom photosite object
    * @details Replaces the default photosite with a user-provided implementation.
    *          Takes ownership of the provided photosite object.
    */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setCustomPhotosite(std::unique_ptr<Photosite<TSpectral>> custom_photosite)
    {
        photosite_ = std::move(custom_photosite);
    }

    /**
     * @brief Gets reference to the camera's photosite for direct configuration
     * @return Reference to the photosite object
     * @throws std::runtime_error if photosite is not initialized
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Photosite<TSpectral>& Camera<TSpectral, TFloat, TMeshFloat>::photosite()
    {
        if (!photosite_) {
            throw std::runtime_error("Photosite not initialized");
        }
        return *photosite_;
    }

    /**
     * @brief Sets the bit depth for default photosite configuration
     * @param bit_depth ADC bit depth (e.g., 8, 10, 12, 14, 16)
     * @details Only affects default photosite configuration. Has no effect
     *          if a custom photosite has already been set.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeBitDepth(size_t bit_depth)
    {
        default_photosite_config_.bit_depth = bit_depth;
    }

    /**
     * @brief Sets the well depth for default photosite configuration
     * @param well_depth Pixel well depth in electrons
     * @details Controls the saturation level of the photosite. Only affects
     *          default photosite configuration.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeWellDepth(size_t well_depth)
    {
        default_photosite_config_.well_depth = well_depth;
    }

    /**
     * @brief Sets uniform quantum efficiency for all wavelengths
     * @param quantum_efficiency Quantum efficiency value (0-1 range, must be positive)
     * @details Sets a constant quantum efficiency across all spectral bands.
     *          Only affects default photosite configuration.
     * @throws std::invalid_argument if quantum_efficiency is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeQuantumEfficiency(double quantum_efficiency)
    {
        vira::utils::validatePositiveDefinite(quantum_efficiency, "Quantum Efficiency");
        default_photosite_config_.quantum_efficiency = TSpectral{ quantum_efficiency };
    }

    /**
     * @brief Sets wavelength-dependent quantum efficiency
     * @param quantum_efficiency Spectral quantum efficiency curve
     * @details Sets wavelength-dependent quantum efficiency for realistic sensor modeling.
     *          Only affects default photosite configuration.
     * @throws std::invalid_argument if any quantum_efficiency values are not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeQuantumEfficiency(TSpectral quantum_efficiency)
    {
        vira::utils::validatePositiveDefinite(quantum_efficiency, "Quantum Efficiency");
        default_photosite_config_.quantum_efficiency = quantum_efficiency;
    }

    /**
     * @brief Sets RGB quantum efficiency values for fast computation
     * @param quantum_efficiency_rgb RGB quantum efficiency values
     * @details Sets RGB-specific quantum efficiency values for efficient RGB rendering paths.
     *          Only affects default photosite configuration.
     * @throws std::invalid_argument if quantum_efficiency_rgb values are not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeQuantumEfficiencyRGB(ColorRGB quantum_efficiency_rgb)
    {
        vira::utils::validatePositiveDefinite(quantum_efficiency_rgb, "Quantum Efficiency RGB");
        default_photosite_config_.quantum_efficiency_rgb = quantum_efficiency_rgb;
    }

    /**
     * @brief Sets quantum efficiency from wavelength-value pairs
     * @tparam UnitType Wavelength unit type (e.g., nanometers, micrometers)
     * @param wavelengths Vector of wavelength values
     * @param quantum_efficiency Vector of corresponding quantum efficiency values
     * @details Interpolates quantum efficiency data to match the spectral sampling.
     *          Only affects default photosite configuration.
     * @throws std::invalid_argument if any quantum_efficiency values are not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsUnit UnitType>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeQuantumEfficiency(std::vector<UnitType> wavelengths, const std::vector<float>& quantum_efficiency)
    {
        for (auto& val : quantum_efficiency) {
            vira::utils::validatePositiveDefinite(val, "Quantum Efficiency");
        }
        default_photosite_config_.setQuantumEfficiency(wavelengths, quantum_efficiency);
    }

    /**
     * @brief Sets the linear scale factor for photosite output normalization
     * @param linear_scale_factor Linear scaling factor (must be positive)
     * @details Controls the overall scaling of the photosite output signal.
     *          Only affects default photosite configuration.
     * @throws std::invalid_argument if linear_scale_factor is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultPhotositeLinearScaleFactor(double linear_scale_factor)
    {
        vira::utils::validatePositiveDefinite(linear_scale_factor, "Linear Scale Factor");
        default_photosite_config_.linear_scale_factor = static_cast<float>(linear_scale_factor);
    }

    // ============================== //
    // === Aperture Configuration === //
    // ============================== //
    /**
     * @brief Sets a custom aperture implementation
     * @param custom_aperture Unique pointer to custom aperture object
     * @details Replaces the default circular aperture with a user-provided implementation.
     *          Takes ownership of the provided aperture object.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setCustomAperture(std::unique_ptr<Aperture<TSpectral, TFloat>> custom_aperture)
    {
        aperture_ = std::move(custom_aperture);
    }

    /**
     * @brief Gets reference to the camera's aperture for direct configuration
     * @return Reference to the aperture object
     * @throws std::runtime_error if aperture is not initialized
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Aperture<TSpectral, TFloat>& Camera<TSpectral, TFloat, TMeshFloat>::aperture()
    {
        if (!aperture_) {
            throw std::runtime_error("Aperture not initialized");
        }
        return *aperture_;
    }

    // ========================= //
    // === PSF Configuration === //
    // ========================= //
    /**
     * @brief Sets a custom point spread function implementation
     * @param custom_psf Unique pointer to custom PSF object
     * @details Replaces the default PSF with a user-provided implementation.
     *          Takes ownership of the provided PSF object.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setCustomPSF(std::unique_ptr<PointSpreadFunction<TSpectral, TFloat>> custom_psf)
    {
        psf_ = std::move(custom_psf);
    }

    /**
     * @brief Gets reference to the camera's PSF for direct configuration
     * @return Reference to the point spread function object
     * @throws std::runtime_error if PSF is not initialized
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    PointSpreadFunction<TSpectral, TFloat>& Camera<TSpectral, TFloat, TMeshFloat>::psf()
    {
        if (!psf_) {
            throw std::runtime_error("PointSpreadFunction not initialized");
        }
        return *psf_;
    }

    /**
     * @brief Configures camera to use physically-based Airy disk PSF
     * @details Sets the default PSF to use diffraction-limited Airy disk pattern
     *          based on aperture diameter and wavelength. PSF will be created
     *          automatically during camera initialization.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultAiryDiskPSF()
    {
        default_psf_option_ = DefaultPSFOption::AIRY_DISK_PSF;
    }

    /**
     * @brief Configures camera to use Gaussian approximation PSF
     * @details Sets the default PSF to use Gaussian approximation of the Airy disk.
     *          Faster than true Airy disk but less physically accurate. PSF will be
     *          created automatically during camera initialization.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultGaussianPSF()
    {
        default_psf_option_ = DefaultPSFOption::GAUSSIAN_PSF;
    }

    /**
     * @brief Sets a circular Gaussian PSF with uniform sigma
     * @param sigma_x Standard deviation in both X and Y directions
     * @param angle Rotation angle in degrees (default: 0)
     * @details Creates a circular Gaussian PSF with the same width in both directions.
     * @throws std::invalid_argument if sigma_x is not positive definite or angle is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setGaussianPSF(TSpectral sigma_x, float angle)
    {
        vira::utils::validatePositiveDefinite(sigma_x, "Gaussian PSF Sigma X");
        vira::utils::validateFinite(angle, "Gaussian PSF Angle");
        psf_ = std::make_unique<GaussianPSF<TSpectral, TFloat>>(sigma_x, angle);
    }

    /**
     * @brief Sets an elliptical Gaussian PSF with different X and Y sigmas
     * @param sigma_x Standard deviation in X direction
     * @param sigma_y Standard deviation in Y direction
     * @param angle Rotation angle in degrees (default: 0)
     * @details Creates an elliptical Gaussian PSF for modeling astigmatism or other aberrations.
     * @throws std::invalid_argument if sigma values are not positive definite or angle is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setGaussianPSF(TSpectral sigma_x, TSpectral sigma_y, float angle)
    {
        vira::utils::validatePositiveDefinite(sigma_x, "Gaussian PSF Sigma X");
        vira::utils::validatePositiveDefinite(sigma_y, "Gaussian PSF Sigma Y");
        vira::utils::validateFinite(angle, "Gaussian PSF Angle");
        psf_ = std::make_unique<GaussianPSF<TSpectral, TFloat>>(sigma_x, sigma_y, angle);
    }

    /**
     * @brief Sets a Gaussian PSF using scalar sigma values
     * @param sigma_x Standard deviation in X direction (scalar)
     * @param sigma_y Standard deviation in Y direction (scalar)
     * @param angle Rotation angle in degrees (default: 0)
     * @details Creates a Gaussian PSF with constant (wavelength-independent) widths.
     * @throws std::invalid_argument if sigma values are not positive definite or angle is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setGaussianPSF(float sigma_x, float sigma_y, float angle)
    {
        vira::utils::validatePositiveDefinite(sigma_x, "Gaussian PSF Sigma X");
        vira::utils::validatePositiveDefinite(sigma_y, "Gaussian PSF Sigma Y");
        vira::utils::validateFinite(angle, "Gaussian PSF Angle");
        psf_ = std::make_unique<GaussianPSF<TSpectral, TFloat>>(sigma_x, sigma_y, angle);
    }

    // ================================ //
    // === Distortion Configuration === //
    // ================================ //
    /**
     * @brief Sets a custom lens distortion model
     * @param custom_distortion Unique pointer to custom distortion object
     * @details Replaces the default distortion model with a user-provided implementation.
     *          Takes ownership of the provided distortion object. Triggers camera
     *          reinitialization to update projection matrices.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setCustomDistortion(std::unique_ptr<Distortion<TSpectral, TFloat>> custom_distortion)
    {
        distortion_ = std::move(custom_distortion);
        needs_initialization_ = true;
    }

    /**
     * @brief Gets reference to the camera's distortion model for direct configuration
     * @return Reference to the distortion object
     * @throws std::runtime_error if distortion is not initialized
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Distortion<TSpectral, TFloat>& Camera<TSpectral, TFloat, TMeshFloat>::distortion()
    {
        if (!distortion_) {
            throw std::runtime_error("Distortion not initialized");
        }
        return *distortion_;
    }

    /**
     * @brief Sets Brown-Conrady distortion model with specified coefficients
     * @param brown_coefficients Brown-Conrady distortion coefficients
     * @details Configures radial and tangential distortion using the Brown-Conrady model.
     *          Triggers camera reinitialization to update projection matrices.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setBrownDistortionCoefficients(BrownCoefficients brown_coefficients)
    {
        distortion_ = std::make_unique<BrownDistortion<TSpectral, TFloat>>(brown_coefficients);
        needs_initialization_ = true;
    }

    /**
     * @brief Sets Owen distortion model with specified coefficients
     * @param owen_coefficients Owen distortion coefficients
     * @details Configures distortion using the Owen model for specialized lens systems.
     *          Triggers camera reinitialization to update projection matrices.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setOwenDistortionCoefficients(OwenCoefficients owen_coefficients)
    {
        distortion_ = std::make_unique<OwenDistortion<TSpectral, TFloat>>(owen_coefficients);
        needs_initialization_ = true;
    }

    /**
     * @brief Sets OpenCV distortion model with specified coefficients
     * @param opencv_coefficients OpenCV distortion coefficients
     * @details Configures comprehensive distortion using the OpenCV model including
     *          rational radial, tangential, and thin prism distortion components.
     *          Triggers camera reinitialization to update projection matrices.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setOpenCVDistortionCoefficients(OpenCVCoefficients opencv_coefficients)
    {
        distortion_ = std::make_unique<OpenCVDistortion<TSpectral, TFloat>>(opencv_coefficients);
        needs_initialization_ = true;
    }

    // ================================= //
    // === Noise Model Configuration === //
    // ================================= //
    /**
     * @brief Sets a custom noise model implementation
     * @param custom_noise_model Unique pointer to custom noise model object
     * @details Replaces the default noise model with a user-provided implementation.
     *          Takes ownership of the provided noise model object.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setCustomNoiseModel(std::unique_ptr<NoiseModel> custom_noise_model)
    {
        noise_model_ = std::move(custom_noise_model);
    }

    /**
     * @brief Gets reference to the camera's noise model for direct configuration
     * @return Reference to the noise model object
     * @throws std::runtime_error if noise model is not initialized
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    NoiseModel& Camera<TSpectral, TFloat, TMeshFloat>::noiseModel()
    {
        if (!noise_model_) {
            throw std::runtime_error("NoiseModel not initialized");
        }
        return *noise_model_;
    }

    /**
     * @brief Configures low-noise camera settings with typical parameters
     * @details Sets up noise model with low dark current and readout noise
     *          typical of high-quality scientific cameras.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultLowNoise()
    {
        default_noise_model_config_.dark_current = 4.5f;
        default_noise_model_config_.readout_noise_mean = 30.f;
        default_noise_model_config_.readout_noise_std = std::sqrt(30.f);
        noise_model_configured_ = true;
    }

    /**
     * @brief Configures noise model with fixed pattern noise characteristics
     * @details Sets up noise model with low base noise plus sinusoidal fixed
     *          pattern noise to simulate CMOS sensor artifacts.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultFixedPatternNoise()
    {
        default_noise_model_config_.dark_current = 4.5f;
        default_noise_model_config_.readout_noise_mean = 30.f;
        default_noise_model_config_.readout_noise_std = std::sqrt(30.f);

        default_noise_model_config_.horizontal_scale = 0.05f;
        default_noise_model_config_.vertical_scale = 0.05f;
        default_noise_model_config_.horizontal_pattern_period = 5;
        default_noise_model_config_.vertical_pattern_period = 5;
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the dark current noise level
     * @param dark_current Dark current in electrons per second (must be finite)
     * @details Controls the thermally-generated noise level in the sensor.
     * @throws std::invalid_argument if dark_current is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultDarkCurrent(double dark_current)
    {
        vira::utils::validateFinite(dark_current, "Dark Current");
        default_noise_model_config_.dark_current = static_cast<float>(dark_current);
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the mean readout noise level
     * @param readout_noise_mean Mean readout noise in electrons (must be finite)
     * @details Sets the average readout noise added during signal digitization.
     * @throws std::invalid_argument if readout_noise_mean is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultReadoutNoiseMean(double readout_noise_mean)
    {
        vira::utils::validateFinite(readout_noise_mean, "Readout Noise Mean");
        default_noise_model_config_.readout_noise_mean = static_cast<float>(readout_noise_mean);
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the readout noise standard deviation
     * @param readout_noise_std Standard deviation of readout noise in electrons (must be finite)
     * @details Controls the variability of readout noise across pixels and exposures.
     * @throws std::invalid_argument if readout_noise_std is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultReadoutNoiseSTD(double readout_noise_std)
    {
        vira::utils::validateFinite(readout_noise_std, "Readout Noise STD");
        default_noise_model_config_.readout_noise_std = static_cast<float>(readout_noise_std);
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the horizontal fixed pattern noise scale
     * @param horizontal_fixed_pattern_scale Scale factor for horizontal pattern noise (must be finite)
     * @details Controls the amplitude of sinusoidal noise patterns in horizontal direction.
     * @throws std::invalid_argument if horizontal_fixed_pattern_scale is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultHorizontalFixedPatternScale(double horizontal_fixed_pattern_scale)
    {
        vira::utils::validateFinite(horizontal_fixed_pattern_scale, "Horizontal Fixed Pattern Scale");
        default_noise_model_config_.horizontal_scale = static_cast<float>(horizontal_fixed_pattern_scale);
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the vertical fixed pattern noise scale
     * @param vertical_fixed_pattern_scale Scale factor for vertical pattern noise (must be finite)
     * @details Controls the amplitude of sinusoidal noise patterns in vertical direction.
     * @throws std::invalid_argument if vertical_fixed_pattern_scale is not finite
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultVerticalFixedPatternScale(double vertical_fixed_pattern_scale)
    {
        vira::utils::validateFinite(vertical_fixed_pattern_scale, "Vertical Fixed Pattern Scale");
        default_noise_model_config_.vertical_scale = static_cast<float>(vertical_fixed_pattern_scale);
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the horizontal fixed pattern noise period
     * @param horizontal_fixed_pattern_period Period of horizontal pattern in pixels
     * @details Controls the spatial frequency of horizontal fixed pattern noise.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultHorizontalFixedPatternPeriod(size_t horizontal_fixed_pattern_period)
    {
        default_noise_model_config_.horizontal_pattern_period = horizontal_fixed_pattern_period;
        noise_model_configured_ = true;
    }

    /**
     * @brief Sets the vertical fixed pattern noise period
     * @param vertical_fixed_pattern_period Period of vertical pattern in pixels
     * @details Controls the spatial frequency of vertical fixed pattern noise.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultVerticalFixedPatternPeriod(size_t vertical_fixed_pattern_period)
    {
        default_noise_model_config_.vertical_pattern_period = vertical_fixed_pattern_period;
        noise_model_configured_ = true;
    }



    // ============================== //
    // === Filter Mosaic Settings === //
    // ============================== //

    /**
     * @brief Sets a custom color filter array mosaic
     * @param filter_mosaic Spectral filter mosaic image
     * @details Replaces the default filter mosaic with a custom one. The mosaic
     *          resolution must exactly match the camera resolution.
     * @throws std::runtime_error if filter_mosaic resolution doesn't match camera resolution
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setCustomFilterMosaic(vira::images::Image<TSpectral> filter_mosaic)
    {
        if (filter_mosaic.resolution() != resolution_) {
            throw std::runtime_error("Provided filter mosaic resolution (" +
                std::to_string(filter_mosaic.resolution().x) + "x" + std::to_string(filter_mosaic.resolution().y) + ")" +
                " does not match the camera resolution (" + std::to_string(resolution_.x) + "x" + std::to_string(resolution_.y) + ")"
            );
        }
        filter_mosaic_ = filter_mosaic;
    }

    /**
     * @brief Sets custom Bayer filter with specified spectral responses
     * @param red_filter Spectral transmission curve for red filter elements
     * @param green_filter Spectral transmission curve for green filter elements
     * @param blue_filter Spectral transmission curve for blue filter elements
     * @details Configures a Bayer color filter array with custom spectral responses.
     *          Enables Bayer filtering and marks filters as custom.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultBayerFilter(TSpectral red_filter, TSpectral green_filter, TSpectral blue_filter)
    {
        default_bayer_red_ = red_filter;
        default_bayer_green_ = green_filter;
        default_bayer_blue_ = blue_filter;
        use_bayer_filter_ = true;
        custom_filters_ = true;
    }

    /**
     * @brief Enables standard Bayer filter with default RGB spectral responses
     * @details Enables Bayer filtering using standard RGB spectral conversion.
     *          Filter responses will be generated automatically during initialization.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::setDefaultBayerFilter()
    {
        use_bayer_filter_ = true;
    }

    // ================================== //
    // === Projection and Ray Casting === //
    // ================================== //

    /**
     * @brief Projects a 3D point in camera coordinates to pixel coordinates
     * @param camera_point 3D point in camera coordinate system
     * @return Corresponding pixel coordinates in the image
     * @details Performs perspective projection including lens distortion if configured.
     *          Handles Blender coordinate system compatibility when enabled.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Pixel Camera<TSpectral, TFloat, TMeshFloat>::projectCameraPoint(vec3<TFloat> camera_point) const
    {
        // Perform perspective division to get normalized coordinates
        Pixel homogeneous{ camera_point[0] / camera_point[2], camera_point[1] / camera_point[2] };

        // Apply lens distortion if configured
        if (distortion_ != nullptr) {
            homogeneous = distortion_->distort(homogeneous);
        }

        // Apply intrinsic matrix to get pixel coordinates
        auto point = intrinsic_matrix_ * vec3<TFloat>{homogeneous, 1};

        // Handle Blender coordinate system compatibility
        if (blender_frame_) {
            point.x = static_cast<float>(resolution_.x - 1) - point.x;
        }

        return point;
    }

    /**
     * @brief Projects a 3D point in world coordinates to pixel coordinates
     * @param world_point 3D point in world coordinate system
     * @return Corresponding pixel coordinates in the image
     * @details Transforms the world point to camera coordinates then projects to pixels.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Pixel Camera<TSpectral, TFloat, TMeshFloat>::projectWorldPoint(vec3<TFloat> world_point) const
    {
        mat4<TFloat> view_matrix = this->getViewMatrix();
        vec3<TFloat> camera_point = vec3<TFloat>(view_matrix * vec4<TFloat>(world_point, 1.f));
        return this->projectCameraPoint(camera_point);
    }

    /**
     * @brief Converts pixel coordinates to ray direction in camera space
     * @param pixel Pixel coordinates in the image
     * @return Normalized ray direction vector in camera coordinate system
     * @details Uses precomputed interpolation if enabled for distorted cameras,
     *          otherwise computes direction directly. Handles lens distortion correction.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> Camera<TSpectral, TFloat, TMeshFloat>::pixelToDirection(Pixel pixel) const
    {
        vec3<TFloat> direction{ 0, 0, 0 };
        if (interpolate_directions_) {
            // Use precomputed interpolated directions for efficiency
            direction = precomputed_pixel_directions_.interpolatePixel(pixel);
        }
        else {
            // Compute direction directly
            direction = pixelToDirectionHelper(pixel);
        }
        return direction;
    }

    /**
     * @brief Generates a ray from pixel coordinates (pinhole camera model)
     * @param pixel Pixel coordinates in the image
     * @return Ray originating from camera position through the specified pixel
     * @details Creates a ray for pinhole camera model without depth of field effects.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::rendering::Ray<TSpectral, TFloat> Camera<TSpectral, TFloat, TMeshFloat>::pixelToRay(Pixel pixel) const
    {
        vec3<TFloat> origin = this->getGlobalPosition();
        vec3<TFloat> direction = this->pixelToDirection(pixel);
        return vira::rendering::Ray<TSpectral, TFloat>{origin, normalize(this->localDirectionToGlobal(direction))};
    }

    /**
     * @brief Generates a ray with depth of field effects from pixel coordinates
     * @param pixel Pixel coordinates in the image
     * @param scene_rng Random number generator for aperture sampling
     * @param dist Uniform distribution for random sampling
     * @return Ray with aperture sampling for depth of field simulation
     * @details Samples a random point on the aperture and adjusts ray origin and direction
     *          to simulate depth of field effects based on focus distance and aperture size.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::rendering::Ray<TSpectral, TFloat> Camera<TSpectral, TFloat, TMeshFloat>::pixelToRay(Pixel pixel, std::mt19937& scene_rng, std::uniform_real_distribution<float>& dist) const
    {
        vec3<TFloat> origin = this->getGlobalPosition();
        vec3<TFloat> direction = this->pixelToDirection(pixel);

        if (depth_of_field_) {
            // Sample random point on aperture for depth of field
            vec2<float> aperture_point = aperture_->samplePoint(scene_rng, dist);
            vec3<TFloat> offset{ aperture_point[0], aperture_point[1], focal_length_ }; // Local coordinates

            // Offset ray origin by aperture sample
            origin += this->getGlobalRotation() * offset;

            // Adjust ray direction to maintain focus at specified distance
            if (!std::isinf(focus_distance_)) {
                direction = (focus_distance_ * normalize(direction)) - offset;
            }
        }

        return vira::rendering::Ray<TSpectral, TFloat>{origin, normalize(this->localDirectionToGlobal(direction))};
    }

    /**
     * @brief Calculates received optical power for a specific pixel
     * @param radiance Incident spectral radiance (W/m2/sr)
     * @param i Pixel column index
     * @param j Pixel row index
     * @return Spectral power received by the pixel (W)
     * @details Combines optical efficiency, aperture area, and pixel solid angle
     *          to compute the total power incident on a specific pixel.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral Camera<TSpectral, TFloat, TMeshFloat>::calculateReceivedPower(TSpectral radiance, int i, int j) const
    {
        return optical_efficiency_ * aperture_->getArea() * pixel_solid_angle_(i, j) * radiance;
    }

    /**
     * @brief Calculates received optical power from irradiance
     * @param irradiance Incident spectral irradiance (W/m^2)
     * @return Spectral power received by the aperture (W)
     * @details Computes power for uniform illumination scenarios where
     *          the solid angle dependence is not needed.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral Camera<TSpectral, TFloat, TMeshFloat>::calculateReceivedPowerIrr(TSpectral irradiance) const
    {
        return optical_efficiency_ * aperture_->getArea() * irradiance;
    }

    /**
     * @brief Computes minimum detectable irradiance based on noise characteristics
     * @return Minimum detectable irradiance (W/m^2)
     * @details Estimates the detection threshold based on a minimum electron count
     *          and the camera's optical and sensor characteristics.
     * @todo Implement proper noise-based calculation using actual noise parameters
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float Camera<TSpectral, TFloat, TMeshFloat>::computeMinimumDetectableIrradiance() const
    {
        float minimum_electrons = 10.f; // TODO: Compute from actual noise parameters
        // TODO: Use actual photosite quantum efficiency
        // TSpectral QE = photosite_->getQuantumEfficiency();
        // TSpectral responsivity = TSpectral::photonEnergies * QE;
        TSpectral responsivity = TSpectral::photonEnergies; // Assumes 100% QE
        float minimum_energy = minimum_electrons * responsivity.integrate();

        return minimum_energy / (aperture_->getArea() * exposure_time_);
    }

    /**
     * @brief Simulates complete sensor response including noise
     * @param total_power_image Spectral power image incident on sensor
     * @return Simulated sensor output image (normalized 0-1)
     * @details Converts optical power to photon counts, applies sensor noise,
     *          and simulates photosite response to produce final digital image.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<float> Camera<TSpectral, TFloat, TMeshFloat>::simulateSensor(const vira::images::Image<TSpectral>& total_power_image) const
    {
        // Convert optical power to photon counts
        vira::images::Image<TSpectral> received_photons = getPhotonCounts(total_power_image);

        // Generate noise for each pixel
        vira::images::Image<float> noise_counts(resolution_, 0);
        if (hasNoiseModel()) {
            for (size_t i = 0; i < static_cast<size_t>(resolution_.x); ++i) {
                for (size_t j = 0; j < static_cast<size_t>(resolution_.y); ++j) {
                    noise_counts(i, j) = noise_model_->simulate(rng_, i, j, exposure_time_);
                }
            }
        }

        // Simulate photosite response
        vira::images::Image<float> output_image(resolution_, 0);
        for (size_t i = 0; i < received_photons.size(); ++i) {
            output_image[i] = photosite_->exposePixel(received_photons[i], noise_counts[i]);
        }

        return output_image;
    }

    /**
     * @brief Simulates RGB sensor response including noise
     * @param total_power_image Spectral power image incident on sensor
     * @return Simulated RGB sensor output image (normalized 0-1)
     * @details Fast RGB simulation path that converts spectral data to RGB
     *          and processes each color channel independently.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Camera<TSpectral, TFloat, TMeshFloat>::simulateSensorRGB(const vira::images::Image<TSpectral>& total_power_image) const
    {
        // Convert optical power to RGB photon counts
        vira::images::Image<ColorRGB> received_photons = getPhotonCountsRGB(total_power_image);

        // Generate RGB noise for each pixel
        vira::images::Image<ColorRGB> noise_counts(resolution_, ColorRGB{ 0 });
        if (hasNoiseModel()) {
            for (size_t i = 0; i < static_cast<size_t>(resolution_.x); ++i) {
                for (size_t j = 0; j < static_cast<size_t>(resolution_.y); ++j) {
                    ColorRGB noise{ 0, 0, 0 };
                    noise[0] = noise_model_->simulate(rng_, i, j, exposure_time_);
                    noise[1] = noise_model_->simulate(rng_, i, j, exposure_time_);
                    noise[2] = noise_model_->simulate(rng_, i, j, exposure_time_);
                    noise_counts(i, j) = noise;
                }
            }
        }

        // Simulate RGB photosite response
        vira::images::Image<ColorRGB> output_image(resolution_, 0);
        for (size_t i = 0; i < received_photons.size(); ++i) {
            output_image[i] = photosite_->exposePixelRGB(received_photons[i], noise_counts[i]);
        }

        return output_image;
    }

    /**
     * @brief Converts received optical power to photon counts with optional noise
     * @param received_power Spectral power image (W)
     * @return Photon count image for each wavelength band
     * @details Converts power to energy using exposure time, then to photons using
     *          photon energies. Applies filter mosaic effects and optional Poisson noise.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<TSpectral> Camera<TSpectral, TFloat, TMeshFloat>::getPhotonCounts(const vira::images::Image<TSpectral>& received_power) const
    {
        vira::images::Image<TSpectral> photon_counts(resolution_, TSpectral{ 0 });

        if (hasFilterMosaic()) {
            // Apply color filter array
            for (size_t i = 0; i < received_power.size(); ++i) {
                TSpectral received_energy = exposure_time_ * received_power[i] * filter_mosaic_[i];
                TSpectral photon_count = received_energy / TSpectral::photonEnergies;

                // Apply Poisson photon noise if enabled
                if (photon_count.total() >= 1 && simulate_photon_noise_) {
                    for (size_t j = 0; j < TSpectral::size(); ++j) {
                        std::poisson_distribution<int> dist(photon_count[j]);
                        photon_count[j] = static_cast<float>(dist(rng_));
                    }
                }

                photon_counts[i] = photon_count;
            }
        }
        else {
            // No filter mosaic - direct spectral to photon conversion
            for (size_t i = 0; i < received_power.size(); ++i) {
                TSpectral received_energy = exposure_time_ * received_power[i];
                TSpectral photon_count = received_energy / TSpectral::photonEnergies;

                // Apply Poisson photon noise if enabled
                if (photon_count.total() >= 1 && simulate_photon_noise_) {
                    for (size_t j = 0; j < TSpectral::size(); ++j) {
                        std::poisson_distribution<int> dist(photon_count[j]);
                        photon_count[j] = static_cast<float>(dist(rng_));
                    }
                }

                photon_counts[i] = photon_count;
            }
        }

        return photon_counts;
    }

    /**
     * @brief Converts received optical power to RGB photon counts with optional noise
     * @param received_power Spectral power image (W)
     * @return RGB photon count image
     * @details Fast RGB conversion path that applies spectral-to-RGB transformation
     *          and computes photon counts for each RGB channel independently.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::images::Image<ColorRGB> Camera<TSpectral, TFloat, TMeshFloat>::getPhotonCountsRGB(const vira::images::Image<TSpectral>& received_power) const
    {
        vira::images::Image<ColorRGB> photon_counts(resolution_, ColorRGB{ 0 });

        if (hasFilterMosaic()) {
            // Apply color filter array and convert to RGB
            for (size_t i = 0; i < received_power.size(); ++i) {
                ColorRGB received_rgb = spectral_to_rgb_(received_power[i] * filter_mosaic_[i]);
                ColorRGB received_energy = exposure_time_ * received_rgb;
                ColorRGB photon_count = received_energy / ColorRGB::photonEnergies;

                // Apply Poisson photon noise if enabled
                if (photon_count.total() >= 1 && simulate_photon_noise_) {
                    for (size_t j = 0; j < ColorRGB::size(); ++j) {
                        std::poisson_distribution<int> dist(static_cast<double>(photon_count[j]));
                        photon_count[j] = static_cast<float>(dist(rng_));
                    }
                }

                photon_counts[i] = photon_count;
            }
        }
        else {
            // Direct spectral to RGB conversion
            for (size_t i = 0; i < received_power.size(); ++i) {
                ColorRGB received_rgb = spectral_to_rgb_(received_power[i]);
                ColorRGB received_energy = exposure_time_ * received_rgb;
                ColorRGB photon_count = received_energy / ColorRGB::photonEnergies;

                // Apply Poisson photon noise if enabled
                if (photon_count.total() >= 1 && simulate_photon_noise_) {
                    for (size_t j = 0; j < ColorRGB::size(); ++j) {
                        std::poisson_distribution<int> dist(static_cast<double>(photon_count[j]));
                        photon_count[j] = static_cast<float>(dist(rng_));
                    }
                }

                photon_counts[i] = photon_count;
            }
        }

        return photon_counts;
    }

    // ============================= //
    // === Geometry Computations === //
    // ============================= //
    /**
     * @brief Tests if a point is behind the camera
     * @param point 3D point in camera coordinate system
     * @return True if the point is behind the camera, false otherwise
     * @details Uses the camera's Z-direction to determine if a point is behind
     *          the camera plane, accounting for coordinate system orientation.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Camera<TSpectral, TFloat, TMeshFloat>::behind(const vec3<TFloat>& point) const
    {
        return (-z_dir_ * point[2]) >= 0;
    }

    /**
     * @brief Calculates Ground Sample Distance (GSD) at a given distance
     * @param distance Distance from camera to target in meters
     * @return Ground sample distance in meters per pixel
     * @details Computes the spatial resolution achievable at the specified distance,
     *          useful for mission planning and image analysis. Returns the minimum
     *          GSD between X and Y directions to ensure coverage requirements.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TFloat Camera<TSpectral, TFloat, TMeshFloat>::calculateGSD(TFloat distance) const
    {
        TFloat gsd_x = distance * sensor_size_.x / (focal_length_ * static_cast<TFloat>(resolution_.x));
        TFloat gsd_y = distance * sensor_size_.y / (focal_length_ * static_cast<TFloat>(resolution_.y));

        return std::min(gsd_x, gsd_y);
    }

    /**
     * @brief Calculates the camera's field of view angles
     * @return Field of view in radians (X, Y)
     * @details Computes the angular field of view based on sensor size and focal length.
     *          Useful for determining camera coverage and positioning requirements.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec2<TFloat> Camera<TSpectral, TFloat, TMeshFloat>::getFOV() const
    {
        TFloat fov_x = 2 * std::atan2(sensor_size_.x / 2, focal_length_);
        TFloat fov_y = 2 * std::atan2(sensor_size_.y / 2, focal_length_);

        return vec2<TFloat>{fov_x, fov_y};
    }

    /**
     * @brief Points the camera toward a target position
     * @param target Target position in world coordinates
     * @param up Up vector for camera orientation (default: Z-up)
     * @details Calculates the direction from camera to target and orients
     *          the camera accordingly while maintaining the specified up vector.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::lookAt(vec3<TFloat> target, vec3<TFloat> up)
    {
        vec3<TFloat> direction = target - this->getGlobalPosition();
        this->lookInDirection(direction, up);
    }

    /**
     * @brief Orients the camera in a specific direction
     * @param direction Direction vector for camera to look toward
     * @param up Up vector for camera orientation (default: Z-up)
     * @details Sets camera rotation to look in the specified direction while
     *          maintaining the up vector. Handles edge cases where direction
     *          and up vectors are aligned by selecting alternative up vectors.
     *          Supports both standard and Blender coordinate systems.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::lookInDirection(vec3<TFloat> direction, vec3<TFloat> up)
    {
        Rotation<TFloat> rotation;
        vec3<TFloat> y_axis;
        vec3<TFloat> z_axis;

        up = normalize(up);
        direction = normalize(direction);

        // Handle degenerate case when direction and up are aligned
        TFloat alignment = std::abs(dot(direction, up));
        const TFloat alignment_threshold = TFloat(0.999);

        if (alignment > alignment_threshold) {
            // Try alternative up vectors to avoid gimbal lock
            vec3<TFloat> world_up_candidates[3] = {
                vec3<TFloat>(0, 1, 0),  // Y-up
                vec3<TFloat>(0, 0, 1),  // Z-up
                vec3<TFloat>(1, 0, 0)   // X-up
            };

            for (const auto& candidate : world_up_candidates) {
                if (std::abs(dot(direction, candidate)) < alignment_threshold) {
                    up = candidate;
                    break;
                }
            }
        }

        // Set up coordinate system based on frame convention
        if (blender_frame_) {
            y_axis = up;
            z_axis = -direction;  // Blender uses negative Z forward
        }
        else {
            y_axis = -up;         // Standard computer vision convention
            z_axis = direction;
        }

        // Construct orthonormal basis
        vec3<TFloat> x_axis = normalize(cross(y_axis, z_axis));
        y_axis = normalize(cross(z_axis, x_axis));

        rotation = Rotation<TFloat>(x_axis, y_axis, z_axis);
        this->setLocalRotation(rotation);
    }

    /**
     * @brief Tests if an oriented bounding box is within the camera's field of view
     * @param obb Oriented bounding box to test
     * @return True if the OBB intersects the camera frustum, false otherwise
     * @details Uses precomputed frustum planes for efficient culling operations.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Camera<TSpectral, TFloat, TMeshFloat>::obbInView(const vira::rendering::OBB<TFloat>& obb) const
    {
        return intersectsFrustum(obb, frustum_planes_);
    }


    // ========================= //
    // === Private Functions === //
    // ========================= //

    /**
     * @brief Initializes the camera intrinsic matrix and its inverse
     * @details Computes the intrinsic camera matrix from focal length, sensor size,
     *          resolution, and calibration parameters. Handles coordinate system
     *          conventions and computes the analytical inverse for efficiency.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::initializeIntrinsicMatrix()
    {
        // Compute pixel scaling factors (pixels per unit length)
        kx_ = static_cast<TFloat>(resolution_.x) / sensor_size_[0];
        ky_ = static_cast<TFloat>(resolution_.y) / sensor_size_[1];

        // Set default principal point to image center if not specified
        if (std::isinf(px_)) {
            px_ = static_cast<TFloat>(resolution_.x) / 2;
        }
        if (std::isinf(py_)) {
            py_ = static_cast<TFloat>(resolution_.y) / 2;
        }

        // Apply coordinate system transformations for Blender compatibility
        TFloat kx = kx_;
        TFloat ky = z_dir_ * ky_;
        TFloat px = z_dir_ * px_;
        TFloat py = z_dir_ * py_;
        TFloat kxy = z_dir_ * kxy_;
        TFloat kyx = z_dir_ * kyx_;

        // Compute focal length in pixel units
        TFloat fx = focal_length_ * kx;
        TFloat fy = focal_length_ * ky;
        TFloat determinant = (fx * fy) - (kxy * kyx);

        // Construct intrinsic matrix in column-major order
        intrinsic_matrix_[0][0] = fx;   intrinsic_matrix_[0][1] = kyx;
        intrinsic_matrix_[1][0] = kxy;  intrinsic_matrix_[1][1] = fy;
        intrinsic_matrix_[2][0] = px;   intrinsic_matrix_[2][1] = py;

        // Compute analytical inverse intrinsic matrix
        intrinsic_matrix_inv_[0][0] = fy / determinant;
        intrinsic_matrix_inv_[1][0] = -kxy / determinant;
        intrinsic_matrix_inv_[2][0] = ((kxy * py) - (fy * px)) / determinant;
        intrinsic_matrix_inv_[0][1] = -kyx / determinant;
        intrinsic_matrix_inv_[1][1] = fx / determinant;
        intrinsic_matrix_inv_[2][1] = ((kyx * px) - (fx * py)) / determinant;

        // Create double precision version for high-accuracy computations
        intrinsic_matrix_d_inv_ = intrinsic_matrix_inv_;
    }

    /**
     * @brief Computes tangent vector for spherical triangle solid angle calculation
     * @param p0 First point on unit sphere
     * @param p1 Second point on unit sphere
     * @return Tangent vector at p0 toward p1
     * @details Helper function for computing solid angles using spherical geometry.
     */
    template <IsFloat TFloat>
    vec3<TFloat> tangent(vec3<TFloat>& p0, vec3<TFloat> p1)
    {
        vec3<TFloat> p = p1 - p0;
        vec3<TFloat> r = cross(p0, p);
        vec3<TFloat> t = cross(r, p0);
        return normalize(t);
    }

    /**
     * @brief Computes solid angle subtended by a spherical triangle
     * @param c0 First vertex of triangle on unit sphere
     * @param c1 Second vertex of triangle on unit sphere
     * @param c2 Third vertex of triangle on unit sphere
     * @return Solid angle in steradians
     * @details Uses Girard's theorem: solid angle = sum of interior angles - pi
     */
    template <IsFloat TFloat>
    TFloat triangleSolidAngle(vec3<TFloat>& c0, vec3<TFloat>& c1, vec3<TFloat>& c2)
    {
        // Compute interior angles using tangent vectors
        vec3<TFloat> t01 = tangent(c0, c1);
        vec3<TFloat> t02 = tangent(c0, c2);
        TFloat angle0 = std::acos(dot(t01, t02));

        vec3<TFloat> t10 = tangent(c1, c0);
        vec3<TFloat> t12 = tangent(c1, c2);
        TFloat angle1 = std::acos(dot(t10, t12));

        vec3<TFloat> t20 = tangent(c2, c0);
        vec3<TFloat> t21 = tangent(c2, c1);
        TFloat angle2 = std::acos(dot(t20, t21));

        // Apply Girard's theorem
        return angle0 + angle1 + angle2 - PI<TFloat>();
    }

    /**
     * @brief Precomputes solid angles for all pixels in the image
     * @details Calculates the solid angle subtended by each pixel using spherical
     *          geometry. Uses double precision arithmetic for numerical accuracy.
     *          Can be computed in parallel for efficiency on multi-core systems.
     *          Essential for accurate radiometric calculations.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::initializePixelSolidAngle()
    {
        pixel_solid_angle_ = vira::images::Image<float>(resolution_, 0.f);

        // Use double precision for accurate solid angle computation
        if (parallel_initialization_) {
            vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)

            tbb::parallel_for(tbb::blocked_range2d<int>(0, resolution_.y, 0, resolution_.x),
                [&](const tbb::blocked_range2d<int>& range) {
                    for (int i = static_cast<int>(range.cols().begin()), i_end = static_cast<int>(range.cols().end()); i < i_end; i++) {
                        for (int j = static_cast<int>(range.rows().begin()), j_end = static_cast<int>(range.rows().end()); j < j_end; j++) {
                            // Calculate normalized directions to pixel corners
                            vec3<double> c0 = normalize(pixelToDirectionHelper_d(Pixel{ i,     j }));
                            vec3<double> c1 = normalize(pixelToDirectionHelper_d(Pixel{ i + 1, j }));
                            vec3<double> c2 = normalize(pixelToDirectionHelper_d(Pixel{ i + 1, j + 1 }));
                            vec3<double> c3 = normalize(pixelToDirectionHelper_d(Pixel{ i,     j + 1 }));

                            // Compute solid angle as sum of two triangular areas
                            double omega1 = triangleSolidAngle<double>(c0, c1, c2);
                            double omega2 = triangleSolidAngle<double>(c0, c2, c3);

                            pixel_solid_angle_(i, j) = static_cast<float>(omega1 + omega2);
                        }
                    }
                });
        }
        else {
            for (int i = 0; i < resolution_.x; ++i) {
                for (int j = 0; j < resolution_.y; ++j) {
                    // Calculate normalized directions to pixel corners
                    vec3<double> c0 = normalize(pixelToDirectionHelper_d(Pixel{ i,     j }));
                    vec3<double> c1 = normalize(pixelToDirectionHelper_d(Pixel{ i + 1, j }));
                    vec3<double> c2 = normalize(pixelToDirectionHelper_d(Pixel{ i + 1, j + 1 }));
                    vec3<double> c3 = normalize(pixelToDirectionHelper_d(Pixel{ i,     j + 1 }));

                    // Compute solid angle as sum of two triangular areas
                    double omega1 = triangleSolidAngle<double>(c0, c1, c2);
                    double omega2 = triangleSolidAngle<double>(c0, c2, c3);

                    pixel_solid_angle_(i, j) = static_cast<float>(omega1 + omega2);
                }
            }
        }
    }

    /**
     * @brief Precomputes ray directions for all pixels to enable fast interpolation
     * @details Calculates and stores normalized ray directions for every pixel,
     *          enabling fast bilinear interpolation during rendering. Particularly
     *          beneficial for cameras with complex lens distortion models.
     *          Can be computed in parallel for efficiency.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::precomputePixelDirections()
    {
        precomputed_pixel_directions_ = vira::images::Image<vec3<TFloat>>(resolution_, vec3<TFloat>{0.f});

        if (parallel_initialization_) {
            vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)

            tbb::parallel_for(tbb::blocked_range2d<int>(0, resolution_.y, 0, resolution_.x),
                [&](const tbb::blocked_range2d<int>& range) {
                    for (int i = static_cast<int>(range.cols().begin()), i_end = static_cast<int>(range.cols().end()); i < i_end; i++) {
                        for (int j = static_cast<int>(range.rows().begin()), j_end = static_cast<int>(range.rows().end()); j < j_end; j++) {
                            Pixel pixel{ i, j };
                            precomputed_pixel_directions_(i, j) = normalize(pixelToDirectionHelper(pixel));
                        }
                    }
                });
        }
        else {
            for (int i = 0; i < resolution_.x; ++i) {
                for (int j = 0; j < resolution_.y; ++j) {
                    Pixel pixel{ i, j };
                    precomputed_pixel_directions_(i, j) = normalize(pixelToDirectionHelper(pixel));
                }
            }
        }
    }

    /**
     * @brief Precomputes camera frustum planes for efficient culling operations
     * @details Calculates the four side planes of the camera frustum based on
     *          corner pixel directions. Used for rapid visibility testing of
     *          scene objects. Currently uses simple corner-based approximation.
     * @todo Extend to handle extreme barrel distortion by testing edge pixels
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Camera<TSpectral, TFloat, TMeshFloat>::precomputeFrustum()
    {
        // Compute directions to image corners
        frustum_corners_[0] = pixelToDirection(Pixel{ 0, 0 });
        frustum_corners_[1] = pixelToDirection(Pixel{ resolution_.x - 1, 0 });
        frustum_corners_[2] = pixelToDirection(Pixel{ resolution_.x - 1, resolution_.y - 1 });
        frustum_corners_[3] = pixelToDirection(Pixel{ 0, resolution_.y - 1 });

        // Create far plane corners (arbitrary distance for plane construction)
        for (size_t i = 0; i < 4; ++i) {
            frustum_corners_[i + 4] = frustum_corners_[i] * 10;
        }

        // Construct frustum planes from corner points
        frustum_planes_[0] = vira::rendering::Plane<TFloat>(frustum_corners_[0], frustum_corners_[4], frustum_corners_[3]); // left
        frustum_planes_[1] = vira::rendering::Plane<TFloat>(frustum_corners_[1], frustum_corners_[2], frustum_corners_[5]); // right
        frustum_planes_[2] = vira::rendering::Plane<TFloat>(frustum_corners_[0], frustum_corners_[1], frustum_corners_[4]); // top
        frustum_planes_[3] = vira::rendering::Plane<TFloat>(frustum_corners_[3], frustum_corners_[7], frustum_corners_[2]); // bottom
    }

    /**
     * @brief Helper function to compute pixel-to-direction conversion
     * @param pixel Pixel coordinates in the image
     * @return Ray direction vector in camera coordinate system
     * @details Applies inverse intrinsic transformation and lens distortion correction.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TFloat> Camera<TSpectral, TFloat, TMeshFloat>::pixelToDirectionHelper(Pixel pixel) const
    {
        // Apply inverse intrinsic matrix to get normalized coordinates
        Pixel homogeneous = intrinsic_matrix_inv_ * vec3<TFloat>{pixel[0], pixel[1], 1};

        // Correct for lens distortion
        if (distortion_ != nullptr) {
            homogeneous = distortion_->undistort(homogeneous);
        }

        return vec3<TFloat>{homogeneous[0], z_dir_* homogeneous[1], z_dir_};
    }

    /**
     * @brief Helper function for double-precision pixel-to-direction conversion
     * @param pixel Pixel coordinates in the image
     * @return Ray direction vector in camera coordinates (double precision)
     * @details Used specifically for accurate solid angle computations where
     *          numerical precision is critical.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<double> Camera<TSpectral, TFloat, TMeshFloat>::pixelToDirectionHelper_d(Pixel pixel) const
    {
        // Apply inverse intrinsic matrix with double precision
        vec2<double> homogeneous = intrinsic_matrix_d_inv_ * vec3<double>{pixel[0], pixel[1], 1};

        // Correct for lens distortion
        if (distortion_ != nullptr) {
            homogeneous = distortion_->undistort(homogeneous);
        }

        const double z_dir_d = static_cast<double>(z_dir_);
        return vec3<double>{homogeneous[0], z_dir_d* homogeneous[1], z_dir_d};
    }
};