#ifndef VIRA_UNRESOLVED_STAR_LIGHT_HPP
#define VIRA_UNRESOLVED_STAR_LIGHT_HPP

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"

namespace vira::unresolved {
    template <IsSpectral TSpectral, IsFloat TFloat>
    class StarLight {
    public:
        StarLight(TSpectral irradiance, vec3<TFloat> direction)
            : irradiance_{ irradiance }, direction_{ normalize(direction) }
        {

        }

        TSpectral getIrradiance() const { return irradiance_; }
        vec3<TFloat> getICRFDirection() const { return direction_; }

    private:
        TSpectral irradiance_;
        vec3<TFloat> direction_;
    };
};

#endif