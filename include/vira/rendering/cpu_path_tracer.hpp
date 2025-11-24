#ifndef VIRA_RENDERING_CPU_PATH_TRACER_HPP
#define VIRA_RENDERING_CPU_PATH_TRACER_HPP

#include <memory>
#include <vector>
#include <random>
#include <cstddef>

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/lights/light.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/rendering/acceleration/tlas.hpp"
#include "vira/rendering/passes.hpp"
#include "vira/rendering/cpu_denoise.hpp"

// Forward Declare:
namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
};

namespace vira::rendering {
    // Path tracer options:
    enum PathTracerType : uint8_t {
        UNIDIRECTIONAL
    };
    
    struct CPUPathTracerOptions {
        size_t samples = 1;
        size_t bounces = 0;

        bool show_background = false;
        bool denoise = false;
        
        PathTracerType tracingType = UNIDIRECTIONAL;

        bool adaptive_sampling = false;
        size_t samples_per_batch = 30;
        float sampling_tolerance = 0.05f;
        size_t samples_to_detect_miss = 30;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class CPUPathTracer {
    public:
        CPUPathTracer() = default;

        void render(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& Scene);

        CPUPathTracerOptions options;

        // Render output settings:
        vira::rendering::RenderPasses<TSpectral, TFloat> renderPasses{};

        EATWTOptions denoiserOptions{};

    private:
        // Path-tracer methods:
        TSpectral unidirectional(cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, 
            DataPayload<TSpectral, TFloat>& dataPayload, std::mt19937& rng, std::uniform_real_distribution<float>& dist);

        TSpectral simulatePath(cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, Ray<TSpectral, TFloat>& ray, DataPayload<TSpectral, TFloat>& dataPayload, std::mt19937& rng, std::uniform_real_distribution<float>& dist);
        TSpectral processIntersection(cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, Ray<TSpectral, TFloat>& ray, DataPayload<TSpectral, TFloat>& dataPayload, std::mt19937& rng, std::uniform_real_distribution<float>& dist);
        
        float PowerHeuristic(int numf, float fPdf, int numg, float gPdf);

        vec3<TFloat> projectOnPlane(vec3<TFloat> position, vec3<TFloat> origin, Normal normal);
        vec3<TFloat> computeShadingPoint(const vec3<TFloat>& intersection, const std::array<vira::geometry::Vertex<TSpectral, TFloat>, 3>& vert, const std::array<TFloat, 3>& w, const vec3<float>& shadingNormal);
    };
};

#include "implementation/rendering/cpu_path_tracer.ipp"

#endif