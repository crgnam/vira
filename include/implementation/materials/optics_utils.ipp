#include <cmath>

#include "vira/spectral_data.hpp"
#include "vira/vec.hpp"
#include "vira/constraints.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    TSpectral fresnelSchlick(float cosTheta, TSpectral F0)
    {
        return F0 + (TSpectral{ 1.f } - F0) * std::pow(1.f - cosTheta, 5.f);
    };

    template <IsFloat TFloat>
    vec3<TFloat> vira_reflect(const Normal& normal, const vec3<TFloat>& incident)
    {
        return incident - 2 * dot(incident, normal) * normal;
    };

    template <IsFloat TFloat>
    vec3<TFloat> vira_refract(const Normal& normal, const vec3<TFloat>& incident)
    {
        //TFloat iDotN = dot(incident, normal);
        //TFloat ratio = eta_i / eta_t;
        //return -(incident + normal * iDotN) * ratio - normal * std::sqrt(1 - (ratio * ratio) * (1 - (iDotN * iDotN)));

        (void)normal;
        (void)incident;
        return vec3<TFloat>{std::numeric_limits<TFloat>::infinity()};
    };
};