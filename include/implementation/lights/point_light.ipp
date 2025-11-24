#include <memory>

#include "vira/vec.hpp"
#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/lights/light.hpp"

namespace vira::lights {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    PointLight<TSpectral, TFloat, TMeshFloat>::PointLight(TSpectral spectralPower) :
        spectralIntensity{ spectralPower / (4.f * PI<float>()) }
    {
        
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral PointLight<TSpectral, TFloat, TMeshFloat>::sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf)
    {
        // Use inverse-square law because this is a point light (no angular span)
        vec3<TFloat> pointToPointLight = this->globalTransformState.position - point;
        distance = static_cast<float>(length(pointToPointLight));
        TFloat dInv = TFloat{ 1 } / static_cast<TFloat>(distance);
        float dSq = distance * distance;

        pdf = 1.f;

        sampleRay = vira::rendering::Ray<TSpectral, TFloat>(point, dInv * pointToPointLight);
        return this->spectralIntensity / dSq;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral PointLight<TSpectral, TFloat, TMeshFloat>::sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        (void)rng;
        (void)dist;
        return this->sample(point, sampleRay, distance, pdf);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float PointLight<TSpectral, TFloat, TMeshFloat>::getPDF(const vec3<TFloat>& intersection, const vec3<TFloat>& direction)
    {
        // Point light: only one possible direction from any surface point
        vec3<TFloat> pointToLight = normalize(this->globalTransformState.position - intersection);
        vec3<TFloat> queryDirection = normalize(direction);

        // Check if the given direction matches the direction to the point light
        float dotProduct = dot(pointToLight, queryDirection);

        // Use a small epsilon for floating point comparison
        const float epsilon = 1e-6f;
        if (dotProduct > (1.0f - epsilon)) {
            return 1.0f;  // Delta function: 100% probability for the correct direction
        }

        return 0.0f;  // 0% probability for any other direction
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral PointLight<TSpectral, TFloat, TMeshFloat>::getIrradiance(const vec3<TFloat>& point)
    {
        vec3<TFloat> pointToPointLight = this->globalTransformState.position - point;
        float distance = static_cast<float>(length(pointToPointLight));
        float dSq = distance * distance;
        return this->spectralIntensity / dSq;
    };
};