#include <cstddef>
#include <limits>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"

namespace vira::rendering {
    template <IsSpectral TSpectral, IsFloat TFloat>
    void RenderPasses<TSpectral, TFloat>::resetImages()
    {
        // ======================= //
        // === Required Fields === //
        // ======================= //
        depth.clear();
        alpha.clear();
        albedo.clear();
        normal_global.clear();

        instance_id.clear();
        mesh_id.clear();
        triangle_id.clear();
        material_id.clear();


        // ======================= //
        // === Optional Fields === //
        // ======================= //
        received_power.clear();
        total_radiance.clear();
        direct_radiance.clear();
        indirect_radiance.clear();

        normal_camera.clear();

        velocity_global.clear();
        velocity_camera.clear();
        
        triangle_size.clear();
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    void RenderPasses<TSpectral, TFloat>::initializeImages(vira::images::Resolution resolution)
    {
        // ======================= //
        // === Required Fields === //
        // ======================= //
        this->depth = vira::images::Image<float>(resolution, std::numeric_limits<float>::infinity());
        this->alpha = vira::images::Image<float>(resolution, 0.f);
        this->albedo = vira::images::Image<TSpectral>(resolution, TSpectral{ 0 });
        this->normal_global = vira::images::Image<vec3<float>>(resolution, vec3<float>(0));
        this->normal_camera = vira::images::Image<vec3<float>>(resolution, vec3<float>(0));

        this->instance_id = vira::images::Image<size_t>(resolution, std::numeric_limits<size_t>::max());
        this->mesh_id = vira::images::Image<size_t>(resolution, std::numeric_limits<size_t>::max());
        this->triangle_id = vira::images::Image<size_t>(resolution, std::numeric_limits<size_t>::max());
        this->material_id = vira::images::Image<size_t>(resolution, std::numeric_limits<size_t>::max());


        // ======================= //
        // === Optional Fields === //
        // ======================= //
        if (simulate_lighting) {
            this->received_power = vira::images::Image<TSpectral>(resolution, TSpectral{ 0 });
            this->total_radiance = vira::images::Image<TSpectral>(resolution, TSpectral{ 0 });
            this->direct_radiance = vira::images::Image<TSpectral>(resolution, TSpectral{ 0 });
            this->indirect_radiance = vira::images::Image<TSpectral>(resolution, TSpectral{ 0 });
        }

        if (save_velocity) {
            this->velocity_global = vira::images::Image<vec3<float>>(resolution, vec3<float>(0));
            this->velocity_camera = vira::images::Image<vec3<float>>(resolution, vec3<float>(0));
        }


        if (save_triangle_size) {
            this->triangle_size = vira::images::Image<float>(resolution, std::numeric_limits<float>::infinity());
        }
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    void RenderPasses<TSpectral, TFloat>::updateImages(const DataPayload<TSpectral, TFloat>& dataPayload)
    {
        const int i = dataPayload.i;
        const int j = dataPayload.j;


        // ======================= //
        // === Required Fields === //
        // ======================= //
        if (dataPayload.count == 0) {
            this->depth(i, j) = std::numeric_limits<float>::infinity();
        }
        else {
            this->depth(i, j) = static_cast<float>(dataPayload.depth) / static_cast<float>(dataPayload.count);
        }

        this->alpha(i, j) = static_cast<float>(dataPayload.count) / static_cast<float>(dataPayload.sample);
        this->albedo(i, j) = dataPayload.albedo;

        this->normal_global(i, j) = dataPayload.normal_global;
        this->normal_camera(i, j) = dataPayload.normal_camera;

        this->instance_id(i, j) = dataPayload.instance_id;
        this->mesh_id(i, j) = dataPayload.mesh_id;
        this->triangle_id(i, j) = dataPayload.triangle_id;
        this->material_id(i, j) = dataPayload.material_id;



        // ======================= //
        // === Optional Fields === //
        // ======================= //
        if (simulate_lighting) {
            this->total_radiance(i, j) = dataPayload.total_radiance;
            this->direct_radiance(i, j) = dataPayload.direct_radiance;
            this->indirect_radiance(i, j) = dataPayload.indirect_radiance;
        }

        if (save_velocity) {
            this->velocity_global(i, j) = dataPayload.velocity_global;
            this->velocity_camera(i, j) = dataPayload.velocity_camera;
        }

        if (save_triangle_size) {
            this->triangle_size(i, j) = dataPayload.triangle_size;
        }
    };
};