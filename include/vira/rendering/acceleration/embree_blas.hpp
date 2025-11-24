#ifndef VIRA_RENDERING_ACCELERATION_EMBREE_BLAS_HPP
#define VIRA_RENDERING_ACCELERATION_EMBREE_BLAS_HPP

#include <array>
#include <cstddef>

// TODO REMOVE THIRD PARTY HEADERS:
#include "embree3/rtcore.h"

#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/blas.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/rendering/acceleration/embree_options.hpp"

namespace vira::rendering {
    // Forward Declare:
    template <IsSpectral TSpectral>
    class EmbreeTLAS;

    template <IsSpectral TSpectral, IsFloat TFloat>
    class EmbreeBLAS : public BLAS<TSpectral, TFloat, float> {
    public:
        EmbreeBLAS() = default;
        EmbreeBLAS(RTCDevice device, vira::geometry::Mesh<TSpectral, TFloat, float>* newMesh, BVHBuildOptions buildOptions);
        ~EmbreeBLAS() override;

        void intersect(Ray<TSpectral, float>& ray) override;

        template <size_t N> requires ValidPacketSize<N>
        void intersectPacket(std::array<Ray<TSpectral, float>, N>& rayPacket);

        AABB<TSpectral, TFloat> getAABB() override;

    private:
        RTCScene scene;
        RTCGeometry geom;
        RTCIntersectContext context;

        friend class EmbreeTLAS<TSpectral>;
    };
};

#include "implementation/rendering/acceleration/embree_blas.ipp"

#endif