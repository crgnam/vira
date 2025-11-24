#include <cstdint>
#include <memory>
#include <vector>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/rendering/acceleration/aabb.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral>
    ViraBLAS<TSpectral>::ViraBLAS(vira::geometry::Mesh<TSpectral, double, double>* newMesh, BVHBuildOptions buildOptions)
    {
        (void)buildOptions;

        this->mesh = newMesh;
        this->mesh_ptr = static_cast<void*>(this->mesh); // Store type-erased pointer to Mesh

        this->mesh->constructTriangles();

        numTriangles = this->mesh->getNumTriangles();

        bvhNode = new ViraBLASNode<TSpectral>[numTriangles * 2 - 1];

        triIdx = new size_t[numTriangles];
        for (size_t i = 0; i < numTriangles; i++) {
            triIdx[i] = i;
        }

        this->build();
    };

    template <IsSpectral TSpectral>
    ViraBLAS<TSpectral>::~ViraBLAS()
    {
        delete[] bvhNode;
        delete[] triIdx;
    };

    template <IsSpectral TSpectral>
    void ViraBLAS<TSpectral>::build()
    {
        ViraBLASNode<TSpectral>& root = bvhNode[0];
        root.leftFirst = 0;
        root.triCount = numTriangles;

        updateNodeBounds(0);

        subdivide(0);

        this->aabb = root.aabb;
    };

    template <IsSpectral TSpectral>
    void ViraBLAS<TSpectral>::intersect(Ray<TSpectral, double>& ray)
    {
        ViraBLASNode<TSpectral>* node = &bvhNode[0];
        ViraBLASNode<TSpectral>* stack[64];
        size_t stackPtr = 0;
        while (1)
        {
            if (node->isLeaf())
            {
                for (size_t i = 0; i < node->triCount; i++) {
                    auto triIndex = triIdx[node->leftFirst + i];
                    const vira::geometry::Triangle<TSpectral, double>& tri = this->mesh->getTriangle(triIndex);

                    tri.intersect(ray, triIndex, this->mesh_ptr);
                }
                if (stackPtr == 0) {
                    break;
                }
                else {
                    node = stack[--stackPtr];
                }

                continue;
            }
            ViraBLASNode<TSpectral>* child1 = &bvhNode[node->leftFirst];
            ViraBLASNode<TSpectral>* child2 = &bvhNode[node->leftFirst + 1];
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

    template <IsSpectral TSpectral>
    AABB<TSpectral, double> ViraBLAS<TSpectral>::getAABB()
    {
        return bvhNode[0].aabb;
    };


    template <IsSpectral TSpectral>
    void ViraBLAS<TSpectral>::updateNodeBounds(size_t nodeIdx)
    {
        ViraBLASNode<TSpectral>& node = bvhNode[nodeIdx];
        for (size_t first = node.leftFirst, i = 0; i < node.triCount; i++)
        {
            size_t index = triIdx[first + i];
            const vira::geometry::Triangle<TSpectral, double>& tri = this->mesh->getTriangle(index);
            node.aabb.grow(tri.vert[0].position);
            node.aabb.grow(tri.vert[1].position);
            node.aabb.grow(tri.vert[2].position);
        }
    };

    template <IsSpectral TSpectral>
    void ViraBLAS<TSpectral>::subdivide(size_t nodeIdx)
    {
        // terminate recursion
        ViraBLASNode<TSpectral>& node = bvhNode[nodeIdx];
        if (node.triCount <= 2) {
            return;
        }

        // determine split axis and position using SAH
        int axis;
        double splitPos;
        double splitCost = findBestSplitPlane(node, axis, splitPos);

        if (splitCost >= calculateNodeCost(node)) {
            return;
        }

        // in-place partition
        size_t i = node.leftFirst;
        size_t j = i + node.triCount - 1;
        while (i <= j) {
            if (this->mesh->getTriangle(triIdx[i]).centroid[axis] < splitPos) {
                i++;
            }
            else {
                std::swap(triIdx[i], triIdx[j--]);
            }
        }

        // abort split if one of the sides is empty
        size_t leftCount = i - node.leftFirst;
        if (leftCount == 0 || leftCount == static_cast<size_t>(node.triCount)) {
            return;
        }

        // create child nodes
        size_t leftChildIdx = nodesUsed++;
        size_t rightChildIdx = nodesUsed++;
        bvhNode[leftChildIdx].leftFirst = node.leftFirst;
        bvhNode[leftChildIdx].triCount = leftCount;
        bvhNode[rightChildIdx].leftFirst = i;
        bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
        node.leftFirst = leftChildIdx;
        node.triCount = 0;
        updateNodeBounds(leftChildIdx);
        updateNodeBounds(rightChildIdx);

        // recurse
        subdivide(leftChildIdx);
        subdivide(rightChildIdx);
    };


    template <IsSpectral TSpectral>
    double ViraBLAS<TSpectral>::findBestSplitPlane(ViraBLASNode<TSpectral>& node, int& axis, double& splitPos)
    {
        double bestCost = std::numeric_limits<double>::max();

        // Find the longest axis:
        int a = 0;
        vec3<double> extent = node.aabb.max() - node.aabb.min();
        if (extent.y > extent.x) {
            a = 1;
        }
        if (extent.z > extent[a]) {
            a = 2;
        }

        // Calculate the centroid bounds:
        double boundsMin = std::numeric_limits<float>::infinity();
        double boundsMax = -std::numeric_limits<float>::infinity();
        for (size_t i = 0; i < node.triCount; i++)
        {
            const vira::geometry::Triangle<TSpectral, double>& tri = this->mesh->getTriangle(triIdx[node.leftFirst + i]);
            boundsMin = std::min(boundsMin, tri.centroid[a]);
            boundsMax = std::max(boundsMax, tri.centroid[a]);
        }

        // Populate the bins
        const size_t BINS = 8;
        Bin bin[BINS];
        double scale = BINS / (boundsMax - boundsMin);
        for (size_t i = 0; i < node.triCount; i++) {
            size_t triID = triIdx[node.leftFirst + i];
            const vira::geometry::Triangle<TSpectral, double>& tri = this->mesh->getTriangle(triID);
            size_t binIdx = std::min(BINS - 1, static_cast<size_t>((tri.centroid[a] - boundsMin) * scale));
            bin[binIdx].triCount++;
            bin[binIdx].bounds.grow(tri.vert[0].position);
            bin[binIdx].bounds.grow(tri.vert[1].position);
            bin[binIdx].bounds.grow(tri.vert[2].position);
        }

        // Gather data for the planes between the bins
        double leftArea[BINS - 1];
        double rightArea[BINS - 1];
        size_t leftCount[BINS - 1];
        size_t rightCount[BINS - 1];
        AABB<TSpectral, double> leftBox, rightBox;
        size_t leftSum = 0, rightSum = 0;
        for (size_t i = 0; i < BINS - 1; i++) {
            leftSum += bin[i].triCount;
            leftCount[i] = leftSum;
            leftBox.grow(bin[i].bounds);
            leftArea[i] = leftBox.area();
            rightSum += bin[BINS - 1 - i].triCount;
            rightCount[BINS - 2 - i] = rightSum;
            rightBox.grow(bin[BINS - 1 - i].bounds);
            rightArea[BINS - 2 - i] = rightBox.area();
        }

        // Calculate SAH cost for the planes
        scale = (boundsMax - boundsMin) / BINS;
        for (size_t i = 0; i < BINS - 1; i++) {
            double planeCost = static_cast<double>(leftCount[i]) * leftArea[i] + static_cast<double>(rightCount[i]) * rightArea[i];
            if (planeCost < bestCost) {
                axis = a;
                splitPos = boundsMin + scale * static_cast<double>(i + 1);
                bestCost = planeCost;
            }
        }

        return bestCost;
    };

    template <IsSpectral TSpectral>
    double ViraBLAS<TSpectral>::evaluateSAH(ViraBLASNode<TSpectral>& node, int axis, double pos)
    {
        // determine triangle counts and bounds for this split candidate
        AABB<TSpectral, double> leftBox, rightBox;
        size_t leftCount = 0, rightCount = 0;
        for (size_t i = 0; i < node.triCount; i++) {
            const vira::geometry::Triangle<TSpectral, double>& tri = this->mesh->getTriangle(triIdx[node.leftFirst + i]);
            if (tri.centroid[axis] < pos) {
                leftCount++;
                leftBox.grow(tri.vert[0].position);
                leftBox.grow(tri.vert[1].position);
                leftBox.grow(tri.vert[2].position);
            }
            else {
                rightCount++;
                rightBox.grow(tri.vert[0].position);
                rightBox.grow(tri.vert[1].position);
                rightBox.grow(tri.vert[2].position);
            }
        }
        double cost = leftCount * leftBox.area() + rightCount * rightBox.area();
        if (cost > 0) {
            return cost;
        }
        else {
            return std::numeric_limits<double>::infinity();
        }
    };

    template <IsSpectral TSpectral>
    double ViraBLAS<TSpectral>::calculateNodeCost(ViraBLASNode<TSpectral>& node)
    {
        vec3<double> e = node.aabb.max() - node.aabb.min();
        double surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
        return static_cast<double>(node.triCount) * surfaceArea;
    };
};
