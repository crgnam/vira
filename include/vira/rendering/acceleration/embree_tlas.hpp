#ifndef VIRA_RENDERING_ACCELERATION_EMBREE_TLAS_HPP
#define VIRA_RENDERING_ACCELERATION_EMBREE_TLAS_HPP

#include <vector>
#include <cstddef>
#include <array>

#include "vira/spectral_data.hpp"
#include "vira/rendering/acceleration/tlas.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/rendering/acceleration/embree_blas.hpp"
#include "vira/rendering/acceleration/embree_options.hpp"
#include "vira/scene/ids.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral>
    struct InstanceMeshData {
        vira::geometry::Mesh<TSpectral, float, float>* mesh;
        vira::scene::Instance<TSpectral, float, float>* instance;
    };

    template <IsSpectral TSpectral>
    class EmbreeTLAS : public TLAS<TSpectral, float, float> {
    public:
        EmbreeTLAS(RTCDevice device, EmbreeOptions embree_options);
        ~EmbreeTLAS() override;

        // Delete copy:
        EmbreeTLAS(const EmbreeTLAS&) = delete;
        EmbreeTLAS& operator=(const EmbreeTLAS&) = delete;

        // Implement move:
        EmbreeTLAS(EmbreeTLAS&& other) noexcept;
        EmbreeTLAS& operator=(EmbreeTLAS&& other) noexcept;

        void newInstance(RTCDevice device, EmbreeBLAS<TSpectral, float>* blas, vira::geometry::Mesh<TSpectral, float, float>* mesh, vira::scene::Instance<TSpectral, float, float>* instance);
        void build();

        void init() override;

        void intersect(Ray<TSpectral, float>& ray) override;

        template <size_t N> requires ValidPacketSize<N>
        void intersectPacket(std::array<Ray<TSpectral, float>, N>& rayPacket);

    private:
        RTCScene global_scene_;
        thread_local static RTCIntersectContext context_;

        std::vector<InstanceMeshData<TSpectral>> instance_array_;
    };
};

#include "implementation/rendering/acceleration/embree_tlas.ipp"

#endif