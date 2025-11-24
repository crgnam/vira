#ifndef VIRA_RENDERING_RAY_HPP
#define VIRA_RENDERING_RAY_HPP

#include <limits>
#include <array>

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/scene/ids.hpp"

// Forward Declaration:
namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Mesh;

    template <IsSpectral TSpectral, IsFloat TFloat>
    struct Triangle;
};

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Instance;
}

namespace vira::rendering {
    // Forward Declaration:
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class CPUPathTracer;

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class CPURasterizer;

    template <IsSpectral TSpectral, IsFloat TFloat>
    class EmbreeBLAS;

    template <IsSpectral TSpectral>
    class EmbreeTLAS;

    template <IsSpectral TSpectral, IsFloat TFloat>
    struct Interaction {
        // Intersection information:
        TFloat t = std::numeric_limits<TFloat>::infinity();

        vec3<float> face_normal;
        std::array<geometry::Vertex<TSpectral, TFloat>,3> vert;
        std::array<TFloat,3> w; // Barycentric coordinates of intersection

        // Triangle/mesh information:
        size_t tri_id = std::numeric_limits<size_t>::max();
        
        MaterialID::ValueType material_cache_index = 0;

        // Allow for casting interactions between type instantiations:
        template <IsFloat TFloat2>
        operator Interaction<TSpectral, TFloat2>();

    private:
        void* instance_ptr = nullptr;
        void* mesh_ptr = nullptr; // Type-erasure of Mesh<TSpectral, TFloat, TMeshFloat> used by Renderers

        template<IsFloat TMeshFloat>
        vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>* getMeshPtr() const {
            return static_cast<vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>*>(this->mesh_ptr);
        }

        template<IsFloat TMeshFloat>
        vira::scene::Instance<TSpectral, TFloat, TMeshFloat>* getInstancePtr() const {
            return static_cast<vira::scene::Instance<TSpectral, TFloat, TMeshFloat>*>(this->instance_ptr);
        }


        // Friend renderers that can access the mesh_ptr pointer
        template <IsSpectral T2Spectral, IsFloat T2Float, IsFloat TMeshFloat> requires LesserFloat<T2Float, TMeshFloat>
        friend class CPUPathTracer;

        template <IsSpectral T2Spectral, IsFloat T2Float, IsFloat TMeshFloat> requires LesserFloat<T2Float, TMeshFloat>
        friend class CPURasterizer;

        friend struct vira::geometry::Triangle<TSpectral, TFloat>;
        friend class EmbreeBLAS<TSpectral, TFloat>;
        friend class EmbreeTLAS<TSpectral>;
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    struct Ray {
        vec3<TFloat> origin{0,0,0};
        vec3<TFloat> direction{0,0,-1};
        vec3<TFloat> reciprocal_direction{ 0,0,-1 };

        Interaction<TSpectral, TFloat> hit;

        Ray() = default;
        Ray(vec3<TFloat> newOrigin, vec3<TFloat> newDirection);
    };


    template <IsFloat TMeshFloat>
    inline vec3<TMeshFloat> offsetIntersection(vec3<TMeshFloat> intersection, const vec3<float>& N);
};

#include "implementation/rendering/ray.ipp"

#endif