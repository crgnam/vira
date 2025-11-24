#ifndef VIRA_CAMERAS_DISTORTIONS_BROWN_DISTORTION_HPP
#define VIRA_CAMERAS_DISTORTIONS_BROWN_DISTORTION_HPP

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/distortions/distortion.hpp"

namespace vira::cameras {
    /**
     * @brief Brown-Conrady distortion model coefficients
     * 
     * Contains radial distortion coefficients (k1, k2, k3) and 
     * tangential distortion coefficients (p1, p2) for the Brown-Conrady model.
     */
    struct BrownCoefficients : public DistortionCoefficients {
        float k1 = 0;  ///< First radial distortion coefficient
        float k2 = 0;  ///< Second radial distortion coefficient  
        float k3 = 0;  ///< Third radial distortion coefficient
        float p1 = 0;  ///< First tangential distortion coefficient
        float p2 = 0;  ///< Second tangential distortion coefficient

        BrownCoefficients() = default;
        
        /**
         * @brief Constructs Brown coefficients with specified values
         * @param k1_val First radial distortion coefficient
         * @param k2_val Second radial distortion coefficient
         * @param k3_val Third radial distortion coefficient
         * @param p1_val First tangential distortion coefficient
         * @param p2_val Second tangential distortion coefficient
         */
        BrownCoefficients(float k1_val, float k2_val, float k3_val, float p1_val, float p2_val)
            : k1(k1_val), k2(k2_val), k3(k3_val), p1(p1_val), p2(p2_val) {}
    };

    /**
     * @brief Brown-Conrady lens distortion model implementation
     * @tparam TSpectral Spectral data type
     * @tparam TFloat Floating point precision type
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class BrownDistortion : public Distortion<TSpectral, TFloat> {
    public:
        BrownDistortion() = default;
        BrownDistortion(BrownCoefficients coefficients);

        Pixel computeDelta(Pixel homogeneous_coords) const;

        Pixel distort(Pixel homogeneous_coords) const override;
        Pixel undistort(Pixel homogeneous_coords) const override;

        std::string getType() const override { return "Brown"; }
        DistortionCoefficients* getCoefficients() override { return &coefficients_; }

    private:
        BrownCoefficients coefficients_{};
    };
}

#include "implementation/cameras/distortions/brown_distortion.ipp"

#endif