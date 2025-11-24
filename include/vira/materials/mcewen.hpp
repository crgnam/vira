#ifndef VIRA_MATERIALS_MCEWEN_HPP
#define VIRA_MATERIALS_MCEWEN_HPP

#include <cmath>

#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/materials/material.hpp"

namespace vira::materials {
    template <IsSpectral TSpectral>
    class McEwen : public Material<TSpectral> {
    public:
        McEwen() = default;

        TSpectral evaluateBSDF(const UV& uv, const Normal& N, const vec3<float>& L, const vec3<float>& V, TSpectral albedo) override;
    };
};

#include "implementation/materials/mcewen.ipp"

#endif