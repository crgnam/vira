#ifndef VIRA_CAMERAS_CAMERA_HPP
#define VIRA_CAMERAS_CAMERA_HPP

#include <memory>
#include <cstddef>
#include <limits>
#include <functional>
#include <random>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/apertures/aperture.hpp"
#include "vira/cameras/distortions/distortion.hpp"
#include "vira/cameras/distortions/brown_distortion.hpp"
#include "vira/cameras/distortions/opencv_distortion.hpp"
#include "vira/cameras/distortions/owen_distortion.hpp"
#include "vira/cameras/photosites/photosite.hpp"
#include "vira/cameras/psfs/psf.hpp"
#include "vira/reference_frame.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/frustum.hpp"
#include "vira/cameras/noise_models/noise_model.hpp"

// Forward Declaration:
namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;

    namespace scene {
        template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
        class Group;
    }
}

namespace vira::cameras {
    /**
     * @brief Comprehensive camera model for realistic image rendering
     * @tparam TSpectral Spectral data type for wavelength-dependent simulation
     * @tparam TFloat Floating point precision for camera calculations
     * @tparam TMeshFloat Floating point precision for mesh data (must be >= TFloat)
     *
     * Implements a physically-based camera model that combines optical components
     * (aperture, distortion, PSF), sensor characteristics (photosites, noise),
     * and geometric projection to simulate realistic image formation. Supports
     * spectral rendering, depth of field, optical aberrations, and various
     * sensor noise models for high-fidelity image synthesis.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Camera : public ReferenceFrame<TFloat> {
    public:
        Camera();

        // Delete copy operations
        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        // Add move operations
        Camera(Camera&& other) noexcept = default;
        Camera& operator=(Camera&& other) noexcept = default;

        // Initialization:
        void initialize();

        // Generic Processing Settings:
        void enableParallelInitialization(bool parallel_initialization = true);
        void enableDistortionInterpolation(bool interpolation = true);
        void enableDepthOfField(bool depth_of_field = true);
        void enableBlenderFrame(bool blender_frame = true);

        // Camera Intrinsic Parameters:
        void setFocalLength(double focal_length);
        TFloat getFocalLength() const { return focal_length_; } ///< Returns the camera focal length in meters
        void setPrincipalPoint(double px, double py);
        void setSkewParameters(double kxy, double kyx);

        void setResolution(vira::images::Resolution resolution);
        void setResolution(size_t x, size_t y);
        vira::images::Resolution getResolution() const { return resolution_; } ///< Returns the current image resolution

        void setSensorSize(vira::vec2<TFloat> sensor_size);
        void setSensorSize(double x, double y);
        void setSensorSize(double x);
        vira::vec2<TFloat> getSensorSize() const { return sensor_size_; } ///< Returns the physical sensor dimensions in meters

        void setOpticalEfficiency(double optical_efficiency);
        void setOpticalEfficiency(TSpectral optical_efficiency);

        // Camera Settings:
        void setExposureTime(double exposure_time);
        float getExposureTime() const { return exposure_time_; } ///< Returns the exposure time in seconds
        void setFocusDistance(double focus_distance);

        void setFStop(double f_stop);
        void setApertureDiameter(double aperture_diameter);

        void setGain(double gain);
        void setGainDB(double gain_db, double new_unity_gain_db = 0);

        // Photosite Configuration:
        void setCustomPhotosite(std::unique_ptr<Photosite<TSpectral>> custom_photosite);
        Photosite<TSpectral>& photosite();

        void setDefaultPhotositeBitDepth(size_t bit_depth);
        void setDefaultPhotositeWellDepth(size_t well_depth);

        void setDefaultPhotositeQuantumEfficiency(double quantum_efficiency);
        void setDefaultPhotositeQuantumEfficiency(TSpectral quantum_efficiency);
        void setDefaultPhotositeQuantumEfficiencyRGB(ColorRGB quantum_efficiency_rgb);

        template <IsUnit UnitType>
        void setDefaultPhotositeQuantumEfficiency(std::vector<UnitType> wavelengths, const std::vector<float>& quantum_efficiency);

        void setDefaultPhotositeLinearScaleFactor(double linear_scale_factor);

        // Aperture Configuration:
        bool hasAperture() const { return (aperture_ != nullptr) ? true : false; } ///< Returns whether an aperture has been defined
        void setCustomAperture(std::unique_ptr<Aperture<TSpectral, TFloat>> custom_aperture);
        Aperture<TSpectral, TFloat>& aperture();

        // PSF Configuration:
        bool hasPSF() const { return (psf_ != nullptr) ? true : false; } ///< Returns whether a PSF has been defined
        void setCustomPSF(std::unique_ptr<PointSpreadFunction<TSpectral, TFloat>> custom_psf);
        PointSpreadFunction<TSpectral, TFloat>& psf();

        void setDefaultAiryDiskPSF();
        void setDefaultGaussianPSF();

        void setGaussianPSF(TSpectral sigma_x, float angle = 0.f);
        void setGaussianPSF(TSpectral sigma_x, TSpectral sigma_y, float angle = 0.f);
        void setGaussianPSF(float sigma_x, float sigma_y, float angle = 0.f);

        // Distortion Configuration:
        bool hasDistortion() const { return (distortion_ != nullptr) ? true : false; } ///< Returns whether a Distortion has been defined
        void setCustomDistortion(std::unique_ptr<Distortion<TSpectral, TFloat>> custom_distortion);
        Distortion<TSpectral, TFloat>& distortion();

        void setBrownDistortionCoefficients(BrownCoefficients brown_coefficients);
        void setOwenDistortionCoefficients(OwenCoefficients owen_coefficients);
        void setOpenCVDistortionCoefficients(OpenCVCoefficients opencv_coefficients);

        // Noise Model Configuration:
        bool hasNoiseModel() const { return (noise_model_ != nullptr) ? true : false; } ///< Returns whether a Noise Model has been defined
        void setCustomNoiseModel(std::unique_ptr<NoiseModel> custom_noise_model);
        NoiseModel& noiseModel();

        void setDefaultLowNoise();
        void setDefaultFixedPatternNoise();

        void setDefaultDarkCurrent(double dark_current);
        void setDefaultReadoutNoiseMean(double readout_noise_mean);
        void setDefaultReadoutNoiseSTD(double readout_noise_std);

        void setDefaultHorizontalFixedPatternScale(double horizontal_fixed_pattern_scale);
        void setDefaultVerticalFixedPatternScale(double vertical_fixed_pattern_scale);

        void setDefaultHorizontalFixedPatternPeriod(size_t horizontal_fixed_pattern_period);
        void setDefaultVerticalFixedPatternPeriod(size_t vertical_fixed_pattern_period);

        // Filter Mosaic Configuration:
        bool hasFilterMosaic() const { return filter_mosaic_.resolution() == resolution_; } ///< Returns whether a Filter Mosaic has been defined
        void setCustomFilterMosaic(vira::images::Image<TSpectral> filter_mosaic);

        void setDefaultBayerFilter(TSpectral red_filter, TSpectral green_filter, TSpectral blue_filter);
        void setDefaultBayerFilter();

        // Projection and Ray Casting:
        Pixel projectCameraPoint(vira::vec3<TFloat> camera_point) const;
        Pixel projectWorldPoint(vira::vec3<TFloat> world_point) const;

        vira::vec3<TFloat> pixelToDirection(Pixel pixel) const;
        vira::rendering::Ray<TSpectral, TFloat> pixelToRay(Pixel pixel) const;
        vira::rendering::Ray<TSpectral, TFloat> pixelToRay(Pixel pixel, std::mt19937& rng, std::uniform_real_distribution<float>& dist) const;

        // Rendering Computations:
        TSpectral calculateReceivedPower(TSpectral radiance, int i, int j) const;
        TSpectral calculateReceivedPowerIrr(TSpectral irradiance) const;

        float computeMinimumDetectableIrradiance() const;

        vira::images::Image<TSpectral> getPhotonCounts(const vira::images::Image<TSpectral>& received_power) const;
        vira::images::Image<ColorRGB> getPhotonCountsRGB(const vira::images::Image<TSpectral>& received_power) const;

        vira::images::Image<float> simulateSensor(const vira::images::Image<TSpectral>& total_power_image) const;
        vira::images::Image<ColorRGB> simulateSensorRGB(const vira::images::Image<TSpectral>& total_power_image) const;

        // Geometry Computations:
        mat4<TFloat> getViewMatrix() const { return inverse(this->getGlobalTransformationMatrix()); } ///< Returns a mat4 View Matrix
        mat3<TFloat> getViewNormalMatrix() const { return this->getGlobalRotation().getInverseMatrix(); } ///< Returns whether a mat3 View Normal Matrix

        bool behind(const vira::vec3<TFloat>& point) const;
        TFloat calculateGSD(TFloat distance) const;
        vira::vec2<TFloat> getFOV() const;

        void lookAt(vira::vec3<TFloat> target, vira::vec3<TFloat> up = vira::vec3<TFloat>{ 0,0,1 });
        void lookInDirection(vira::vec3<TFloat> direction, vira::vec3<TFloat> up = vira::vec3<TFloat>{ 0,0,1 });

        bool obbInView(const vira::rendering::OBB<TFloat>& obb) const;

        const CameraID& getID() const { return id_; } ///< Returns the camera's unique CameraID

    private:
        CameraID id_;
        mutable std::mt19937 rng_{ std::random_device{}() };
        bool needs_initialization_ = true;

        // Generic Processing Settings:
        bool parallel_initialization_ = true;
        bool interpolate_distortion_directions_ = true;
        bool depth_of_field_ = false;
        bool blender_frame_ = false;

        // TODO DO SOMETHING WITH THESE:
        bool simulate_photon_noise_ = false;
        std::function<ColorRGB(const TSpectral&)> spectral_to_rgb_ = spectralToRGB_val<TSpectral>;
        std::function<TSpectral(const ColorRGB&)> rgb_to_spectral_ = rgbToSpectral_val<TSpectral>;

        // Camera Intrinsic Parameters:
        TFloat focal_length_ = static_cast<TFloat>(50. / 1000.);
        TFloat kx_ = std::numeric_limits<TFloat>::quiet_NaN();
        TFloat ky_ = std::numeric_limits<TFloat>::quiet_NaN();
        TFloat px_ = std::numeric_limits<TFloat>::infinity();
        TFloat py_ = std::numeric_limits<TFloat>::infinity();
        TFloat kxy_ = 0;
        TFloat kyx_ = 0;

        vira::images::Resolution resolution_{ 1920, 1080 };
        vec2<TFloat> sensor_size_{ 36. / 1000., 20.25 / 1000. };
        TSpectral optical_efficiency_{ 1 };

        // Camera Settings:
        float exposure_time_ = .01f;
        TFloat focus_distance_ = std::numeric_limits<TFloat>::infinity();
        float f_stop_ = 2.8f;
        float aperture_diameter_;

        // Photosite Configuration:
        PhotositeConfig<TSpectral> default_photosite_config_{};
        std::unique_ptr<Photosite<TSpectral>> photosite_ = nullptr;

        // Aperture Configuration:
        std::unique_ptr<Aperture<TSpectral, TFloat>> aperture_ = nullptr;

        // PSF Configuration:
        enum DefaultPSFOption {
            NO_PSF,
            AIRY_DISK_PSF,
            GAUSSIAN_PSF
        };
        DefaultPSFOption default_psf_option_ = DefaultPSFOption::NO_PSF;
        std::unique_ptr<PointSpreadFunction<TSpectral, TFloat>> psf_ = nullptr;

        // Distortion Configuration:
        std::unique_ptr<Distortion<TSpectral, TFloat>> distortion_ = nullptr;

        // Noise Model Configuration:
        bool noise_model_configured_ = false;
        NoiseModelConfig default_noise_model_config_{};
        std::unique_ptr<NoiseModel> noise_model_ = nullptr;

        // Mosaic Filter Configuration:
        bool use_bayer_filter_ = false;
        bool custom_filters_ = false;
        TSpectral default_bayer_red_;
        TSpectral default_bayer_green_;
        TSpectral default_bayer_blue_;
        vira::images::Image<TSpectral> filter_mosaic_{};

        // Pre-computed cached values:
        bool square_pixel_ = true;
        bool interpolate_directions_ = false;
        vira::images::Image<vec3<TFloat>> precomputed_pixel_directions_;
        vira::images::Image<float> pixel_solid_angle_;
        float z_dir_ = 1.f;
        vec2<float> pixel_size_;
        std::array<vec3<TFloat>, 8> frustum_corners_;
        std::array<vira::rendering::Plane<TFloat>, 4> frustum_planes_;

        // Intrinsic Parameters:
        mat23<TFloat> intrinsic_matrix_;
        mat23<TFloat> intrinsic_matrix_inv_;
        mat23<double> intrinsic_matrix_d_inv_;

        // Initialization Functions:
        void initializeIntrinsicMatrix();
        void initializePixelSolidAngle();
        void precomputePixelDirections();
        void precomputeFrustum();

        // Helper functions:
        vec3<TFloat> pixelToDirectionHelper(Pixel pixel) const;
        vec3<double> pixelToDirectionHelper_d(Pixel pixel) const;

        friend class vira::scene::Group<TSpectral, TFloat, TMeshFloat>;
    };
}

#include "implementation/cameras/camera.ipp"

#endif