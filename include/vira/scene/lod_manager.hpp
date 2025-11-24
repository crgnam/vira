#ifndef VIRA_SCENE_LOD_MANAGER_HPP
#define VIRA_SCENE_LOD_MANAGER_HPP

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/rendering/cpu_rasterizer.hpp"

namespace vira::scene {

    // Forward Declare:
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;

    struct LevelOfDetailOptions {
        float target_triangle_size = 2.f;

        float cone_angle = 0; // (Zero indicates to ignore this override)
        float max_view_angle = 90;

        bool check_shadows = true;

        bool parallel_update = true;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class LevelOfDetailManager {
    public:
        LevelOfDetailManager() = default;

        LevelOfDetailOptions options;

        void update(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene);

    private:
        
        vira::rendering::CPURasterizer<TSpectral, TFloat, TMeshFloat> rasterizer;

        void updateByMapping(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene);
    };
};

#include "implementation/scene/lod_manager.ipp"

#endif