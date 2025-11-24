#ifndef VIRA_RENDERING_ACCELERATION_AABB_HPP
#define VIRA_RENDERING_ACCELERATION_AABB_HPP

#include <limits>
#include <array>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/obb.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat>
    class AABB {
    public:
        AABB() = default;
        AABB(vec3<TFloat> bmin, vec3<TFloat> bmax);

        template <IsFloat TFloat2>
        operator AABB<TSpectral, TFloat2>() {
            vec3<TFloat2> castMin{ this->bmin_[0], this->bmin_[1], this->bmin_[2] };
            vec3<TFloat2> castMax{ this->bmax_[0], this->bmax_[1], this->bmax_[2] };

            return AABB<TSpectral, TFloat2>(castMin, castMax);
        }

        const vec3<TFloat>& min() const { return bmin_; }
        const vec3<TFloat>& max() const { return bmax_; }

        vec3<TFloat> extent() const { return bmax_ - bmin_; }
        vec3<TFloat> center() const { return (bmin_ + bmax_) / 2; }

        void grow(const vec3<TFloat>& newPoint);
        void grow(const AABB<TSpectral, TFloat>& aabb);
        TFloat area() const;
        vec3<TFloat> vecMax(const vec3<TFloat>& a, const vec3<TFloat>& b) const;
        vec3<TFloat> vecMin(const vec3<TFloat>& a, const vec3<TFloat>& b) const;
        std::array<vec3<TFloat>, 8> getCorners() const;
        std::array<std::array<vec3<TFloat>, 4>, 6> getFaces() const;

        TFloat intersect(const Ray<TSpectral, TFloat>& ray) const;

        AABB<TSpectral, TFloat> applyTransformation(const mat4<TFloat>& transformation) const;

        OBB<TFloat> toOBB(const mat4<TFloat>& transformation) const;

    private:
        vec3<TFloat> bmin_{ std::numeric_limits<TFloat>::infinity() };
        vec3<TFloat> bmax_{ -std::numeric_limits<TFloat>::infinity() };
    };
};

#include "implementation/rendering/acceleration/aabb.ipp"

#endif