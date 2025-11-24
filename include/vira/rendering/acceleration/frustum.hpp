#ifndef VIRA_RENDERING_ACCELERATION_FRUSTUM_HPP
#define VIRA_RENDERING_ACCELERATION_FRUSTUM_HPP

#include <array>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/obb.hpp"

namespace vira::rendering {
    // Declaration and Implementation of Plane class used by precomputeFrustum
    template <IsFloat TFloat>
    class Plane {
    public:
        Plane() = default;
        Plane(const vec3<TFloat>& p1, const vec3<TFloat>& p2, const vec3<TFloat>& p3);
        Plane(vec3<TFloat> normal, TFloat distance);

        TFloat getDistance() const { return distance_; }
        const vec3<TFloat>& getNormal() const { return normal_; }
        bool inside(const vec3<TFloat>& point) const;

    private:
        vec3<TFloat> normal_{ 0,0,1 };
        TFloat distance_ = 0;
    };

    // Function to test if an AABB<TSpectral, TFloat> intersects a frustum defined by 6 Plane<TFloat>
    template <IsFloat TFloat, IsSpectral TSpectral, size_t N>
    bool intersectsFrustum(const AABB<TSpectral, TFloat>& aabb, const std::array<Plane<TFloat>, N>& frustumPlanes);

    template <IsFloat TFloat, size_t N>
    bool intersectsFrustum(const OBB<TFloat>& obb, const std::array<Plane<TFloat>, N>& frustumPlanes);

    template <IsFloat TFloat>
    std::array<Plane<TFloat>, 6> makeShadowFrustum(const OBB<TFloat>& obb, const vec3<TFloat>& lightPosition);
};

#include "implementation/rendering/acceleration/frustum.ipp"

#endif