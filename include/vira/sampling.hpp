#ifndef VIRA_SAMPLING_HPP
#define VIRA_SAMPLING_HPP

#include <random>

#include "vira/vec.hpp"

namespace vira {
    inline vec3<float> UniformHemisphereSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist);
    inline float UniformHemispherePDF();

    inline vec3<float> UniformSphereSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist);
    inline float UniformSpherePDF();

    inline vec2<float> UniformDiskSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist);
    inline vec2<float> ConcentricDiskSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist);

    inline vec3<float> CosineHemisphereSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist);
    inline float CosineHemispherePDF(float cosTheta);

    inline vec3<float> UniformConeSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist, float cosThetaMax);
    inline float UniformConePDF(float cosThetaMax);

    inline vec2<float> UniformTriangleSample(std::mt19937& rng, std::uniform_real_distribution<float>& dist);
    inline float UniformTrianglePDF(float area);
};

#include "implementation/sampling.ipp"

#endif