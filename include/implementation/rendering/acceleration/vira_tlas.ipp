#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/acceleration/nodes.hpp"
#include "vira/rendering/ray.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    ViraTLAS<TSpectral, TMeshFloat>::ViraTLAS(std::vector<TLASLeaf<TSpectral, double, TMeshFloat>> leafList)
    {
        leafs = leafList;
        tlasNode = new ViraTLASNode<TSpectral>[2 * leafList.size()];
        nodesUsed = 0;

        this->build();
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    ViraTLAS<TSpectral, TMeshFloat>::~ViraTLAS()
    {
        delete[] tlasNode;
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    void ViraTLAS<TSpectral, TMeshFloat>::build()
    {
        if (leafs.size() == 0) {
            throw std::runtime_error("ViraTLAS Cannot be built when no leaf nodes are provided.  Did you add any models to the Scene?");
        }
        size_t* nodeIdx = new size_t[leafs.size()];

        // assign a ViraTLASleaf node to each BLAS
        size_t nodeIndices = static_cast<size_t>(leafs.size());
        nodesUsed = 1;
        for (size_t i = 0; i < static_cast<size_t>(leafs.size()); i++)
        {
            nodeIdx[i] = nodesUsed;
            tlasNode[nodesUsed].aabb = leafs[i].getAABB();
            tlasNode[nodesUsed].leafID = i;
            tlasNode[nodesUsed++].leftRight = 0; // makes it a leaf
        }

        // use agglomerative clustering to build the ViraTLAS
        size_t A = 0;
        size_t B = findBestMatch(nodeIdx, nodeIndices, A);
        while (nodeIndices > 1) {
            size_t C = findBestMatch(nodeIdx, nodeIndices, B);
            if (A == C) {
                size_t nodeIdxA = nodeIdx[A];
                size_t nodeIdxB = nodeIdx[B];

                ViraTLASNode<TSpectral>& nodeA = tlasNode[nodeIdxA];
                ViraTLASNode<TSpectral>& nodeB = tlasNode[nodeIdxB];
                ViraTLASNode<TSpectral>& newNode = tlasNode[nodesUsed];

                newNode.leftRight = nodeIdxA + (nodeIdxB << 16);
                newNode.aabb.grow(nodeA.aabb);
                newNode.aabb.grow(nodeB.aabb);

                nodeIdx[A] = nodesUsed++;
                nodeIdx[B] = nodeIdx[nodeIndices - 1];
                B = findBestMatch(nodeIdx, --nodeIndices, A);
            }
            else {
                A = B;
                B = C;
            }
        }
        tlasNode[0] = tlasNode[nodeIdx[A]];

        aabb_ = tlasNode[0].aabb;

        delete[] nodeIdx;
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    size_t ViraTLAS<TSpectral, TMeshFloat>::findBestMatch(size_t* list, size_t N, size_t A)
    {
        double smallest = std::numeric_limits<double>::infinity();
        size_t bestB = 0;
        for (size_t B = 0; B < N; B++) if (B != A)
        {
            AABB<TSpectral, double> aabb;
            aabb.grow(tlasNode[list[A]].aabb);
            aabb.grow(tlasNode[list[B]].aabb);

            vec3<double> e = aabb.max() - aabb.min();
            double surfaceArea = e[0] * e[1] + e[1] * e[2] + e[2] * e[0];
            if (surfaceArea < smallest) {
                smallest = surfaceArea;
                bestB = B;
            }
        }
        return bestB;
    };

    template <IsSpectral TSpectral, IsFloat TMeshFloat>
    void ViraTLAS<TSpectral, TMeshFloat>::intersect(Ray<TSpectral, double>& ray)
    {
        ViraTLASNode<TSpectral>* node = &tlasNode[0];
        ViraTLASNode<TSpectral>* stack[64];
        size_t stackPtr = 0;

        while (true) {
            if (node->isLeaf()) {
                leafs[node->leafID].intersect(ray);
                if (stackPtr == 0) {
                    break;
                }
                else {
                    node = stack[--stackPtr];
                }
                continue;
            }
            ViraTLASNode<TSpectral>* child1 = &tlasNode[node->leftRight & 0xffff];
            ViraTLASNode<TSpectral>* child2 = &tlasNode[node->leftRight >> 16];

            double dist1 = child1->aabb.intersect(ray);
            double dist2 = child2->aabb.intersect(ray);
            if (dist1 > dist2) {
                std::swap(dist1, dist2);
                std::swap(child1, child2);
            }

            if (std::isinf(dist1))
            {
                if (stackPtr == 0) {
                    break;
                }
                else {
                    node = stack[--stackPtr];
                }
            }
            else
            {
                node = child1;
                if (!std::isinf(dist2)) {
                    stack[stackPtr++] = child2;
                }
            }
        }
    };
};