#ifndef VIRA_MATERIALS_GGX_HPP
#define VIRA_MATERIALS_GGX_HPP

#include "vira/vec.hpp"

namespace vira::materials {
    inline float SchlickGGX(float NdotX, float k);

    inline float GeometrySmith(float NdotV, float NdotL, float k);

    inline float distributionGGX(float NdotH, float rough);

    // Implement:
    inline float GGX_D(Normal N, float alpha_x, float alpha_y);
    inline float GGX_G1(vec3<float> V, float alpha_x, float alpha_y);
    inline vec3<float> ggxVNDFSample(const vec3<float>& Ve, float alpha_x, float alpha_y, float U1, float U2);
    inline float ggxVNDFPDF(const vec3<float>& Ve, const vec3<float>& Ne, float alpha_x, float alpha_y);
};

#include "implementation/materials/ggx.ipp"

#endif