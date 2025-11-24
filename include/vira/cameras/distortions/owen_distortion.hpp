#ifndef VIRA_CAMERAS_DISTORTIONS_OWEN_DISTORTION_HPP
#define VIRA_CAMERAS_DISTORTIONS_OWEN_DISTORTION_HPP

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/distortions/distortion.hpp"

namespace vira::cameras {
    /**
     * @brief Owen distortion model coefficients
     *
     * Contains the six coefficients (e1-e6) used by the Owen distortion model,
     * which combines radial and tangential distortion components in a unique formulation.
     */
    struct OwenCoefficients : public DistortionCoefficients {
        float e1 = 0;  ///< First Owen distortion coefficient (radial odd terms)
        float e2 = 0;  ///< Second Owen distortion coefficient (radial even terms)
        float e3 = 0;  ///< Third Owen distortion coefficient (radial cubic terms)
        float e4 = 0;  ///< Fourth Owen distortion coefficient (radial quartic terms)
        float e5 = 0;  ///< Fifth Owen distortion coefficient (y-direction linear)
        float e6 = 0;  ///< Sixth Owen distortion coefficient (x-direction linear)

        OwenCoefficients() = default;

        /**
         * @brief Constructs Owen coefficients with specified values
         * @param e1_val First Owen distortion coefficient
         * @param e2_val Second Owen distortion coefficient
         * @param e3_val Third Owen distortion coefficient
         * @param e4_val Fourth Owen distortion coefficient
         * @param e5_val Fifth Owen distortion coefficient
         * @param e6_val Sixth Owen distortion coefficient
         */
        OwenCoefficients(float e1_val, float e2_val, float e3_val, float e4_val, float e5_val, float e6_val)
            : e1(e1_val), e2(e2_val), e3(e3_val), e4(e4_val), e5(e5_val), e6(e6_val) {
        }
    };

    /**
     * @brief Owen lens distortion model implementation
     * @tparam TSpectral Spectral data type
     * @tparam TFloat Floating point precision type
     *
     * Implements the Owen distortion model which uses a unique combination of
     * radial and tangential distortion terms with both coordinate-aligned and
     * rotated components for specialized lens correction applications.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class OwenDistortion : public Distortion<TSpectral, TFloat> {
    public:
        OwenDistortion() = default;
        OwenDistortion(OwenCoefficients coefficients);

        Pixel computeDelta(Pixel homogeneous_coords) const;

        Pixel distort(Pixel homogeneous_coords) const override;
        Pixel undistort(Pixel homogeneous_coords) const override;

        std::string getType() const override { return "Owen"; }
        DistortionCoefficients* getCoefficients() override { return &coefficients_; }

    private:
        OwenCoefficients coefficients_{};
    };
}

#include "implementation/cameras/distortions/owen_distortion.ipp"

#endif