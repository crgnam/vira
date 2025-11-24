#include <cstdint>
#include <limits>

#include "embree3/rtcore.h"

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/triangle.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/rendering/acceleration/embree_options.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat>
    EmbreeBLAS<TSpectral, TFloat>::EmbreeBLAS(RTCDevice device, vira::geometry::Mesh<TSpectral, TFloat, float>* newMesh, BVHBuildOptions buildOptions)
    {
        (void)buildOptions;

        // Initialize Embree data structures:
        scene = rtcNewScene(device);
        geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

        this->mesh = newMesh;
        this->mesh_ptr = static_cast<void*>(this->mesh); // Store type-erased pointer to Mesh

        auto vertexBuffer = this->mesh->getVertexBuffer();
        auto indexBuffer = this->mesh->getIndexBuffer();

        // Create shared geometry buffer:
        unsigned int slotVert = 0;
        const void* vPtr = vertexBuffer.data();
        size_t byteOffsetVert = 0;
        size_t byteStrideVert = sizeof(vira::geometry::Vertex<TSpectral, float>);
        size_t itemCountVert = vertexBuffer.size();
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, slotVert, RTC_FORMAT_FLOAT3, vPtr, byteOffsetVert, byteStrideVert, itemCountVert);

        unsigned int slotInd = 0;
        const void* iPtr = indexBuffer.data();
        size_t byteOffsetInd = 0;
        size_t byteStrideInd = 3 * sizeof(uint32_t);
        size_t itemCountInd = indexBuffer.size() / 3;
        rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, slotInd, RTC_FORMAT_UINT3, iPtr, byteOffsetInd, byteStrideInd, itemCountInd);



        // Set BVH Build Settings:
        if (buildOptions.embree_options.robust_build) {
            rtcSetSceneFlags(this->scene, RTC_SCENE_FLAG_ROBUST);
        }

        RTCBuildQuality build_quality = RTC_BUILD_QUALITY_HIGH;
        if (buildOptions.embree_options.build_quality == BUILD_QUALITY_LOW) {
            build_quality = RTC_BUILD_QUALITY_LOW;
        }
        else if (buildOptions.embree_options.build_quality == BUILD_QUALITY_MEDIUM) {
            build_quality = RTC_BUILD_QUALITY_MEDIUM;
        }
        rtcSetSceneBuildQuality(this->scene, build_quality);



        // Attach geometry:
        rtcCommitGeometry(geom);
        rtcAttachGeometry(scene, geom);
        rtcReleaseGeometry(geom);

        // Trigger Embree BVH Builder:
        rtcCommitScene(scene);

        // Initialize the intersector:
        rtcInitIntersectContext(&context);
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    EmbreeBLAS<TSpectral, TFloat>::~EmbreeBLAS()
    {
        if (!this->device_freed_) {
            rtcReleaseScene(scene);
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    void EmbreeBLAS<TSpectral, TFloat>::intersect(Ray<TSpectral, float>& ray)
    {
        RTCRayHit embreeRay;
        embreeRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        embreeRay.ray.tnear = 0.f;
        embreeRay.ray.tfar = std::numeric_limits<float>::infinity();
        embreeRay.ray.org_x = ray.origin[0];
        embreeRay.ray.org_y = ray.origin[1];
        embreeRay.ray.org_z = ray.origin[2];
        embreeRay.ray.dir_x = ray.direction[0];
        embreeRay.ray.dir_y = ray.direction[1];
        embreeRay.ray.dir_z = ray.direction[2];

        // Perform intersection:
        rtcIntersect1(scene, &context, &embreeRay);

        if (embreeRay.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            ray.hit.tri_id = embreeRay.hit.primID;
            const vira::geometry::Triangle<TSpectral, float>& tri = this->mesh->getTriangle(ray.hit.tri_id);

            // TODO Why is this winding order offet?
            // Are w[3] values being calculated incorrectly?
            // This is same for Triangle::intersect, but not for Rasterizer...
            ray.hit.vert[0] = tri.vert[1];
            ray.hit.vert[1] = tri.vert[2];
            ray.hit.vert[2] = tri.vert[0];

            ray.hit.face_normal = tri.face_normal;
            ray.hit.w[0] = embreeRay.hit.u;
            ray.hit.w[1] = embreeRay.hit.v;
            ray.hit.w[2] = 1.f - embreeRay.hit.u - embreeRay.hit.v;
            ray.hit.t = embreeRay.ray.tfar;
            ray.hit.material_cache_index = tri.material_cache_index;

            ray.hit.mesh_ptr = this->mesh_ptr; // TODO Remove this?
        }
    };

    
    template <IsSpectral TSpectral, IsFloat TFloat>
    template <size_t N> requires ValidPacketSize<N>
    void EmbreeBLAS<TSpectral, TFloat>::intersectPacket(std::array<Ray<TSpectral, float>, N>& rayPacket)
    {
        typename RTCRayHitSelector<N>::RayHitType embreeRays;

        alignas(4 * N) int valid[N];
        for (size_t i = 0; i < N; ++i) {
            //size_t validIndex = (N - 1) - i;
            size_t validIndex = i;
            if (std::isinf(rayPacket[i].origin.x)) {
                valid[validIndex] = 0;
                embreeRays.hit.geomID[i] = RTC_INVALID_GEOMETRY_ID;
            }
            else {
                valid[validIndex] = -1; // -1 is valid, 0 is invalid.  Initialize all to valid

                embreeRays.hit.geomID[i] = RTC_INVALID_GEOMETRY_ID;
                embreeRays.ray.tfar[i] = std::numeric_limits<float>::infinity();
                embreeRays.ray.tnear[i] = 0.f;
                embreeRays.ray.org_x[i] = rayPacket[i].origin.x;
                embreeRays.ray.org_y[i] = rayPacket[i].origin.y;
                embreeRays.ray.org_z[i] = rayPacket[i].origin.z;
                embreeRays.ray.dir_x[i] = rayPacket[i].direction.x;
                embreeRays.ray.dir_y[i] = rayPacket[i].direction.y;
                embreeRays.ray.dir_z[i] = rayPacket[i].direction.z;
            }
        }

        RTCRayHitSelector<N>::intersect(valid, scene, &context, &embreeRays);

        for (size_t i = 0; i < N; ++i) {
            if (embreeRays.hit.geomID[i] != RTC_INVALID_GEOMETRY_ID) {
                rayPacket[i].hit.triID = embreeRays.hit.primID[i];
                const vira::geometry::Triangle<TSpectral, float>& tri = this->mesh->getTriangle(rayPacket[i].hit.triID);

                // TODO Why is this winding order offet?
                // Are w[3] values being calculated incorrectly?
                // This is same for Triangle::intersect, but not for Rasterizer...
                rayPacket[i].hit.vert[0] = tri.vert[1];
                rayPacket[i].hit.vert[1] = tri.vert[2];
                rayPacket[i].hit.vert[2] = tri.vert[0];

                rayPacket[i].hit.faceNormal = tri.faceNormal;
                rayPacket[i].hit.w[0] = embreeRays.hit.u[i];
                rayPacket[i].hit.w[1] = embreeRays.hit.v[i];
                rayPacket[i].hit.w[2] = 1.f - embreeRays.hit.u[i] - embreeRays.hit.v[i];
                rayPacket[i].hit.smoothShading = this->mesh->getSmoothShading();
                rayPacket[i].hit.t = embreeRays.ray.tfar[i];
                rayPacket[i].hit.materialID = tri.materialID;

                rayPacket[i].hit.mesh_ptr = this->mesh_ptr;
            }
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    AABB<TSpectral, TFloat> EmbreeBLAS<TSpectral, TFloat>::getAABB()
    {
        
        RTCBounds bounds;
        rtcGetSceneBounds(scene, &bounds);
        
        vec3<TFloat> bmin{ bounds.lower_x, bounds.lower_y, bounds.lower_z };
        vec3<TFloat> bmax{ bounds.upper_x, bounds.upper_y, bounds.upper_z };
        return AABB<TSpectral, TFloat>(bmin, bmax);
    };
};