#ifndef VIRA_UNRESOLVED_STAR_HPP
#define VIRA_UNRESOLVED_STAR_HPP

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/utils/valid_value.hpp"

namespace vira::unresolved {
    class Star {
    public:
        Star() = default;
        Star(double RAm, double DEm, double pmRA, double pmDE, double Vmag, double temp, double omega);
        Star(double RA, double DE, double Vmag, double temp, double omega);

        template <IsSpectral TSpectral>
        TSpectral getIrradiance() const;
        
        template <IsFloat TFloat>
        vec3<TFloat> getUnitVector(double et) const;
        
        inline bool isInitialized() { return initialized; }

        inline static bool compareByMag(const Star& a, const Star& b);

    protected:
        bool initialized = false;

        // Angle information:
        double RAm_ = vira::utils::INVALID_VALUE<double>(); // radians
        double DEm_ = vira::utils::INVALID_VALUE<double>(); // radians
        double pmRA_   = 0; // radians per year
        double pmDE_   = 0; // radians per year

        // Computed values for computing Irradiance:
        double Vmag_  = vira::utils::INVALID_VALUE<double>(); // Johnson Visual Magnitude
        double temp_  = vira::utils::INVALID_VALUE<double>(); // Temperature approximation
        double omega_ = vira::utils::INVALID_VALUE<double>(); // Angular size
    };
}
    
#include "implementation/unresolved/star.ipp"

#endif