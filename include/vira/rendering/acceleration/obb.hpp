#ifndef VIRA_RENDERING_ACCELERATION_OBB_HPP
#define VIRA_RENDERING_ACCELERATION_OBB_HPP

#include <array>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/rotation.hpp"

namespace vira::rendering {
    template <IsFloat TFloat>
    class OBB {
    public:
        OBB() = default;
        OBB(vec3<TFloat> bcenter, vec3<TFloat> bhalfSize, Rotation<TFloat> rotation);
        OBB(vec3<TFloat> bcenter, vec3<TFloat> bhalfSize, std::array<vec3<TFloat>, 3> baxes);

        template <IsFloat TFloat2>
        operator OBB<TFloat2>() {
            vec3<TFloat2> castCenter = this->bcenter_;
            vec3<TFloat2> castSize = this->bhalfSize_;
            std::array<vec3<TFloat2>, 3> castAxes;
            for (size_t i = 0; i < 3; ++i) {
                castAxes[i] = baxes_[i];
            }
            return OBB<TFloat2>(castCenter, castSize, castAxes);
        }

        const vec3<TFloat>& center() const { return bcenter_; }
        const vec3<TFloat>& halfSize() const { return bhalfSize_; }
        const std::array<vec3<TFloat>, 3>& axes() const { return baxes_; }

        std::array<vec3<TFloat>, 8> getCorners() const;
        std::array<std::array<size_t, 2>, 12> getEdgeIndices() const;

    private:
        vec3<TFloat> bcenter_{ 0,0,0 };
        vec3<TFloat> bhalfSize_{ 0, 0,0 };
        
        std::array<vec3<TFloat>, 3> baxes_;
    };
};

#include "implementation/rendering/acceleration/obb.ipp"

#endif