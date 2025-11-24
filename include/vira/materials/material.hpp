#ifndef VIRA_MATERIALS_MATERIAL_HPP
#define VIRA_MATERIALS_MATERIAL_HPP

#include <random>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image.hpp"
#include "vira/scene/ids.hpp"
#include "vira/materials/material_sampling.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    class Material {
    public:
        Material() = default;
        virtual ~Material() = default;

        void setSamplingStrategy(SamplingStrategy sampling_strategy) { sampling_strategy_ = sampling_strategy; }

        virtual TSpectral evaluateBSDF(const UV& uv, const Normal& N, const vec3<float>& L, const vec3<float>& V, TSpectral albedo) = 0;

        vec3<float> sampleDirection(const vec3<float>& V, const vec3<float>& N, const mat3<float>& tangentToWorld, 
            const vec2<float>& uv, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist);

        float getPDF(const vec3<float>& V, const vec3<float>& N, const vec3<float>& L, const mat3<float>& tangentToWorld, const vec2<float>& uv);

        TSpectral applyAmbient(TSpectral ambient, const TSpectral& albedo, const vec2<float>& uv);

        void setAlbedo(vira::images::Image<TSpectral> albedoMap);
        void setAlbedo(TSpectral albedo);
        TSpectral getAlbedo(const UV& uv);

        vira::images::Image<TSpectral> getAlbedoMap() { return albedoMap; }

        void setNormalMap(vira::images::Image<Normal> normalMap);
        vec3<float> getNormal(const vec2<float>& UV, const vec3<float>& N, const mat3<float>& tangentToWorld);
        vira::images::Image<Normal> getNormalMap() { return normalMap; }
        
        void setRoughness(vira::images::Image<float> roughnessMap);
        void setRoughness(float roughness);
        float getRoughness(const vec2<float>& UV);
        vira::images::Image<float> getRoughnessMap() { return roughnessMap; }

        void setMetalness(vira::images::Image<float> metalnessMap);
        void setMetalness(float metalness);
        float getMetalness(const vec2<float>& UV);
        vira::images::Image<float> getMetalnessMap() { return metalnessMap; }


        void setTransmission(vira::images::Image<TSpectral> transmissionMap);
        void setTransmission(TSpectral transmission);
        TSpectral getTransmission(const UV& uv);
        vira::images::Image<TSpectral> getTransmissionMap() { return transmissionMap; }

        void setEmission(vira::images::Image<TSpectral> emissionMap);
        void setEmission(TSpectral emission);
        TSpectral getEmission(const vec2<float>& UV);
        vira::images::Image<TSpectral> getEmissionMap() { return emissionMap; }

        // Getter for BSDF name
        int getBSDFID() const { return bsdfID_; }
        
        // Setter for BSDF name (optional, if you need to change it later)
        void setBSDFID(int bsdfID) { bsdfID_ = bsdfID; }

        const MaterialID& getID() { return id_; }

    protected:

        vira::images::Image<TSpectral> albedoMap = vira::images::Image<TSpectral>(vira::images::Resolution{ 1,1 }, TSpectral{ 1.f });
        vira::images::Image<vec3<float>> normalMap = vira::images::Image<Normal>(vira::images::Resolution{ 0,0 }, vec3<float>({0.f, 0.f, 0.f}));
        vira::images::Image<float> roughnessMap = vira::images::Image<float>(vira::images::Resolution{ 1,1 }, 0.5f);
        vira::images::Image<float> metalnessMap = vira::images::Image<float>(vira::images::Resolution{ 1,1 }, 0.f);
        vira::images::Image<TSpectral> transmissionMap = vira::images::Image<TSpectral>(vira::images::Resolution{ 1,1 }, TSpectral{ 0.f });
        vira::images::Image<TSpectral> emissionMap = vira::images::Image<TSpectral>(vira::images::Resolution{ 1,1 }, TSpectral{ 0.f });

        int bsdfID_; // BSDF name property

    private:
        MaterialID id_{};

        SamplingStrategy sampling_strategy_ = SamplingStrategy::COSINE_WEIGHTED;

        template <IsSpectral TSpectral2, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
        friend class vira::Scene;
    };
};

#include "implementation/materials/material.ipp"

#endif
