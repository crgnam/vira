#include <cmath>

#include "vira/math.hpp"
#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/unresolved/magnitudes.hpp"

namespace vira::unresolved {

    static inline float lambertPhaseFunction(float phase)
    {
        return (std::sin(phase) + (PI<float>() - phase) * std::cos(phase)) / PI<float>();
    };

    static inline float asteroidPhi1(float alpha)
    {
        float A = 3.33f;
        float B = 0.64f;
        return std::exp(-A * std::pow(std::tan(alpha / 2), B));
    };

    static inline float asteroidPhi2(float alpha)
    {
        float A = 1.87f;
        float B = 1.22f;
        return std::exp(-A * std::pow(std::tan(alpha / 2), B));
    };


    template <IsSpectral TSpectral>
    TSpectral powerToIrradiance(TSpectral emissivePower, float distance)
    {
        return emissivePower / (distance * distance);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral lambertianSphereToIrradiance(float radius, const vec3<TFloat>& spherePosition, const vec3<TFloat>& observerPosition, vira::lights::Light<TSpectral, TFloat, TMeshFloat>& light, TSpectral albedo)
    {
        vec3<TFloat> relativePosition = spherePosition - observerPosition;
        TFloat distance = length(relativePosition);

        // Sample the light:
        TSpectral sourceIrr = light.getIrradiance(spherePosition);

        // Compute phase angle:
        vec3<TFloat> V = relativePosition / distance;
        vec3<TFloat> L = normalize(spherePosition - light.getGlobalTransformState().position);
        float phase = std::acos(dot(V, L));

        float A = PI<float>() * radius * radius; // Cross-sectional area
        TSpectral reflectedPower = albedo * A * sourceIrr * lambertPhaseFunction(phase);

        return reflectedPower / (distance * distance);
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    TSpectral asteroidHGToIrradiance(float H, float G, const vec3<TFloat>& asteroidPosition, const vec3<TFloat>& observerPosition, const vec3<TFloat>& sunPosition, TSpectral albedo)
    {
        vec3<TFloat> observerRelPos = observerPosition - asteroidPosition;
        vec3<TFloat> sunRelPos = sunPosition - asteroidPosition;

        float rE = static_cast<float>(length(observerRelPos));
        float rS = static_cast<float>(length(sunRelPos));

        float alpha = std::acos(dot(observerRelPos, sunRelPos)) / (rS * rE);

        rE = rE / AUtoKM<float>();
        rS = rS / AUtoKM<float>();

        float Ha = H - (2.5f * std::log10((1.f - G) * asteroidPhi1(alpha) + G * asteroidPhi2(alpha)));
        float V = Ha + (5.f * std::log10(rS * rE));

        return visualMagnitudeToIrradiance(V, albedo);
    };

    template <IsSpectral TSpectral>
    TSpectral visualMagnitudeToIrradiance(float V, TSpectral albedo)
    {
        double photonFlux = vira::unresolved::Vband.fluxFromMagnitude(V);
        TSpectral irradiance = photonFlux * TSpectral{ TSpectral::photonEnergies };
        //irradiance = TSpectral{ irradiance.magnitude() / TSpectral::size() };
        return irradiance * albedo / albedo.magnitude();
    };


    // ========================= //
    // === Unresolved Object === //
    // ========================= //
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    UnresolvedObject<TSpectral, TFloat, TMeshFloat>::UnresolvedObject(TSpectral newIrradiance) :
        irradiance{ newIrradiance }
    {

    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void UnresolvedObject<TSpectral, TFloat, TMeshFloat>::setIrradiance(TSpectral newIrradiance)
    {
        this->irradiance = newIrradiance;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void UnresolvedObject<TSpectral, TFloat, TMeshFloat>::setIrradianceFromPower(TSpectral emissivePower, float distance)
    {
        this->irradiance = powerToIrradiance<TSpectral>(emissivePower, distance);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void UnresolvedObject<TSpectral, TFloat, TMeshFloat>::setIrradianceFromLambertianSphere(float radius, const vec3<TFloat>& observerPosition, vira::lights::Light<TSpectral, TFloat, TMeshFloat>& light, TSpectral albedo)
    {
        this->irradiance = lambertianSphereToIrradiance<TSpectral, TFloat>(radius, this->getGlobalTransformState().position, observerPosition, light, albedo);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void UnresolvedObject<TSpectral, TFloat, TMeshFloat>::setIrradianceFromAsteroidHG(float H, float G, const vec3<TFloat>& observerPosition, const vec3<TFloat>& sunPosition, TSpectral albedo)
    {
        this->irradiance = asteroidHGToIrradiance<TSpectral, TFloat>(H, G, this->getGlobalTransformState().position, observerPosition, sunPosition, albedo);
    };
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void UnresolvedObject<TSpectral, TFloat, TMeshFloat>::setIrradianceFromVisualMagnitude(float V, TSpectral albedo)
    {
        this->irradiance = visualMagnitudeToIrradiance<TSpectral>(V, albedo);
    };
};