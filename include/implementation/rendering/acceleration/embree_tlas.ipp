#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>
#include <array>

#include "embree3/rtcore.h"

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/rendering/acceleration/nodes.hpp"
#include "vira/rendering/ray.hpp"
#include "vira/rendering/acceleration/aabb.hpp"
#include "vira/rendering/acceleration/embree_blas.hpp"
#include "vira/rendering/acceleration/embree_options.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral>
    EmbreeTLAS<TSpectral>::EmbreeTLAS(RTCDevice device, EmbreeOptions embree_options)
    {
        global_scene_ = rtcNewScene(device);

        if (embree_options.robust_build) {
            rtcSetSceneFlags(this->global_scene_, RTC_SCENE_FLAG_ROBUST);
        }

        RTCBuildQuality build_quality = RTC_BUILD_QUALITY_HIGH;
        if (embree_options.build_quality == BUILD_QUALITY_LOW) {
            build_quality = RTC_BUILD_QUALITY_LOW;
        }
        else if (embree_options.build_quality == BUILD_QUALITY_MEDIUM) {
            build_quality = RTC_BUILD_QUALITY_MEDIUM;
        }
        rtcSetSceneBuildQuality(global_scene_, build_quality);
    };

    template <IsSpectral TSpectral>
    EmbreeTLAS<TSpectral>::~EmbreeTLAS()
    {
        if (!this->device_freed_) {
            rtcReleaseScene(global_scene_);
        }
    };

    template <IsSpectral TSpectral>
    EmbreeTLAS<TSpectral>::EmbreeTLAS(EmbreeTLAS&& other) noexcept
        : TLAS<TSpectral, float, float>(std::move(other))
        , global_scene_(other.global_scene_)
        , instance_array_(std::move(other.instance_array_))
    {
        // Invalidate the moved-from object so its destructor won't release the scene
        other.global_scene_ = nullptr;
    }

    template <IsSpectral TSpectral>
    EmbreeTLAS<TSpectral>& EmbreeTLAS<TSpectral>::operator=(EmbreeTLAS&& other) noexcept
    {
        if (this != &other) {
            // Clean up our current resources
            if (global_scene_ && !this->device_freed_) {
                rtcReleaseScene(global_scene_);
            }

            // Move from other
            TLAS<TSpectral, float, float>::operator=(std::move(other));
            global_scene_ = other.global_scene_;
            instance_array_ = std::move(other.instance_array_);

            // Invalidate the moved-from object
            other.global_scene_ = nullptr;
        }
        return *this;
    }

    template <IsSpectral TSpectral>
    void EmbreeTLAS<TSpectral>::newInstance(RTCDevice device, EmbreeBLAS<TSpectral, float>* blas, vira::geometry::Mesh<TSpectral, float, float>* mesh, vira::scene::Instance<TSpectral, float, float>* instance)
    {
        RTCGeometry rtc_instance = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_INSTANCE);

        rtcSetGeometryInstancedScene(rtc_instance, blas->scene);
        rtcSetGeometryTimeStepCount(rtc_instance, 1);

        // Assign the transformation to the RTC instance:
        mat4<float> modelMatrix = instance->getModelMatrix();
        float transform[16];
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                transform[i * 4 + j] = modelMatrix[i][j];
            }
        }
        rtcSetGeometryTransform(rtc_instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, transform);

        // Commit and attach
        rtcCommitGeometry(rtc_instance);
        unsigned int rtc_instance_id = rtcAttachGeometry(global_scene_, rtc_instance);

        // Resize vector if needed for direct indexing
        if (rtc_instance_id >= instance_array_.size()) {
            instance_array_.resize(rtc_instance_id + 1);
        }

        InstanceMeshData<TSpectral> mesh_data;
        mesh_data.mesh = mesh;
        mesh_data.instance = instance;
        instance_array_[rtc_instance_id] = mesh_data;

        rtcReleaseGeometry(rtc_instance);
    };

    template <IsSpectral TSpectral>
    void EmbreeTLAS<TSpectral>::build()
    {
        rtcCommitScene(this->global_scene_);
    };

    template <IsSpectral TSpectral>
    thread_local RTCIntersectContext EmbreeTLAS<TSpectral>::context_;

    template <IsSpectral TSpectral>
    void EmbreeTLAS<TSpectral>::init()
    {
        // Initialize the intersector:
        rtcInitIntersectContext(&context_);
    };

    template <IsSpectral TSpectral>
    void EmbreeTLAS<TSpectral>::intersect(Ray<TSpectral, float>& ray)
    {
        RTCRayHit embreeRay;
        embreeRay.ray.tnear = 0.f;
        embreeRay.ray.tfar = std::numeric_limits<float>::infinity();
        embreeRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        embreeRay.ray.org_x = ray.origin[0];
        embreeRay.ray.org_y = ray.origin[1];
        embreeRay.ray.org_z = ray.origin[2];
        embreeRay.ray.dir_x = ray.direction[0];
        embreeRay.ray.dir_y = ray.direction[1];
        embreeRay.ray.dir_z = ray.direction[2];

        // Perform intersection:
        rtcIntersect1(global_scene_, &context_, &embreeRay);

        if (embreeRay.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            ray.hit.tri_id = embreeRay.hit.primID;
            unsigned int rtcInstanceID = embreeRay.hit.instID[0];

            const auto& mesh_data = instance_array_[rtcInstanceID];
            auto mesh = mesh_data.mesh;
            auto instance = mesh_data.instance;

            ray.hit.t = embreeRay.ray.tfar;

            const vira::geometry::Triangle<TSpectral, float>& tri = mesh->getTriangle(ray.hit.tri_id);
            ray.hit.face_normal = tri.face_normal;

            // TODO Why is this winding order offet?
            // Are w[3] values being calculated incorrectly?
            // This is same for Triangle::intersect, but not for Rasterizer...
            ray.hit.vert[0] = tri.vert[1];
            ray.hit.vert[1] = tri.vert[2];
            ray.hit.vert[2] = tri.vert[0];

            ray.hit.w[0] = embreeRay.hit.u;
            ray.hit.w[1] = embreeRay.hit.v;
            ray.hit.w[2] = 1.f - embreeRay.hit.u - embreeRay.hit.v;

            ray.hit.material_cache_index = tri.material_cache_index;

            ray.hit.mesh_ptr = mesh;
            ray.hit.instance_ptr = instance;
        }
    };
};