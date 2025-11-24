#include <cmath>

#include "vira/math.hpp"
#include "vira/vec.hpp"

namespace vira::materials {
    float SchlickGGX(float NdotX, float k)
    {
        float denom = NdotX * (1.f - k) + k;
        return NdotX / denom;
    };

    float GeometrySmith(float NdotV, float NdotL, float k)
    {
        float ggx1 = SchlickGGX(NdotV, k);
        float ggx2 = SchlickGGX(NdotL, k);
        return ggx1 * ggx2;
    };

    float distributionGGX(float NdotH, float rough)
    {
        float alphaSq = (rough * rough);
        float a = (NdotH * NdotH) * (alphaSq - 1.f) + 1.f;
        return alphaSq / (PI<float>() * a * a);
    }


    // New Implementation:
    float GGX_D(Normal N, float alpha_x, float alpha_y)
    {
        float t1 = PI<float>() * alpha_x * alpha_y;

        float a = (N.x / alpha_x) * (N.x / alpha_x);
        float b = (N.y / alpha_y) * (N.y / alpha_y);
        float c = N.z * N.z;
        float t2 = a + b + c;

        return 1.f / (t1 * (t2 * t2));
    };

    float GGX_G1(vec3<float> V, float alpha_x, float alpha_y)
    {
        float t = ((alpha_x * alpha_x) * (V.x * V.x) + (alpha_y * alpha_y) * (V.y * V.y)) / (V.z * V.z);
        float L = (-1.f + std::sqrt(1.f + t)) / 2.f;
        return (1.f / (1.f + L));
    };

    vec3<float> ggxVNDFSample(const vec3<float>& Ve, float alpha_x, float alpha_y, float U1, float U2)

    {
        // Section 3.2: transforming the view direction to the hemisphere configuration
        vec3<float> Vh = normalize(vec3<float>{alpha_x* Ve.x, alpha_y* Ve.y, Ve.z});

        // Section 4.1: orthonormal basis (with special case if cross product is zero)
        float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
        vec3<float> T1;
        if (lensq > 0) {
            T1 = vec3<float>{ -Vh.y, Vh.x, 0 } *(1.f / sqrt(lensq));
        }
        else {
            T1 = vec3<float>{ 1, 0, 0 };
        }

        vec3<float> T2 = cross(Vh, T1);

        // Section 4.2: parameterization of the projected area
        float r = std::sqrt(U1);
        float phi = 2.f * PI<float>() * U2;
        float t1 = r * cos(phi);
        float t2 = r * sin(phi);
        float s = 0.5f * (1.f + Vh.z);
        t2 = (1.f - s) * sqrt(1.f - t1 * t1) + s * t2;

        // Section 4.3: reprojection onto hemisphere
        vec3<float> Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.f, 1.f - t1 * t1 - t2 * t2)) * Vh;

        // Section 3.4: transforming the normal back to the ellipsoid configuration
        vec3<float> Ne = normalize(vec3<float>(alpha_x * Nh.x, alpha_y * Nh.y, std::max<float>(0.0, Nh.z)));
        return Ne;
    };

    float ggxVNDFPDF(const vec3<float>& Ve, const vec3<float>& Ne, float alpha_x, float alpha_y)
    {
        return GGX_G1(Ve, alpha_x, alpha_y) * std::max(0.f, dot(Ve, Ne)) * GGX_D(Ne, alpha_x, alpha_y) / Ve.z;
    };
};