#include <array>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/rotation.hpp"

namespace vira::rendering {
    template <IsFloat TFloat>
    OBB<TFloat>::OBB(vec3<TFloat> bcenter, vec3<TFloat> bhalfSize, Rotation<TFloat> rotation) :
        bcenter_{ bcenter }, bhalfSize_{ bhalfSize }
    {
        auto matrix = rotation.getMatrix();
        baxes_[0] = vec3<TFloat>{ matrix[0][0], matrix[0][1], matrix[0][2] };
        baxes_[1] = vec3<TFloat>{ matrix[1][0], matrix[1][1], matrix[1][2] };
        baxes_[2] = vec3<TFloat>{ matrix[2][0], matrix[2][1], matrix[2][2] };
    }

    template <IsFloat TFloat>
    OBB<TFloat>::OBB(vec3<TFloat> bcenter, vec3<TFloat> bhalfSize, std::array<vec3<TFloat>, 3> baxes) :
        bcenter_{ bcenter }, bhalfSize_{ bhalfSize }, baxes_{ baxes }
    {

    }

    template <IsFloat TFloat>
    std::array<vec3<TFloat>, 8> OBB<TFloat>::getCorners() const
    {
        std::array<vec3<TFloat>, 8> corners;

        for (int i = 0; i < 8; ++i) {
            vec3<TFloat> corner = bcenter_;
            for (int j = 0; j < 3; ++j) {
                TFloat sign = static_cast<TFloat>((i & (1 << j)) ? 1 : -1);
                corner += sign * bhalfSize_[j] * baxes_[static_cast<size_t>(j)];
            }
            corners[static_cast<size_t>(i)] = corner;
        }
        return corners;
    }

    template <IsFloat TFloat>
    std::array<std::array<size_t, 2>, 12> OBB<TFloat>::getEdgeIndices() const
    {
        std::array<std::array<size_t, 2>, 12> edgeIndices {{
            {{0,1}}, {{1,3}}, {{3,2}}, {{2,0}},  // front face
            {{4,5}}, {{5,7}}, {{7,6}}, {{6,4}},  // back face
            {{0,4}}, {{1,5}}, {{2,6}}, {{3,7}}   // connecting edges
        }};

        return edgeIndices;
    }
};