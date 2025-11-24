#ifndef VIRA_UNRESOLVED_UNRESOLVED_OBJECT_HPP
#define VIRA_UNRESOLVED_UNRESOLVED_OBJECT_HPP

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/scene/ids.hpp"

// Forward Declaration:
namespace vira::lights {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Light;
}

namespace vira::unresolved {
    template <IsSpectral TSpectral>
    TSpectral powerToIrradiance(TSpectral emissivePower, float distance);

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TSpectral lambertianSphereToIrradiance(float radius, const vec3<TFloat>& spherePosition, const vec3<TFloat>& observerPosition, vira::lights::Light<TSpectral, TFloat, TMeshFloat>& light, TSpectral albedo = TSpectral{ 1 });

    template <IsSpectral TSpectral, IsFloat TFloat>
    TSpectral asteroidHGToIrradiance(float H, float G, const vec3<TFloat>& asteroidPosition, const vec3<TFloat>& observerPosition, const vec3<TFloat>& sunPosition, TSpectral albedo = TSpectral{ 1 });

    template <IsSpectral TSpectral>
    TSpectral visualMagnitudeToIrradiance(float V, TSpectral albedo = TSpectral{ 1 });


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class UnresolvedObject : public ReferenceFrame<TFloat> {
    public:
        UnresolvedObject() = default;
        UnresolvedObject(TSpectral irradiance);

        // Delete copying
        UnresolvedObject(const UnresolvedObject&) = delete;
        UnresolvedObject& operator=(const UnresolvedObject&) = delete;

        // Allow moving
        UnresolvedObject(UnresolvedObject&&) = default;
        UnresolvedObject& operator=(UnresolvedObject&&) = default;
        
        void setIrradiance(TSpectral irradiance);
        void setIrradianceFromPower(TSpectral emissivePower, float distance);
        void setIrradianceFromLambertianSphere(float radius, const vec3<TFloat>& observerPosition, vira::lights::Light<TSpectral, TFloat, TMeshFloat>& light, TSpectral albedo = TSpectral{ 1 });
        void setIrradianceFromAsteroidHG(float H, float G, const vec3<TFloat>& observerPosition, const vec3<TFloat>& sunPosition, TSpectral albedo = TSpectral{ 1 });
        void setIrradianceFromVisualMagnitude(float V, TSpectral albedo = TSpectral{ 1 });

        TSpectral getIrradiance() const { return irradiance; }

        const UnresolvedID& getID() { return id_; }

    private:
        TSpectral irradiance{ 0 };

        UnresolvedID id_{};

        friend class vira::scene::Group<TSpectral, TFloat, TMeshFloat>;
    };
};

#include "implementation/unresolved/unresolved_object.ipp"

#endif