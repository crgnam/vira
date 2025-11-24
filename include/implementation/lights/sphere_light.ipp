#include <memory>
#include <random>
#include <cmath>
#include <stdexcept>

#include "vira/vec.hpp"
#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/lights/light.hpp"

#include "vira/utils/valid_value.hpp"

namespace vira::lights {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    SphereLight<TSpectral, TFloat, TMeshFloat>::SphereLight(TSpectral spectralInput, TFloat newRadius, bool isPower)
        : radius{ newRadius }
    {
        vira::utils::validatePositiveDefinite(radius, "SphereLight Radius");

        if (isPower) {
            spectralRadiance = spectralInput / ((4 * PI<float>() * static_cast<float>(radius * radius)) * PI<float>());
        }
        else {
            spectralRadiance = spectralInput;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral SphereLight<TSpectral, TFloat, TMeshFloat>::sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf)
    {
        // Construct sampling frame:
        vec3<TFloat> pointToLight = this->getGlobalPosition() - point;
        distance = static_cast<float>(length(pointToLight));
        TFloat dInv = TFloat{ 1 } / static_cast<TFloat>(distance);

        sampleRay = vira::rendering::Ray<TSpectral, TFloat>(point, dInv * pointToLight);

        // Handle the inside-sphere case gracefully
        if (distance <= this->radius) {
            // Point is inside or on the sphere - sample to the closest surface point
            if (distance < 1e-3f) {
                // Point is at light center - pick arbitrary direction
                vec3<TFloat> arbitraryDir = vec3<TFloat>{ 1, 0, 0 }; // or use a random direction
                sampleRay = vira::rendering::Ray<TSpectral, TFloat>(point, arbitraryDir);
                distance = this->radius;
                pdf = 1.0f / (4.0f * PI<float>() * this->radius * this->radius); // Full sphere
            }
            else {
                // Point is inside - ray to surface
                sampleRay = vira::rendering::Ray<TSpectral, TFloat>(point, dInv * pointToLight);
                distance = this->radius; // Distance to surface
                pdf = 1.0f / (4.0f * PI<float>() * this->radius * this->radius); // Full sphere visible
            }
            return this->spectralRadiance;
        }

        float r2 = static_cast<float>(this->radius * this->radius);
        float Omega = 2 * PI<float>() * (1.f - std::sqrt((distance * distance) - r2) / distance);
        pdf = 1.f / Omega;

        return this->spectralRadiance;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral SphereLight<TSpectral, TFloat, TMeshFloat>::sample(const vec3<TFloat>& point, vira::rendering::Ray<TSpectral, TFloat>& sampleRay, float& distance, float& pdf, std::mt19937& rng, std::uniform_real_distribution<float>& dist)
    {
        // Generate random samples:
        float rand1 = dist(rng);
        float rand2 = dist(rng);

        // Construct sampling frame:
        vec3<TFloat> N;
        vec3<TFloat> pointToLight = this->getGlobalPosition() - point;
        distance = static_cast<float>(length(pointToLight));
        TFloat dInv = TFloat{ 1 } / static_cast<TFloat>(distance);
        vec3<TFloat> W = dInv * pointToLight;
        const vec3<TFloat> N1{1, 0, 0};
        const vec3<TFloat> N2{0, 1, 0};
        if (dot(N1, W) < dot(N2, W)) {
            N = N1;
        }
        else {
            N = N2;
        }
        vec3<TFloat> V = normalize(cross(W, N));
        vec3<TFloat> U = normalize(cross(W, V));
        mat3<TFloat> frame;
        frame[0][0] = U[0];
        frame[1][0] = U[1];
        frame[2][0] = U[2];
        frame[0][1] = W[0];
        frame[1][1] = W[1];
        frame[2][1] = W[2];
        frame[0][2] = V[0];
        frame[1][2] = V[1];
        frame[2][2] = V[2];

        float phi = 2 * PI<float>() * rand1;
        float zeta = rand2;

        float c = static_cast<float>(this->radius) / distance;
        float theta = std::acos(1 - zeta + zeta * std::sqrt(1 - (c * c)));

        float sinT = std::sin(theta);
        float sinP = std::sin(phi);
        float cosT = std::cos(theta);
        float cosP = std::cos(phi);
        vec3<TFloat> directionInit{cosP* sinT, cosT, sinP* sinT};
        vec3<TFloat> direction = directionInit * frame;

        sampleRay = vira::rendering::Ray<TSpectral, TFloat>(point, direction);


        // Dividing by PDF same as multiplying by pdfInv:
        // TFloat pdf = TFloat{ 1 } / Omega;

        float r2 = static_cast<float>(this->radius * this->radius);
        float Omega = 2 * PI<float>() * (1.f - std::sqrt((distance * distance) - r2) / distance);
        pdf = 1.f / Omega;

        return this->spectralRadiance;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float SphereLight<TSpectral, TFloat, TMeshFloat>::getPDF(const vec3<TFloat>& intersection, const vec3<TFloat>& direction)
    {
        // First check if the direction can possibly hit the sphere
        vec3<TFloat> toCenter = this->getGlobalPosition() - intersection;
        float distanceToCenter = length(toCenter);

        // Handle inside-sphere case
        if (distanceToCenter <= this->radius) {
            return 1.0f / (4.0f * PI<float>() * this->radius * this->radius);
        }

        // Check if direction is within the cone that can hit the sphere
        vec3<TFloat> toCenterNorm = toCenter / distanceToCenter;
        float cosMaxAngle = sqrt(1.0f - (this->radius * this->radius) / (distanceToCenter * distanceToCenter));

        float cosAngle = dot(direction, toCenterNorm);
        if (cosAngle < cosMaxAngle) {
            return 0.0f;  // Direction is outside the cone that can hit the sphere
        }

        // Direction can hit the sphere - return the same PDF as sample()
        float r2 = static_cast<float>(this->radius * this->radius);
        float d2 = distanceToCenter * distanceToCenter;
        float Omega = 2 * PI<float>() * (1.0f - sqrt(d2 - r2) / distanceToCenter);

        return 1.0f / Omega;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral SphereLight<TSpectral, TFloat, TMeshFloat>::getIrradiance(const vec3<TFloat>& point)
    {
        vec3<TFloat> pointToLight = this->getGlobalPosition() - point;
        float distance = static_cast<float>(length(pointToLight));

        float r2 = static_cast<float>(this->radius * this->radius);
        float Omega = 2 * PI<float>() * (1.f - std::sqrt((distance * distance) - r2) / distance);

        return this->spectralRadiance * Omega;
    };
};