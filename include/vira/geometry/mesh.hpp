#ifndef VIRA_GEOMETRY_MESH_HPP
#define VIRA_GEOMETRY_MESH_HPP

#include <memory>
#include <vector>
#include <cstdint>
#include <limits>
#include <algorithm> 

#include "embree3/rtcore.h"

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/materials/material.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/blas.hpp"
#include "vira/quipu/dem_quipu.hpp"
#include "vira/scene/ids.hpp"

// Forward Declaration:
namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
}

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Group;
}

namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class GeometryInterface;
}

namespace vira::geometry {
    /**
     * @brief Represents a 3D mesh with vertices, indices, materials, and rendering optimizations.
     *
     * @tparam TSpectral The spectral type used for color representation (must satisfy IsSpectral concept)
     * @tparam TFloat The floating-point type used for general calculations (must satisfy IsFloat concept)
     * @tparam TMeshFloat The floating-point type used for mesh geometry (must satisfy IsFloat concept and have greater precision than TFloat)
     *
     * The Mesh class encapsulates all data and operations for a 3D geometric object in the spectral
     * rendering pipeline. It manages vertex buffers, index buffers, materials, and provides various
     * geometric operations and rendering optimizations.
     *
     * Key features:
     * - Multiple construction methods supporting standard geometry, material-mapped geometry, and DEM data
     * - Flexible material system with per-face material assignment and runtime material management
     * - Geometric operations including transformations, normal calculation, and center computation
     * - Adaptive level-of-detail through Ground Sample Distance (GSD) control
     * - Bounding Volume Hierarchy (BVH) acceleration structure support for ray tracing
     * - Triangle mesh representation with smooth/flat shading options
     * - Integration with DEM Quipu system for terrain data
     * - Axis-Aligned Bounding Box (AABB) computation for spatial queries
     *
     * The class supports both CPU and GPU rendering pipelines through friend relationships with
     * various renderer classes. Materials can be assigned per-face, allowing for complex multi-material
     * meshes while maintaining efficient memory layout.
     *
     * Precision management is enforced through template constraints, ensuring mesh geometry uses
     * higher precision floating-point than general scene calculations to maintain geometric accuracy
     * during transformations and computations.
     *
     * @note The class is non-copyable to prevent expensive deep copies of large mesh data.
     * @note BVH construction requires either an RTCDevice for hardware acceleration or can fall back to CPU implementation.
     *
     * @see VertexBuffer, IndexBuffer, Triangle, vira::materials::Material
     * @see vira::rendering::BLAS, vira::rendering::AABB
     * @see vira::quipu::DEMQuipu
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Mesh {
    public:
        Mesh(VertexBuffer<TSpectral, TMeshFloat> vertexBuffer, IndexBuffer indexBuffer);
        Mesh(VertexBuffer<TSpectral, TMeshFloat> vertexBuffer, IndexBuffer indexBuffer, std::vector<MaterialID::ValueType> materialIndices);
        Mesh(vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu);

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        void updateGSD(float targetGSD);
        float getDefaultGSD();
        float getGSD();
        Normal getNormal();
        float getConeAngle();

        void setSmoothShading(bool smoothShading);
        bool getSmoothShading() const { return smoothShading; }

        void constructTriangles();
        void calculateNormals();
        vec3<TMeshFloat> calculateCenter();

        void applyTransformState(const mat4<TMeshFloat>& transformation);
        void applyPosition(const vec3<TMeshFloat>& translation);
        void applyRotation(const mat3<TMeshFloat>& rotaiton);
        void applyScale(const vec3<TMeshFloat>& scale);
        void applyScale(TMeshFloat scale);

        const VertexBuffer<TSpectral, TMeshFloat>& getVertexBuffer() const { return vertexBuffer; }
        const IndexBuffer& getIndexBuffer() const { return indexBuffer; }

        std::vector<TSpectral> getAlbedos() const;

        void setMaterial(size_t index, MaterialID newMaterialID);
        bool removeMaterial(MaterialID removeMaterialID);
        const std::vector<MaterialID>& getMaterialIDs() const { return materialIDs_; }
        const std::vector<MaterialID::ValueType>& getMaterialIndices() const { return materialCacheIndices; }
        size_t getMaterialCount() const { return material_cache_.size(); }

        size_t getVertexCount() const { return static_cast<size_t>(vertexBuffer.size()); }
        size_t getIndexCount() const { return static_cast<size_t>(indexBuffer.size()); }
        size_t getNumTriangles() const { return numTriangles; }

        const std::vector<Triangle<TSpectral, TMeshFloat>>& getTriangles() const { return triangles; }
        const Triangle<TSpectral, TMeshFloat>& getTriangle(size_t index) const { return triangles[index]; }

        void buildBVH(RTCDevice device, vira::rendering::BVHBuildOptions bvhBuildOptions);
        void buildBVH(vira::rendering::BVHBuildOptions bvhBuildOptions);
        vira::rendering::AABB<TSpectral, TFloat> getAABB();

        bool hasQuipu() { return hasQuipu_; }
        fs::path getQuipuFilepath() { return quipu.getFilepath(); }

        const MeshID& getID() const { return id_; }

    private:
        MeshID id_{};

        VertexBuffer<TSpectral, TMeshFloat> vertexBuffer;
        IndexBuffer indexBuffer;

        bool hasQuipu_ = false;
        vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> quipu{};

        void deviceFreed();
        std::unique_ptr<vira::rendering::BLAS<TSpectral, TFloat, TMeshFloat>> bvh;

        bool smoothShading = false;
        bool modified = true;

        size_t numTriangles = 0;
        std::vector<Triangle<TSpectral, TMeshFloat>> triangles;

        vira::rendering::AABB<TSpectral, TFloat> aabb;

        std::vector<vira::MaterialID::ValueType> materialCacheIndices;
        std::vector<vira::materials::Material<TSpectral>*> material_cache_;
        std::vector<vira::MaterialID> materialIDs_;
        vira::materials::Material<TSpectral>* default_material_cache_ = nullptr;

        void init();
        void update();

        vira::Scene<TSpectral, TFloat, TMeshFloat>* scene_ = nullptr;
        void setScene(vira::Scene<TSpectral, TFloat, TMeshFloat>* scene) { scene_ = scene; }
        
        // Make Scene a friend-class:
        friend class vira::Scene<TSpectral, TFloat, TMeshFloat>;

        friend class vira::rendering::CPUPathTracer<TSpectral, TFloat, TMeshFloat>;
        friend class vira::rendering::CPURasterizer<TSpectral, TFloat, TMeshFloat>;

        friend class vira::geometry::GeometryInterface<TSpectral, TFloat, TMeshFloat>;
    };
};

#include "implementation/geometry/mesh.ipp"

#endif