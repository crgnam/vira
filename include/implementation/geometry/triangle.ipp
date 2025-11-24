#include <cstdint>
#include <limits>
#include <algorithm>
#include <memory>
#include <array>

#include "glm/glm.hpp"

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/constraints.hpp"
#include "vira/scene/ids.hpp"

namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    Triangle<TSpectral, TMeshFloat>::Triangle(Vertex<TSpectral, TMeshFloat> v0, Vertex<TSpectral, TMeshFloat> v1, Vertex<TSpectral, TMeshFloat> v2, bool smooth, MaterialID::ValueType new_material_cache_index)
        : smooth_shading{ smooth }, material_cache_index{ new_material_cache_index }
    {
        vert[0] = v0;
        vert[1] = v1;
        vert[2] = v2;

        init();
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    Triangle<TSpectral, TMeshFloat>::Triangle(std::array<Vertex<TSpectral, TMeshFloat>, 3> newVert, bool smooth, MaterialID::ValueType new_material_cache_index)
        : vert{ newVert }, smooth_shading{ smooth }, material_cache_index{ new_material_cache_index }
    {
        init();
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    void Triangle<TSpectral, TMeshFloat>::init()
    {
        centroid = TMeshFloat{ 1.f / 3.f } *(vert[0].position + vert[1].position + vert[2].position);

        e[0] = vert[0].position - vert[1].position;
        e[1] = vert[2].position - vert[0].position;
        n = glm::cross(e[0], e[1]);

        face_normal = -glm::normalize(n);
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    void Triangle<TSpectral, TMeshFloat>::intersect(vira::rendering::Ray<TSpectral, TMeshFloat>& ray, size_t tri_index, void* mesh_ptr) const
    {
        // This heuristic is acceptable only sometimes:
        static constexpr TMeshFloat tol = static_cast<TMeshFloat>(0.00001);

        // TODO Why is the winding order here OPPOSITE what it should be?  (v2, v1, v0) instead of (v0,v1,v2)
        // This is only an issue in the path-tracer, not the rasterizer.

        vec3<TMeshFloat> c = vert[0].position - ray.origin;
        vec3<TMeshFloat> r = glm::cross(ray.direction, c);
        TMeshFloat inv_det = static_cast<TMeshFloat>(1.) / glm::dot(n, ray.direction);

        std::array<TMeshFloat, 3> w;
        w[0] = glm::dot(r, e[1]) * inv_det;
        w[1] = glm::dot(r, e[0]) * inv_det;
        w[2] = static_cast<TMeshFloat>(1.) - w[0] - w[1];

        if (w[0] >= 0 && w[1] >= 0 && w[2] >= 0) {
            TMeshFloat t = glm::dot(n, c) * inv_det;
            if (t < ray.hit.t && t > tol) {
                ray.hit.t = t;

                // TODO Why is this winding order offet?
                // Are w[3] values being calculated incorrectly?
                // This is same for Embree rtcIntersect1, but not for Rasterizer...
                ray.hit.vert[0] = vert[1];
                ray.hit.vert[1] = vert[2];
                ray.hit.vert[2] = vert[0];

                ray.hit.w = w;

                ray.hit.face_normal = face_normal;

                ray.hit.tri_id = tri_index;
                ray.hit.material_cache_index = material_cache_index;

                // Set the type-erased mesh_ptr:
                ray.hit.mesh_ptr = mesh_ptr;
            }
        }
    };

    template <IsFloat T>
    T edgeFunction(const vec2<T>& a, const vec2<T>& b, const vec2<T>& c)
    {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    };
};