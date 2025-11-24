#ifndef VIRA_MATERIALS_LAMBERTIAN_HPP
#define VIRA_MATERIALS_LAMBERTIAN_HPP

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/materials/material.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    class Lambertian : public Material<TSpectral> {
    public:
        Lambertian() = default;

        TSpectral evaluateBSDF(const UV& uv, const Normal& N, const vec3<float>& L, const vec3<float>& V, TSpectral albedo) override;
    };
};

#include "implementation/materials/lambertian.ipp"

#endif