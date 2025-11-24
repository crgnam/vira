#ifndef VIRA_RENDERING_ACCELERATION_VIRA_TLAS_HPP
#define VIRA_RENDERING_ACCELERATION_VIRA_TLAS_HPP

#include <vector>
#include <memory>
#include <cstdint>

#include "vira/constraints.hpp"
#include "vira/rendering/acceleration/tlas.hpp"
#include "vira/rendering/acceleration/nodes.hpp"
#include "vira/rendering/ray.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral>
    struct ViraTLASNode : public TreeNode<TSpectral, double>
    {
        // Indirection is not needed as TLAS leaf will never contain more than one BLAS
        size_t leftRight;
        size_t leafID;
        bool isLeaf() { return leftRight == 0; }
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    class ViraTLAS : public TLAS<TSpectral, double, TMeshFloat> {
    public:
        ViraTLAS() = default;
        ~ViraTLAS() override;

        ViraTLAS(std::vector<TLASLeaf<TSpectral, double, TMeshFloat>> leafList);
        void intersect(Ray<TSpectral, double>& ray) override;

        size_t findBestMatch(size_t* list, size_t N, size_t A);

    private:
        void build();

        ViraTLASNode<TSpectral>* tlasNode = nullptr;
        std::vector<TLASLeaf<TSpectral, double, TMeshFloat>> leafs;
        size_t nodesUsed;

        AABB<TSpectral, double> aabb_;
    };
};

#include "implementation/rendering/acceleration/vira_tlas.ipp"

#endif