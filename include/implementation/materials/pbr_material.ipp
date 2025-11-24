#include <cmath>
#include <algorithm>

#include "vira/math.hpp"
#include "vira/materials/ggx.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    TSpectral PBRMaterial<TSpectral>::evaluateBSDF(const UV& uv, const Normal& N,
        const vec3<float>& L, const vec3<float>& V, TSpectral vertAlbedo)
    {
        // Sample material properties
        TSpectral albedo = vertAlbedo * this->getAlbedo(uv);
        float metalness = this->getMetalness(uv);
        float roughness = this->getRoughness(uv);

        // Evaluate PBR BRDF
        TSpectral brdf = evaluatePBR(N, L, V, albedo, metalness, roughness);

        // Apply lighting equation: Lo = Li * BRDF * cos(theta)
        float NdotL = std::max(dot(N, L), 0.0f);
        return brdf * NdotL;
    }

    template <IsSpectral TSpectral>
    TSpectral PBRMaterial<TSpectral>::evaluatePBR(const vec3<float>& N, const vec3<float>& L, const vec3<float>& V,
        TSpectral albedo, float metalness, float roughness)
    {
        vec3<float> H = normalize(V + L);

        float NdotL = std::max(dot(N, L), 0.0f);
        float NdotV = std::max(dot(N, V), 0.0f);
        float NdotH = std::max(dot(N, H), 0.0f);
        float VdotH = std::max(dot(V, H), 0.0f);

        // Early exit for grazing angles
        if (NdotL <= 0.0f || NdotV <= 0.0f) {
            return TSpectral{ 0.0f };
        }

        // Calculate F0 based on metalness
        TSpectral f0 = F0 * (1.0f - metalness) + albedo * metalness;

        // Fresnel term using correct dot product (V * H)
        TSpectral F = fresnelSchlick(VdotH, f0);

        // Normal Distribution Function
        float D = distributionGGX(NdotH, roughness);

        // Geometry function
        float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f; // Direct lighting
        float G = GeometrySmith(NdotV, NdotL, k);

        // Cook-Torrance specular term
        float denominator = 4.0f * NdotV * NdotL;
        TSpectral specular = (D * G * F) / std::max(denominator, 0.001f);

        // Energy conservation
        TSpectral kS = F;
        TSpectral kD = (TSpectral{ 1.0f } - kS) * (1.0f - metalness);

        // Lambertian diffuse
        TSpectral diffuse = kD * albedo / PI<float>();

        return diffuse + specular;
    }
}