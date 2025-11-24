#ifndef VIRA_RENDERING_ACCELERATION_VIRA_BLAS_HPP
#define VIRA_RENDERING_ACCELERATION_VIRA_BLAS_HPP

#include <vector>
#include <memory>
#include <iostream>

#include "vira/constraints.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/nodes.hpp"
#include "vira/rendering/acceleration/blas.hpp"

// Forward Declare:
namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Mesh;
};

namespace vira::rendering {
    template <IsSpectral TSpectral>
    struct ViraBLASNode : public TreeNode<TSpectral, double>
    {
        size_t leftFirst;
        size_t triCount;

        bool isLeaf() { return triCount > 0; }
    };

    template <IsSpectral TSpectral>
    class ViraBLAS : public BLAS<TSpectral, double, double> {
    public:
        ViraBLAS() = default;

        ViraBLAS(vira::geometry::Mesh<TSpectral, double, double>* newMesh, BVHBuildOptions buildOptions);

        ~ViraBLAS() override;

        void build();

        void intersect(Ray<TSpectral, double>& ray) override;

        AABB<TSpectral, double> getAABB() override;

    private:
        struct Bin {
            AABB<TSpectral, double> bounds;
            size_t triCount = 0;
        };

        size_t numTriangles = 0;

        ViraBLASNode<TSpectral>* bvhNode = nullptr;
        size_t* triIdx = nullptr;
        size_t nodesUsed = 1;


        // Construction methods:
        void updateNodeBounds(size_t nodeIdx);

        void subdivide(size_t nodeIdx);

        double findBestSplitPlane(ViraBLASNode<TSpectral>& node, int& axis, double& splitPos);

        double evaluateSAH(ViraBLASNode<TSpectral>& node, int axis, double pos);

        double calculateNodeCost(ViraBLASNode<TSpectral>& node);
    };
};

#include "implementation/rendering/acceleration/vira_blas.ipp"

#endif
