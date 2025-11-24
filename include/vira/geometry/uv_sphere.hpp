#ifndef VIRA_GEOMETRY_UV_SPHERE_HPP
#define VIRA_GEOMETRY_UV_SPHERE_HPP

#include <cstddef>
#include <memory>

#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/geometry/mesh.hpp"

namespace vira::geometry {
    inline IndexBuffer uvSphereIndexBuffer(size_t numCuts, size_t numRings);

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::unique_ptr<Mesh<TSpectral, TFloat, TMeshFloat>> makeUVSphere(TMeshFloat radius = 1, size_t numCuts = 32, size_t numRings = 16);
};

#include "implementation/geometry/uv_sphere.ipp"

#endif