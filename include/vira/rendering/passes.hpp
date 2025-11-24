#ifndef VIRA_RENDERING_PASSES_HPP
#define VIRA_RENDERING_PASSES_HPP

#include <cstddef>
#include <limits>

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/images/color_map.hpp"

namespace vira::rendering {
    // Data payload struct (used internally for rendering):
    template <IsSpectral TSpectral, IsFloat TFloat>
    struct DataPayload {
        DataPayload(int I, int J) : i{ I }, j{ J } {}

        int i = 0;
        int j = 0;
        size_t count = 0;

        TFloat depth = 0;

        TSpectral total_radiance = TSpectral{ 0 };
        TSpectral direct_radiance = TSpectral{ 0 };
        TSpectral indirect_radiance = TSpectral{ 0 };
        
        TSpectral albedo = TSpectral{ 0 };

        vec3<float> normal_global{ 0 };
        vec3<float> normal_camera{ 0 };

        vec3<TFloat> velocity_global{ 0 };
        vec3<TFloat> velocity_camera{ 0 };
        
        size_t instance_id = std::numeric_limits<size_t>::max();
        size_t mesh_id = std::numeric_limits<size_t>::max();
        size_t triangle_id = std::numeric_limits<size_t>::max();
        size_t material_id = std::numeric_limits<size_t>::max();
        
        float triangle_size = std::numeric_limits<float>::infinity();

        // Tracking of operations:
        size_t bounce = 0;
        size_t sample = 0;
        bool first_hit = true;
        TSpectral throughput;
    };


    // Render passes:
    template<IsSpectral TSpectral, IsFloat TFloat>
    struct RenderPasses {
        // ======================= //
        // === Required Fields === //
        // ======================= //
        vira::images::Image<float> depth;
        vira::images::Image<float> alpha;
        vira::images::Image<TSpectral> albedo;
        vira::images::Image<vec3<float>> normal_global;
        vira::images::Image<vec3<float>> normal_camera;

        vira::images::Image<size_t> instance_id;
        vira::images::Image<size_t> mesh_id;
        vira::images::Image<size_t> triangle_id;
        vira::images::Image<size_t> material_id;



        // ======================= //
        // === Optional Fields === //
        // ======================= //
        bool simulate_lighting = false;
        vira::images::Image<TSpectral> received_power;
        vira::images::Image<TSpectral> total_radiance;
        vira::images::Image<TSpectral> direct_radiance;
        vira::images::Image<TSpectral> indirect_radiance;


        bool save_velocity = false;
        vira::images::Image<vec3<float>> velocity_global;
        vira::images::Image<vec3<float>> velocity_camera;


        bool save_triangle_size = false;
        vira::images::Image<float> triangle_size;



        // ====================== //
        // === Update Methods === //
        // ====================== //
        void resetImages();
        void initializeImages(vira::images::Resolution resolution);
        void updateImages(const DataPayload<TSpectral, TFloat>& dataPayload);
    };
};

#include "implementation/rendering/passes.ipp"

#endif