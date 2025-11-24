#ifndef VIRA_MATERIALS_OPTICS_UTILS_HPP 
#define VIRA_MATERIALS_OPTICS_UTILS_HPP

#include <cmath>

#include "vira/spectral_data.hpp"
#include "vira/vec.hpp"
#include "vira/constraints.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    TSpectral fresnelSchlick(float cosTheta, TSpectral F0);

    template <IsFloat TFloat>
    vec3<TFloat> vira_reflect(const Normal& normal, const vec3<TFloat>& incident);

    template <IsFloat TFloat>
    vec3<TFloat> vira_refract(const Normal& normal, const vec3<TFloat>& incident);
};

#include "implementation/materials/optics_utils.ipp"

#endif