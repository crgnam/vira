#ifndef VIRA_MATERIALS_PBR_MATERIAL_HPP
#define VIRA_MATERIALS_PBR_MATERIAL_HPP

#include "vira/spectral_data.hpp"
#include "vira/materials/material.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    class PBRMaterial : public Material<TSpectral> {
    public:
        PBRMaterial() {
            this->setBSDFID(2); // or whatever ID you want
            // Default F0 for dielectrics (around 0.04)
            F0 = TSpectral{ 0.04f };
        }

        TSpectral evaluateBSDF(const UV& uv, const Normal& N,
            const vec3<float>& L, const vec3<float>& V, TSpectral vertAlbedo) override;

        void setF0(TSpectral f0) { F0 = f0; }
        TSpectral getF0() const { return F0; }

    private:
        TSpectral F0;

        float cookTorranceSpecular(float NdotL, float NdotV, float NdotH, float roughness);
        TSpectral evaluatePBR(const vec3<float>& N, const vec3<float>& L, const vec3<float>& V,
            TSpectral albedo, float metalness, float roughness);
    };
}

#include "implementation/materials/pbr_material.ipp"

#endif