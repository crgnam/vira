#include <limits>
#include <algorithm>
#include <array>

#include "vira/vec.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat>
    AABB<TSpectral, TFloat>::AABB(vec3<TFloat> bmin, vec3<TFloat> bmax) :
        bmin_{ bmin }, bmax_{ bmax }
    {

    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    void AABB<TSpectral, TFloat>::grow(const vec3<TFloat>& newPoint)
    {
        bmin_ = vecMin(bmin_, newPoint);
        bmax_ = vecMax(bmax_, newPoint);
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    void AABB<TSpectral, TFloat>::grow(const AABB<TSpectral, TFloat>& b) {
        if (!std::isinf(b.bmin_[0])) {
            grow(b.bmin_);
            grow(b.bmax_);
        }
    }

    template <IsSpectral TSpectral, IsFloat TFloat>
    TFloat AABB<TSpectral, TFloat>::area() const
    {
        vec3<TFloat> e = bmax_ - bmin_;
        return (e.x * e.y + e.y * e.z + e.z * e.x);
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    vec3<TFloat> AABB<TSpectral, TFloat>::vecMax(const vec3<TFloat>& a, const vec3<TFloat>& b) const
    {
        return vec3<TFloat>(std::max(a.x, b.x),
            std::max(a.y, b.y),
            std::max(a.z, b.z));
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    vec3<TFloat> AABB<TSpectral, TFloat>::vecMin(const vec3<TFloat>& a, const vec3<TFloat>& b) const
    {
        return vec3<TFloat>(std::min(a.x, b.x),
            std::min(a.y, b.y),
            std::min(a.z, b.z));
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    std::array<vec3<TFloat>, 8> AABB<TSpectral, TFloat>::getCorners() const
    {
        std::array<vec3<TFloat>, 8> corners;
        corners[0] = { bmin_[0], bmin_[1], bmin_[2] }; // 0 back left bottom
        corners[1] = { bmin_[0], bmin_[1], bmax_[2] }; // 1 back left top
        corners[2] = { bmin_[0], bmax_[1], bmin_[2] }; // 2 back right bottom
        corners[3] = { bmax_[0], bmin_[1], bmin_[2] }; // 3 front left bottom
        corners[4] = { bmax_[0], bmax_[1], bmin_[2] }; // 4 front right bottom
        corners[5] = { bmax_[0], bmin_[1], bmax_[2] }; // 5 front left top
        corners[6] = { bmin_[0], bmax_[1], bmax_[2] }; // 6 back right top
        corners[7] = { bmax_[0], bmax_[1], bmax_[2] }; // 7 front right top
        return corners;
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    std::array<std::array<vec3<TFloat>, 4>, 6> AABB<TSpectral, TFloat>::getFaces() const
    {
        std::array<vec3<TFloat>, 8> corners = this->getCorners();
        std::array<std::array<vec3<TFloat>, 4>, 6> faces;

        faces[0] = { corners[0], corners[2], corners[4], corners[3] }; // Bottom face
        faces[1] = { corners[1], corners[5], corners[7], corners[6] }; // Top face
        faces[2] = { corners[0], corners[3], corners[5], corners[1] }; // Left face
        faces[3] = { corners[2], corners[6], corners[7], corners[4] }; // Right face
        faces[4] = { corners[0], corners[1], corners[6], corners[2] }; // Back face
        faces[5] = { corners[3], corners[4], corners[7], corners[5] }; // Front face

        return faces;
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    TFloat AABB<TSpectral, TFloat>::intersect(const Ray<TSpectral, TFloat>& ray) const
    {
        // Need to rationalize about templated float/double vs register size
        TFloat tx1 = (bmin_.x - ray.origin.x) * ray.reciprocal_direction.x;
        TFloat tx2 = (bmax_.x - ray.origin.x) * ray.reciprocal_direction.x;

        TFloat tmin = std::min(tx1, tx2);
        TFloat tmax = std::max(tx1, tx2);

        TFloat ty1 = (bmin_.y - ray.origin.y) * ray.reciprocal_direction.y;
        TFloat ty2 = (bmax_.y - ray.origin.y) * ray.reciprocal_direction.y;

        tmin = std::max(tmin, std::min(ty1, ty2));
        tmax = std::min(tmax, std::max(ty1, ty2));

        TFloat tz1 = (bmin_.z - ray.origin.z) * ray.reciprocal_direction.z;
        TFloat tz2 = (bmax_.z - ray.origin.z) * ray.reciprocal_direction.z;

        tmin = std::max(tmin, std::min(tz1, tz2));
        tmax = std::min(tmax, std::max(tz1, tz2));

        if ((tmax >= tmin) && (tmin < ray.hit.t) && (tmax > 0)) {
            return tmin;
        }
        else {
            return std::numeric_limits<TFloat>::infinity();
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    AABB<TSpectral, TFloat> AABB<TSpectral, TFloat>::applyTransformation(const mat4<TFloat>& transformation) const
    {
        AABB<TSpectral, TFloat> newAABB;
        std::array<vec3<TFloat>, 8> corners = this->getCorners();
        for (vec3<TFloat>& corner : corners) {
            vec3<TFloat> newCorner = transformPoint(transformation, corner);
            newAABB.grow(newCorner);
        }

        return newAABB;
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    OBB<TFloat> AABB<TSpectral, TFloat>::toOBB(const mat4<TFloat>& transformation) const
    {
        vec3<TFloat> position = ReferenceFrame<TFloat>::getPositionFromTransformation(transformation);
        vec3<TFloat> scale = ReferenceFrame<TFloat>::getScaleFromTransformation(transformation);
        Rotation<TFloat> rotation = ReferenceFrame<TFloat>::getRotationFromTransformation(transformation, scale);
        
        vec3<TFloat> bcenter = position + (scale * this->center());
        vec3<TFloat> bhalfSize = scale * (this->extent() / 2);
        
        return OBB<TFloat>(bcenter, bhalfSize, rotation);
    };
};