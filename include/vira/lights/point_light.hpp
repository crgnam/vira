#ifndef VIRA_LIGHTS_POINT_LIGHT_HPP
#define VIRA_LIGHTS_POINT_LIGHT_HPP

#include <random>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/lights/light.hpp"
#include "vira/rendering/ray.hpp"

namespace vira::lights {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class PointLight : public Light<TSpectral, TFloat, TMeshFloat> {
    public:
        PointLight(TSpectral spectralPower);

        TSpectral sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf) override;
        TSpectral sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist) override;

        float getPDF(const vec3<TFloat>& intersection, const vec3<TFloat>& direction) override;

        TSpectral getIrradiance(const vec3<TFloat>& point) override;

        LightType getType() const override { return POINT_LIGHT; }

    private:
        TSpectral spectralIntensity{ 0 };
    };
};

#include "implementation/lights/point_light.ipp"

#endif