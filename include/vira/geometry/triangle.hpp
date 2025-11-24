#ifndef VIRA_GEOMETRY_TRIANGLE_HPP
#define VIRA_GEOMETRY_TRIANGLE_HPP

#include <cstdint>
#include <vector>
#include <array>

#include "vira/math.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/materials/material.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/scene/ids.hpp"

namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    struct Triangle {
        std::array<Vertex<TSpectral, TMeshFloat>, 3> vert;
        vec3<TMeshFloat> face_normal;
        vec3<TMeshFloat> centroid{};

        bool smooth_shading = false;
        MaterialID::ValueType material_cache_index = 0;

        std::array<vec3<TMeshFloat>, 2> e;
        vec3<TMeshFloat> n;

        Triangle() = default;
        Triangle(Vertex<TSpectral, TMeshFloat> v0, Vertex<TSpectral, TMeshFloat> v1, Vertex<TSpectral, TMeshFloat> v2, bool smooth, MaterialID::ValueType new_material_cache_index);
        Triangle(std::array<Vertex<TSpectral, TMeshFloat>, 3> new_vert, bool smooth, MaterialID::ValueType new_material_cache_index);

        void init();

        void intersect(vira::rendering::Ray<TSpectral, TMeshFloat>& ray, size_t tri_index, void* mesh_ptr = nullptr) const;
    };

    template <IsFloat T>
    T edgeFunction(const vec2<T>& a, const vec2<T>& b, const vec2<T>& c);
};

#include "implementation/geometry/triangle.ipp"

#endif