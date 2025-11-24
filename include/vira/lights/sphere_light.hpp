#ifndef VIRA_LIGHTS_SPHERE_LIGHT_HPP
#define VIRA_LIGHTS_SPHERE_LIGHT_HPP

#include <random>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/lights/light.hpp"
#include "vira/rendering/ray.hpp"

namespace vira::lights {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class SphereLight : public Light<TSpectral, TFloat, TMeshFloat> {
    public:
        SphereLight(TSpectral spectralInput, TFloat radius, bool isPower);

        TSpectral sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf) override;
        TSpectral sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist) override;

        float getPDF(const vec3<TFloat>& intersection, const vec3<TFloat>& direction) override;

        TSpectral getIrradiance(const vec3<TFloat>& point) override;

        LightType getType() const override { return SPHERE_LIGHT; }

    private:
        TFloat radius;
        TSpectral spectralRadiance;
    };
};

#include "implementation/lights/sphere_light.ipp"

#endif