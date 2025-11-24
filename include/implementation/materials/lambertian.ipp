#include "vira/vec.hpp"
#include "vira/math.hpp"
#include "vira/spectral_data.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    TSpectral Lambertian<TSpectral>::evaluateBSDF(const UV& uv, const vec3<float>& N, const vec3<float>& L, const vec3<float>& V, TSpectral albedo)
    {
        (void)uv;
        (void)V;

        constexpr const float inv_pi = 1.f / PI<float>();

        return albedo * inv_pi * std::max(dot(N, L), 0.f);
    };
};