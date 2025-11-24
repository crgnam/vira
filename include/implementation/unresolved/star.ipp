#include <cmath>
#include <vector>
#include <cstddef>

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::unresolved {
    template <IsSpectral TSpectral>
    TSpectral Star::getIrradiance() const
    {
        return omega_ * blackBodyRadiance<TSpectral>(temp_, 100);
    };

    template <IsFloat TFloat>
    vec3<TFloat> Star::getUnitVector(double et) const
    {
        vec3<TFloat> vec;

        // Compute time:
        double dt = et / SECONDS_PER_YEAR<double>();

        TFloat delta = static_cast<TFloat>(DEm_ + (pmRA_ * dt));
        TFloat alpha = static_cast<TFloat>(RAm_ + (pmDE_ * dt));

        vec.x = std::cos(delta) * std::cos(alpha);
        vec.y = std::cos(delta) * std::sin(alpha);
        vec.z = std::sin(delta);

        return vec;
    };

    Star::Star(double RAm, double DEm, double pmRA, double pmDE, double Vmag, double temp, double omega) :
        RAm_{ RAm }, DEm_{ DEm }, pmRA_{ pmRA }, pmDE_{ pmDE }, Vmag_{ Vmag }, temp_{ temp }, omega_{ omega }
    {
        initialized = true;
    };

    Star::Star(double RA, double DE, double Vmag, double temp, double omega) :
        RAm_{ RA }, DEm_{ DE }, Vmag_{ Vmag }, temp_{ temp }, omega_{ omega }
    {
        initialized = true;
    };

    bool Star::compareByMag(const Star& a, const Star& b)
    {
        return a.Vmag_ < b.Vmag_;
    };
};