#ifndef VIRA_MATERIALS_MATERIAL_SAMPLING_HPP
#define VIRA_MATERIALS_MATERIAL_SAMPLING_HPP

#include "vira/vec.hpp"
#include "vira/sampling.hpp"

namespace vira::materials {
    enum class SamplingStrategy {
        COSINE_WEIGHTED,
        UNIFORM_HEMISPHERE,
        GGX_IMPORTANCE,
        GGX_BRDF_IMPORTANCE
    };

    static inline vira::vec3<float> cosineWeightedSample(const vira::mat3<float>& tangentToWorld,
        float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist) 
    {
        vira::vec3<float> localSample = vira::CosineHemisphereSample(rng, dist);
        pdf = vira::CosineHemispherePDF(localSample.z);
        return tangentToWorld * localSample;
    }

    static inline float cosineWeightedPDF(const vira::vec3<float>& N, const vira::vec3<float>& L)
    {
        float cosTheta = dot(N, L);
        if (cosTheta <= 0.f) {
            return 0.f;
        }
        return vira::CosineHemispherePDF(cosTheta);
    };



    static inline vira::vec3<float> uniformSample(const vira::mat3<float>& tangentToWorld,
        float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist) {

        vira::vec3<float> localSample = vira::UniformHemisphereSample(rng, dist);
        pdf = 1.0f / (2.0f * vira::PI<float>());
        return tangentToWorld * localSample;
    }

    static inline float uniformPDF(const vira::vec3<float>& N, const vira::vec3<float>& L) {
        float cosTheta = dot(N, L);
        if (cosTheta <= 0.f) {
            return 0.f;
        }
        return 1.0f / (2.0f * vira::PI<float>());
    }




    // GGX/Trowbridge-Reitz Distribution Function
    static inline float ggxDistribution(float cosTheta, float roughness) {
        if (cosTheta <= 0.0f) return 0.0f;

        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;
        float cosTheta2 = cosTheta * cosTheta;
        float tanTheta2 = (1.0f - cosTheta2) / cosTheta2;

        float denom = cosTheta2 * cosTheta2 * (alpha2 + tanTheta2) * (alpha2 + tanTheta2);
        return alpha2 / (PI<float>() * denom);
    }

    // Sample a microfacet normal using GGX distribution (importance sampling)
    static inline vira::vec3<float> ggxImportanceSample(const vira::vec3<float>& N, const vira::mat3<float>& tangentToWorld,
        float& pdf, float roughness, std::mt19937& rng, std::uniform_real_distribution<float>& dist) {

        (void)N;

        float u1 = dist(rng);
        float u2 = dist(rng);

        // Sample microfacet normal in local coordinates
        float alpha = roughness * roughness;
        float alpha2 = alpha * alpha;

        // GGX importance sampling formulas
        float cosTheta = sqrt((1.0f - u1) / (1.0f + (alpha2 - 1.0f) * u1));
        float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
        float phi = 2.0f * PI<float>() * u2;

        // Convert to Cartesian coordinates in local space (microfacet normal)
        vira::vec3<float> localMicrofacet = vira::vec3<float>(
            sinTheta * cos(phi),
            sinTheta * sin(phi),
            cosTheta
        );

        // Transform microfacet normal to world space
        vira::vec3<float> worldMicrofacet = tangentToWorld * localMicrofacet;

        // Calculate PDF for this microfacet normal
        float D = ggxDistribution(cosTheta, roughness);
        pdf = D * cosTheta;  // PDF = D(h) * cos(theta_h)

        return worldMicrofacet;
    }

    // Calculate PDF for GGX sampling given a microfacet normal
    static inline float ggxPDF(const vira::vec3<float>& N, const vira::vec3<float>& microfacetNormal, float roughness) {
        float cosTheta = dot(N, microfacetNormal);
        if (cosTheta <= 0.0f) return 0.0f;

        float D = ggxDistribution(cosTheta, roughness);
        return D * cosTheta;  // PDF = D(h) * cos(theta_h)
    }

    // === COMPLETE GGX BRDF SAMPLING (More Realistic) ===
    // This samples the actual reflection direction, not just the microfacet normal

    static inline vira::vec3<float> ggxBrdfSample(const vira::vec3<float>& V, const vira::vec3<float>& N,
        const vira::mat3<float>& tangentToWorld, float& pdf, float roughness,
        std::mt19937& rng, std::uniform_real_distribution<float>& dist) {

        // First sample a microfacet normal using GGX importance sampling
        float microfacetPdf;
        vira::vec3<float> microfacetNormal = ggxImportanceSample(N, tangentToWorld, microfacetPdf, roughness, rng, dist);

        // Reflect the view direction around the microfacet normal to get light direction
        vira::vec3<float> L = reflect(-V, microfacetNormal);

        // Check if the reflection is in the correct hemisphere
        if (dot(N, L) <= 0.0f) {
            pdf = 0.0f;
            return vira::vec3<float>(0, 0, 0);
        }

        // Transform microfacet PDF to reflection direction PDF
        // PDF_L = PDF_h / (4 * |V * H|) where H is the microfacet normal
        float VdotH = dot(V, microfacetNormal);
        if (VdotH <= 0.0f) {
            pdf = 0.0f;
            return vira::vec3<float>(0, 0, 0);
        }

        pdf = microfacetPdf / (4.0f * VdotH);

        return L;
    }

    // Calculate PDF for GGX BRDF sampling
    static inline float ggxBrdfPDF(const vira::vec3<float>& V, const vira::vec3<float>& N,
        const vira::vec3<float>& L, float roughness) {

        if (dot(N, L) <= 0.0f) return 0.0f;

        // Calculate the microfacet normal (halfway vector)
        vira::vec3<float> H = normalize(V + L);

        // Get the microfacet PDF
        float microfacetPdf = ggxPDF(N, H, roughness);

        // Transform to reflection direction PDF
        float VdotH = dot(V, H);
        if (VdotH <= 0.0f) return 0.0f;

        return microfacetPdf / (4.0f * VdotH);
    }

    // === HELPER FUNCTIONS ===

    // Reflect vector V around normal N
    static inline vira::vec3<float> reflect(const vira::vec3<float>& V, const vira::vec3<float>& N) {
        return V - 2.0f * dot(V, N) * N;
    }

    // Clamp function
    static inline float clamp(float value, float minVal, float maxVal) {
        return std::max(minVal, std::min(maxVal, value));
    }
};

#endif