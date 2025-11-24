#include <cstdint>
#include <vector>
#include <memory>
#include <fstream>
#include <limits>
#include <cmath>

#include "embree3/rtcore.h"

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/materials/material.hpp"
#include "vira/materials/lambertian.hpp"
#include "vira/constraints.hpp"
#include "vira/quipu/dem_quipu.hpp"
#include "vira/rendering/acceleration/vira_blas.hpp"
#include "vira/rendering/acceleration/embree_blas.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/scene.hpp"

namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Mesh<TSpectral, TFloat, TMeshFloat>::Mesh(VertexBuffer<TSpectral, TMeshFloat> newVertexBuffer, IndexBuffer newIndexBuffer)
        : vertexBuffer{ newVertexBuffer }, indexBuffer{ newIndexBuffer }
    {
        init();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Mesh<TSpectral, TFloat, TMeshFloat>::Mesh(VertexBuffer<TSpectral, TMeshFloat> newVertexBuffer, IndexBuffer newIndexBuffer, std::vector<MaterialID::ValueType> materialIndices)
        : vertexBuffer{ newVertexBuffer }, indexBuffer{ newIndexBuffer }, materialCacheIndices{ materialIndices }
    {

        if (!materialCacheIndices.empty()) {
            // Find the maximum index to determine cache size
            auto maxElement = std::max_element(materialCacheIndices.begin(), materialCacheIndices.end());
            size_t maxIndex = static_cast<size_t>(*maxElement);
            material_cache_.resize(maxIndex + 1, nullptr);
            materialIDs_.resize(maxIndex + 1, MaterialID{});
        }

        init();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Mesh<TSpectral, TFloat, TMeshFloat>::Mesh(vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat> newQuipu) :
        quipu{ newQuipu }
    {
        this->quipu.readBuffers(this->vertexBuffer, this->indexBuffer);
        hasQuipu_ = true;
        init();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::init()
    {
        numTriangles = static_cast<size_t>(indexBuffer.size() / 3);

        // TODO Is this a good check for if normal vectors have been set?
        if (glm::length(vertexBuffer[0].normal) == 0) {
            calculateNormals();
        }

        if (materialCacheIndices.empty()) {
            materialCacheIndices = std::vector<MaterialID::ValueType>(numTriangles, 0);
            this->material_cache_.clear();
            this->material_cache_.push_back(nullptr);

            this->materialIDs_.clear();
            this->materialIDs_.push_back(vira::MaterialID{});
        }

        // TODO Consider removing this at somepoint! (as embree duplicates this)
        // Make it optional, or cache only when requested.  Something along those lines.
        constructTriangles();

        // Inform the Scene that things have changed
        if (scene_ != nullptr) {
            scene_->markDirty();
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::updateGSD(float targetGSD)
    {
        // Update if needed:
        if (hasQuipu_) {
            if (std::isinf(targetGSD)) {
                targetGSD = quipu.getDefaultGSD();
            }

            if (targetGSD == quipu.getCurrentGSD()) {
                return;
            }

            // Save materials to re-apply:
            std::vector<vira::materials::Material<TSpectral>*> material_cache_SAVE = material_cache_;
            std::vector<vira::MaterialID> materialIDs_SAVE = materialIDs_;
            vira::materials::Material<TSpectral>* default_material_cache_SAVE = default_material_cache_;

            materialCacheIndices.clear();

            quipu.readBuffers(this->vertexBuffer, this->indexBuffer, targetGSD);
            modified = true;
            init();

            // Reapply materials:
            material_cache_ = material_cache_SAVE;
            materialIDs_ = materialIDs_SAVE;
            default_material_cache_ = default_material_cache_SAVE;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float Mesh<TSpectral, TFloat, TMeshFloat>::getDefaultGSD()
    {
        if (hasQuipu_) {
            return quipu.getDefaultGSD();
        }
        else {
            return 0.f;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float Mesh<TSpectral, TFloat, TMeshFloat>::getGSD()
    {
        if (hasQuipu_) {
            return quipu.getCurrentGSD();
        }
        else {
            return 0.f;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    Normal Mesh<TSpectral, TFloat, TMeshFloat>::getNormal()
    {
        if (hasQuipu_) {
            return quipu.getNormal();
        }
        else {
            return Normal{0, 0, 0};
        }
    };
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    float Mesh<TSpectral, TFloat, TMeshFloat>::getConeAngle()
    {
        if (hasQuipu_) {
            return quipu.getConeAngle();
        }
        else {
            return 0;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::update()
    {
        modified = true;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::setSmoothShading(bool newSmoothShading)
    {
        this->smoothShading = newSmoothShading;

        // Update pre-constructed triangles:
        for (auto& tri : triangles) {
            tri.smooth_shading = newSmoothShading;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::constructTriangles()
    {
        if (modified) {
            size_t triID = 0;
            triangles = std::vector<Triangle<TSpectral, TMeshFloat>>(numTriangles);
            for (size_t i = 0; i < numTriangles; i++) {
                auto id0 = indexBuffer[3 * i + 0];
                auto id1 = indexBuffer[3 * i + 1];
                auto id2 = indexBuffer[3 * i + 2];

                auto v0 = vertexBuffer[id0];
                auto v1 = vertexBuffer[id1];
                auto v2 = vertexBuffer[id2];

                Triangle<TSpectral, TMeshFloat> tri(v0, v1, v2, smoothShading, materialCacheIndices[i]);
                triangles[triID] = tri;
                triID++;
            }
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::calculateNormals()
    {
        // Calculate face normals:
        for (size_t i = 0; i < indexBuffer.size(); i = i + 3) {
            size_t i0 = indexBuffer[i + 0];
            size_t i1 = indexBuffer[i + 1];
            size_t i2 = indexBuffer[i + 2];

            auto& p0 = vertexBuffer[i0].position;
            auto& p1 = vertexBuffer[i1].position;
            auto& p2 = vertexBuffer[i2].position;

            if (!std::isinf(p0[0]) && !std::isinf(p1[0]) && !std::isinf(p2[0])) {
                vec3<float> e01 = p1 - p0;
                vec3<float> e02 = p2 - p0;
                Normal faceNormal = glm::normalize(glm::cross(e01, e02));

                // TODO: Revisit this... as this is not a solution
                if (!std::isnan(faceNormal[0]) &&
                    !std::isnan(faceNormal[1]) &&
                    !std::isnan(faceNormal[2])) {
                    vertexBuffer[i0].normal = vertexBuffer[i0].normal + faceNormal;
                    vertexBuffer[i1].normal = vertexBuffer[i1].normal + faceNormal;
                    vertexBuffer[i2].normal = vertexBuffer[i2].normal + faceNormal;
                }
            }
        }

        // Normalize vertex normals:
        for (size_t i = 0; i < vertexBuffer.size(); i++) {
            if (glm::length(vertexBuffer[i].normal) != 0) {
                vertexBuffer[i].normal = glm::normalize(vertexBuffer[i].normal);
            }
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<TMeshFloat> Mesh<TSpectral, TFloat, TMeshFloat>::calculateCenter()
    {
        vec3<TMeshFloat> center{0, 0, 0};
        TMeshFloat valid = 0;
        for (auto& vertex : vertexBuffer) {
            if (!std::isinf(vertex.position[0]) && !std::isinf(vertex.position[1]) && !std::isinf(vertex.position[2])) {
                center += vertex.position;
                valid += 1;
            }
        }
        center /= valid;
        return center;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::applyTransformState(const mat4<TMeshFloat>& transformation)
    {
        Rotation<TMeshFloat> rotation = ReferenceFrame<TMeshFloat>::getRotationFromTransformation(transformation);

        for (auto& vertex : vertexBuffer) {
            vec3<TMeshFloat> v = vertex.position;
            if (std::isinf(v[0]) || std::isinf(v[1]) || std::isinf(v[2])) {
                vertex.position = vec3<TMeshFloat>{ std::numeric_limits<TMeshFloat>::infinity() };
            }
            else {
                vertex.position = vec3<TMeshFloat>(transformation * vec4<TMeshFloat>(v, 1));
            }

            vec3<float> n = vertex.normal;
            if (std::isinf(n[0]) || std::isinf(n[1]) || std::isinf(n[2])) {
                vertex.normal = vec3<float>{ std::numeric_limits<float>::infinity() };
            }
            else {
                vertex.normal = rotation * n;
            }
        }

        update();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::applyPosition(const vec3<TMeshFloat>& position)
    {
        for (auto& vertex : vertexBuffer) {
            vertex.position = vertex.position + position;
        }

        update();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::applyRotation(const mat3<TMeshFloat>& rotation)
    {
        mat3<TMeshFloat> rotationTranspose = glm::transpose(rotation);

        for (auto& vertex : vertexBuffer) {
            auto v = vertex.position;
            if (std::isinf(v[0]) || std::isinf(v[1]) || std::isinf(v[2])) {
                vertex.position = vec3<TMeshFloat>{ std::numeric_limits<TMeshFloat>::infinity() };
            }
            else {
                vertex.position = rotationTranspose * v;
            }

            auto n = vertex.normal;
            if (std::isinf(n[0]) || std::isinf(n[1]) || std::isinf(n[2])) {
                vertex.normal = vec3<TMeshFloat>{ std::numeric_limits<TMeshFloat>::infinity() };
            }
            else {
                vertex.normal = rotationTranspose * n;
            }
        }

        update();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::applyScale(const vec3<TMeshFloat>& scale)
    {
        for (auto& vertex : vertexBuffer) {
            auto v = vertex.position;
            if (std::isinf(v[0]) || std::isinf(v[1]) || std::isinf(v[2])) {
                vertex.position = vec3<TMeshFloat>{ std::numeric_limits<TMeshFloat>::infinity() };
            }
            else {
                vertex.position = scale * v;
            }
        }

        update();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::applyScale(TMeshFloat scale)
    {
        this->applyScale(vec3<TMeshFloat>{scale});
    };
    
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<TSpectral> Mesh<TSpectral, TFloat, TMeshFloat>::getAlbedos() const {
        std::vector<TSpectral> albedoData;
        albedoData.reserve(vertexBuffer.size()); // Preallocate space for efficiency

        std::transform(vertexBuffer.begin(), vertexBuffer.end(), std::back_inserter(albedoData),
            [](const Vertex<TSpectral, TMeshFloat>& v) { return v.albedo; });

        return albedoData;
    }

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::setMaterial(size_t index, MaterialID newMaterialID)
    {
        if (this->scene_ == nullptr) {
            throw std::runtime_error("Cannot assign Material to a Mesh that is not part of a Scene");
        }
        this->materialIDs_[index] = newMaterialID;
        material_cache_[index] = &(*this->scene_)[newMaterialID];
        
        // TODO Is this necessary?
        scene_->markDirty();
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    bool Mesh<TSpectral, TFloat, TMeshFloat>::removeMaterial(MaterialID removeMaterialID)
    {
        bool materialFound = false;
        vira::MaterialID defaultMaterialID = default_material_cache_->getID();

        size_t removedCacheIndex = std::numeric_limits<size_t>::max();

        // Find which cache entry contains the material to be removed
        for (size_t cacheIdx = 0; cacheIdx < material_cache_.size(); ++cacheIdx) {
            if (material_cache_[cacheIdx] && material_cache_[cacheIdx]->getID() == removeMaterialID) {
                removedCacheIndex = cacheIdx;
                break;
            }
        }

        // Step 2: If found, update the cache entry and materialIDs
        if (removedCacheIndex != std::numeric_limits<size_t>::max()) {
            // Update the cache entry to point to default material
            material_cache_[removedCacheIndex] = default_material_cache_;

            // Update materialIDs vector
            for (size_t i = 0; i < materialIDs_.size(); ++i) {
                if (materialIDs_[i] == removeMaterialID) {
                    materialIDs_[i] = defaultMaterialID;
                    materialFound = true;
                }
            }
        }

        if (materialFound) {
            scene_->markDirty();
        }

        return materialFound;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::buildBVH(RTCDevice device, vira::rendering::BVHBuildOptions bvhBuildOptions)
    {
        if constexpr (std::same_as<TMeshFloat, float>) {
            if (this->modified) {
                this->bvh = std::make_unique<vira::rendering::EmbreeBLAS<TSpectral, TFloat>>(device, this, bvhBuildOptions);
                this->aabb = this->bvh->getAABB();
                this->modified = false;
            }
        }
        else {
            throw std::runtime_error("An unxpected error occured, as Embree was attempted to use with double precision.  Please report!");
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::buildBVH(vira::rendering::BVHBuildOptions bvhBuildOptions)
    {
        if (this->modified) {
            this->bvh = std::make_unique<vira::rendering::ViraBLAS<TSpectral>>(this, bvhBuildOptions);
            this->aabb = this->bvh->getAABB();
            this->modified = false;
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::rendering::AABB<TSpectral, TFloat> Mesh<TSpectral, TFloat, TMeshFloat>::getAABB()
    {
        if (modified) {
            // Re-compute AABB
            for (auto& vert : vertexBuffer) {
                aabb.grow(vert.position);
            }
        }
        return aabb;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void Mesh<TSpectral, TFloat, TMeshFloat>::deviceFreed()
    {
        if (bvh != nullptr) { 
            this->bvh->device_freed_ = true; 
        }
    };
};