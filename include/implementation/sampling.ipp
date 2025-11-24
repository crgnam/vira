#include <random>
#include <algorithm>
#include <cmath>

#include "vira/math.hpp"
#include "vira/vec.hpp"

namespace vira {
    vec3<float> UniformHemisphereSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        float z = rand1;
        float r = std::sqrt(std::max(0.f, 1.f - z * z));
        float phi = 2 * PI<float>() * rand2;

        return vec3<float>{r* std::cos(phi), r* std::sin(phi), z};
    };
    float UniformHemispherePDF()
    {
        return 1.f / (2 * PI<float>());
    };


    vec3<float> UniformSphereSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        float z = 1 - 2 * rand1;
        float r = std::sqrt(std::max(0.f, 1.f - z * z));
        float phi = 2 * PI<float>() * rand2;

        return vec3<float>{r* std::cos(phi), r* std::sin(phi), z};
    };
    float UniformSpherePDF()
    {
        return INV_4_PI<float>();
    };


    vec2<float> UniformDiskSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        float r = std::sqrt(rand1);
        float theta = 2 * PI<float>() * rand2;

        return vec2<float>{ r* std::cos(theta), r* std::sin(theta) };
    };
    vec2<float> ConcentricDiskSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        vec2<float> uOffset = 2.f * vec2<float>{rand1, rand2} - vec2<float>{1, 1};

        constexpr float EPSILON = 1e-6f;  // Or std::numeric_limits<float>::epsilon()
        if (std::abs(uOffset.x) < EPSILON && std::abs(uOffset.y) < EPSILON) {
            return vec2<float>{0, 0};
        }

        float theta;
        float r;
        if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
            r = uOffset.x;
            theta = PI_OVER_4<float>() * (uOffset.y / uOffset.x);
        }
        else {
            r = uOffset.y;
            theta = PI_OVER_2<float>() - PI_OVER_4<float>() * (uOffset.x / uOffset.y);
        }

        return r * vec2<float>{ std::cos(theta), std::sin(theta) };
    };

    vec3<float> CosineHemisphereSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        vec2<float> d = ConcentricDiskSample(rng, dist);
        float z = std::sqrt(std::max(0.f, 1 - d.x * d.x - d.y * d.y));
        return vec3<float>{d.x, d.y, z};
    };
    float CosineHemispherePDF(float cosTheta)
    {
        return cosTheta * INV_PI<float>();
    };

    vec3<float> UniformConeSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist, float cosThetaMax)
    {
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        float cosTheta = (1.f - rand1) + rand2 * cosThetaMax;
        float sinTheta = std::sqrt(1.f - cosTheta * cosTheta);
        float phi = rand1 * 2 * PI<float>();

        return vec3<float>{std::cos(phi)* sinTheta, std::sin(phi)* sinTheta, cosTheta};
    };
    float UniformConePDF(float cosThetaMax)
    {
        return INV_2_PI<float>() * (1.f - cosThetaMax);
    };

    vec2<float> UniformTriangleSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        float su0 = std::sqrt(rand1);

        return vec2<float>{1 - su0, rand2* su0};
    };
    float UniformTrianglePDF(float area)
    {
        return 1.f / area;
    }
};