#ifndef VIRA_CAMERAS_DISTORTIONS_OPENCV_DISTORTION_HPP
#define VIRA_CAMERAS_DISTORTIONS_OPENCV_DISTORTION_HPP

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/distortions/distortion.hpp"

namespace vira::cameras {
    /**
     * @brief OpenCV camera distortion model coefficients
     *
     * Contains the complete set of distortion coefficients used by OpenCV's
     * camera calibration model, including radial, tangential, and thin prism distortions.
     */
    struct OpenCVCoefficients : public DistortionCoefficients {
        float k1 = 0.0;  ///< First radial distortion coefficient
        float k2 = 0.0;  ///< Second radial distortion coefficient
        float k3 = 0.0;  ///< Third radial distortion coefficient
        float k4 = 0.0;  ///< Fourth radial distortion coefficient
        float k5 = 0.0;  ///< Fifth radial distortion coefficient
        float k6 = 0.0;  ///< Sixth radial distortion coefficient

        float p1 = 0.0;  ///< First tangential distortion coefficient
        float p2 = 0.0;  ///< Second tangential distortion coefficient

        float s1 = 0.0;  ///< First thin prism distortion coefficient
        float s2 = 0.0;  ///< Second thin prism distortion coefficient
        float s3 = 0.0;  ///< Third thin prism distortion coefficient
        float s4 = 0.0;  ///< Fourth thin prism distortion coefficient

        OpenCVCoefficients() = default;

        /**
         * @brief Constructs OpenCV coefficients with specified values
         * @param k1_val First radial distortion coefficient
         * @param k2_val Second radial distortion coefficient
         * @param k3_val Third radial distortion coefficient
         * @param k4_val Fourth radial distortion coefficient
         * @param k5_val Fifth radial distortion coefficient
         * @param k6_val Sixth radial distortion coefficient
         * @param p1_val First tangential distortion coefficient
         * @param p2_val Second tangential distortion coefficient
         * @param s1_val First thin prism distortion coefficient
         * @param s2_val Second thin prism distortion coefficient
         * @param s3_val Third thin prism distortion coefficient
         * @param s4_val Fourth thin prism distortion coefficient
         */
        OpenCVCoefficients(float k1_val, float k2_val, float k3_val, float k4_val, float k5_val, float k6_val,
            float p1_val, float p2_val, float s1_val, float s2_val, float s3_val, float s4_val)
            : k1(k1_val), k2(k2_val), k3(k3_val), k4(k4_val), k5(k5_val), k6(k6_val),
            p1(p1_val), p2(p2_val), s1(s1_val), s2(s2_val), s3(s3_val), s4(s4_val) {
        }
    };

    /**
     * @brief OpenCV lens distortion model implementation
     * @tparam TSpectral Spectral data type
     * @tparam TFloat Floating point precision type
     *
     * Implements the comprehensive OpenCV camera distortion model that includes
     * rational radial distortion, tangential distortion, and thin prism distortion
     * components for high-accuracy camera calibration compatibility.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class OpenCVDistortion : public Distortion<TSpectral, TFloat> {
    public:
        OpenCVDistortion() = default;
        OpenCVDistortion(OpenCVCoefficients coefficients);

        Pixel computeDelta(Pixel homogeneous_coords) const;

        Pixel distort(Pixel homogeneous_coords) const override;
        Pixel undistort(Pixel homogeneous_coords) const override;

        std::string getType() const override { return "OpenCV"; }
        DistortionCoefficients* getCoefficients() override { return &coefficients_; }

    private:
        OpenCVCoefficients coefficients_{};
    };
}

#include "implementation/cameras/distortions/opencv_distortion.ipp"

#endif