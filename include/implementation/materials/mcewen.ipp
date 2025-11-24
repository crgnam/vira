#include "vira/materials/mcewen.hpp"

#include "vira/math.hpp"
#include "vira/spectral_data.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    TSpectral McEwen<TSpectral>::evaluateBSDF(const UV& uv, const Normal& N, const vec3<float>& L, const vec3<float>& V, TSpectral vertAlbedo)
    {
        // Determine if surface is illuminated:
        Normal N_use = this->getNormal(uv, N);
        float cosI = std::max(dot(N_use, L), 0.f);
        if (cosI == 0) {
            return TSpectral{ 0.f };
        }

        // Pre-compute albedo and emission cosine:
        TSpectral albedo = vertAlbedo * this->getAlbedo(uv);
        float cosE = std::max(dot(N_use, V), 0.f);

        // Lambertian component:
        constexpr const float inv_pi = 1.f / PI<float>();
        float lambert = inv_pi * cosI;

        // Lommel-Seeliger (Lunar Lambert) component:
        // TODO Validate energy conservation here
        constexpr const float inv_4_pi = 1.f / (4.f * PI<float>());
        float lommel_seeliger = inv_4_pi * (cosI / (cosI + cosE));

        // Compute mixing fraction:
        const float alpha0 = 60; // Degrees
        float alpha = RAD2DEG<float>() * std::acos(dot(L, V)); // Phase angle in degrees
        float beta = std::exp(-alpha / alpha0);

        // Evaluate reflectance:
        return albedo * (((1 - beta) * lambert) + (beta * lommel_seeliger));
    };
};