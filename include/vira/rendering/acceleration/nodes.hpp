#ifndef VIRA_RENDERING_ACCELERATION_NODES_HPP
#define VIRA_RENDERING_ACCELERATION_NODES_HPP

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/blas.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/scene/ids.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat>
    struct TreeNode
    {
        AABB<TSpectral, TFloat> aabb;
    };


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class TLASLeaf {
    public:
        TLASLeaf(BLAS<TSpectral, TFloat, TMeshFloat>* blas, mat4<TFloat> global_transformation, MeshID mesh_id, InstanceID instance_id);

        void intersect(Ray<TSpectral, TFloat>& ray);

        AABB<TSpectral, TFloat> getAABB() { return aabb; }

    private:
        BLAS<TSpectral, TFloat, TMeshFloat>* blas;
        mat4<TFloat> local_to_global_;
        mat4<TFloat> global_to_local_;
        MeshID mesh_id;
        InstanceID instance_id;

        AABB<TSpectral, TFloat> aabb;
    };
};

#include "implementation/rendering/acceleration/nodes.ipp"

#endif
