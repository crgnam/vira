#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/images/interfaces/image_interface.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    vec3<float> Material<TSpectral>::sampleDirection(const vec3<float>& V, const vec3<float>& N, const mat3<float>& tangentToWorld,
        const vec2<float>& uv, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        (void)V;

        float roughness;

        switch (sampling_strategy_) {
        case SamplingStrategy::COSINE_WEIGHTED:
            return cosineWeightedSample(tangentToWorld, pdf, rng, dist);

        case SamplingStrategy::UNIFORM_HEMISPHERE:
            return uniformSample(tangentToWorld, pdf, rng, dist);

        case SamplingStrategy::GGX_IMPORTANCE:
            roughness = this->getRoughness(uv);
            return ggxImportanceSample(N, tangentToWorld, pdf, roughness, rng, dist);

        case SamplingStrategy::GGX_BRDF_IMPORTANCE:
            roughness = this->getRoughness(uv);
            return ggxBrdfSample(V, N, tangentToWorld, pdf, roughness, rng, dist);

        default:
            return cosineWeightedSample(tangentToWorld, pdf, rng, dist);
        }
    };

    template <IsSpectral TSpectral>
    float Material<TSpectral>::getPDF(const vec3<float>& V, const vec3<float>& N, const vec3<float>& L, const mat3<float>& tangentToWorld, const vec2<float>& uv)
    {
        (void)V; // TODO IMPLEMENT GGX
        (void)tangentToWorld;

        float roughness;

        switch (sampling_strategy_) {
        case SamplingStrategy::COSINE_WEIGHTED:
            return cosineWeightedPDF(N, L);

        case SamplingStrategy::UNIFORM_HEMISPHERE:
            return uniformPDF(N, L);

        case SamplingStrategy::GGX_IMPORTANCE:
            // For microfacet sampling, this gets complex - you'd need the microfacet normal
            // Usually you'd store this or recalculate it
            roughness = this->getRoughness(uv);
            return ggxPDF(N, normalize(V + L), roughness) / (4.0f * dot(V, normalize(V + L)));

        case SamplingStrategy::GGX_BRDF_IMPORTANCE:
            roughness = this->getRoughness(uv);
            return ggxBrdfPDF(V, N, L, roughness);

        default:
            return cosineWeightedPDF(N, L);
        }
    };

    template <IsSpectral TSpectral>
    TSpectral Material<TSpectral>::applyAmbient(TSpectral ambient, const TSpectral& albedo, const vec2<float>& uv)
    {
        float roughness;
        float ambientScale;

        switch (sampling_strategy_) {
        case SamplingStrategy::COSINE_WEIGHTED:
            return ambient * albedo;

        case SamplingStrategy::UNIFORM_HEMISPHERE:
            return ambient * albedo;

        case SamplingStrategy::GGX_IMPORTANCE:
            // Shiny surfaces (low roughness) get less diffuse ambient
            // Rough surfaces (high roughness) get more diffuse ambient
            roughness = this->getRoughness(uv);
            ambientScale = 0.2f + 0.8f * roughness;

            return ambient * albedo * ambientScale;

        case SamplingStrategy::GGX_BRDF_IMPORTANCE:
            // Shiny surfaces (low roughness) get less diffuse ambient
            // Rough surfaces (high roughness) get more diffuse ambient
            roughness = this->getRoughness(uv);
            ambientScale = 0.2f + 0.8f * roughness;

            return ambient * albedo * ambientScale;

        default:
            return ambient * albedo;
        }
    };


    template <IsSpectral TSpectral>
    void Material<TSpectral>::setAlbedo(vira::images::Image<TSpectral> newAlbedoMap)
    {
        this->albedoMap = newAlbedoMap;
    };

    template <IsSpectral TSpectral>
    void Material<TSpectral>::setAlbedo(TSpectral albedo)
    {
        this->albedoMap = vira::images::Image<TSpectral>(vira::images::Resolution{ 1,1 }, albedo);
    };

    template <IsSpectral TSpectral>
    TSpectral Material<TSpectral>::getAlbedo(const UV& uv)
    {
        return this->albedoMap.sampleUVs(uv);
    };




    template <IsSpectral TSpectral>
    void Material<TSpectral>::setNormalMap(vira::images::Image<Normal> newNormalMap)
    {
        this->normalMap = newNormalMap;
    };

    template <IsSpectral TSpectral>
    Normal Material<TSpectral>::getNormal(const UV& uv, const Normal& N, const mat3<float>& tangentToWorld)
    {
        if (this->normalMap.size() == 0) {
            return N;
        }

        // Sample the normal map at the given UV coordinates
        vec3<float> tangentSpaceNormal = this->normalMap.sampleUVs(uv);

        // Transform the tangent space normal to world space
        vec3<float> worldSpaceNormal = tangentToWorld * tangentSpaceNormal;

        // Normalize and return
        return normalize(worldSpaceNormal);
    };




    template <IsSpectral TSpectral>
    void Material<TSpectral>::setRoughness(vira::images::Image<float> newRoughnessMap)
    {
        this->roughnessMap = newRoughnessMap;
    };

    template <IsSpectral TSpectral>
    void Material<TSpectral>::setRoughness(float roughness)
    {
        this->roughnessMap = vira::images::Image<float>(vira::images::Resolution{ 1,1 }, roughness);
    };

    template <IsSpectral TSpectral>
    float Material<TSpectral>::getRoughness(const UV& uv)
    {
        return this->roughnessMap.sampleUVs(uv);
    };



    template <IsSpectral TSpectral>
    void Material<TSpectral>::setMetalness(vira::images::Image<float> newMetalnessMap)
    {
        this->metalnessMap = newMetalnessMap;
    };

    template <IsSpectral TSpectral>
    void Material<TSpectral>::setMetalness(float metalness)
    {
        this->metalnessMap = vira::images::Image<float>(vira::images::Resolution{ 1,1 }, metalness);
    };

    template <IsSpectral TSpectral>
    float Material<TSpectral>::getMetalness(const UV& uv)
    {
        return this->metalnessMap.sampleUVs(uv);
    };



    template <IsSpectral TSpectral>
    void Material<TSpectral>::setTransmission(vira::images::Image<TSpectral> newTransmissionMap)
    {
        this->transmissionMap = newTransmissionMap;
    };

    template <IsSpectral TSpectral>
    void Material<TSpectral>::setTransmission(TSpectral transmission)
    {
        this->transmissionMap = vira::images::Image<TSpectral>(vira::images::Resolution{ 1,1 }, transmission);
    };

    template <IsSpectral TSpectral>
    TSpectral Material<TSpectral>::getTransmission(const UV& uv)
    {
        return this->transmissionMap.sampleUVs(uv);
    };



    template <IsSpectral TSpectral>
    void Material<TSpectral>::setEmission(vira::images::Image<TSpectral> newEmissionMap)
    {
        this->emissionMap = newEmissionMap;
    };

    template <IsSpectral TSpectral>
    void Material<TSpectral>::setEmission(TSpectral emission)
    {
        this->emissionMap = vira::images::Image<TSpectral>(vira::images::Resolution{ 1,1 }, emission);
    };

    template <IsSpectral TSpectral>
    TSpectral Material<TSpectral>::getEmission(const UV& uv)
    {
        return this->emissionMap.sampleUVs(uv);
    };

};
