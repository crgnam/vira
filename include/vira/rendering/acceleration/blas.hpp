#ifndef VIRA_RENDERING_ACCELERATION_BLAS_HPP
#define VIRA_RENDERING_ACCELERATION_BLAS_HPP

#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/embree_options.hpp"

// Forward Declare:
namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Mesh;
};

namespace vira::rendering {
    struct BVHBuildOptions {
        EmbreeOptions embree_options{};
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class BLAS {
    public:
        virtual ~BLAS() = default;

        virtual void intersect(Ray<TSpectral, TMeshFloat>& ray) = 0;

        virtual AABB<TSpectral, TFloat> getAABB() = 0;

    protected:
        vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>* mesh = nullptr;
        void* mesh_ptr = nullptr;

        AABB<TSpectral, TFloat> aabb{};

        bool device_freed_ = false;
        friend class vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>;
    };
};

#endif