#include <memory>
#include <limits>
#include <array>

#include "vira/vec.hpp"
#include "vira/reference_frame.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/scene/ids.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    TLASLeaf<TSpectral, TFloat, TMeshFloat>::TLASLeaf(BLAS<TSpectral, TFloat, TMeshFloat>* newBlas, mat4<TFloat> global_transformation, MeshID new_mesh_id, InstanceID new_instance_id)
        : blas{ newBlas }, local_to_global_{ global_transformation }, mesh_id{ new_mesh_id }, instance_id{ new_instance_id }
    {
        AABB<TSpectral, TFloat> childAABB = blas->getAABB();
        this->aabb = childAABB.applyTransformState(local_to_global_);

        global_to_local_ = inverse(local_to_global_);
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void TLASLeaf<TSpectral, TFloat, TMeshFloat>::intersect(Ray<TSpectral, TFloat>& ray)
    {       
        // TODO UPDATE THIS TO PAY CLOSE ATTENTION TO NUMERICAL PRECISION
        
        // Transform ray origin to local space
        vec3<TMeshFloat> local_origin = transformPoint(global_to_local_, ray.origin);
        vec3<TMeshFloat> local_direction = transformDirection(global_to_local_, ray.direction);
        Ray<TSpectral, TMeshFloat> local_ray(local_origin, normalize(local_direction));

        // Contract the hit distance into the new frame:
        TFloat contraction = length(local_direction);
        local_ray.hit = ray.hit;
        local_ray.hit.t = static_cast<TMeshFloat>(contraction * ray.hit.t);
        TMeshFloat initialDist = static_cast<TMeshFloat>(local_ray.hit.t);

        // Intersect transformed ray with the BLAS:
        blas->intersect(local_ray);

        // Apply transformState:
        if (local_ray.hit.t < initialDist) {
            ray.hit = local_ray.hit;
            ray.hit.t = (TFloat{ 1 } / contraction) * ray.hit.t;
            ray.hit.globla_transformation_ = local_to_global_;
            ray.hit.instance_id = instance_id;
        }
    };
};