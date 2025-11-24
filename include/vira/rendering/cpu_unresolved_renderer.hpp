#ifndef VIRA_RENDERING_CPU_UNRESOLVED_RENDERING_HPP
#define VIRA_RENDERING_CPU_UNRESOLVED_RENDERING_HPP

#include <vector>

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/cameras/camera.hpp"

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
};

namespace vira::rendering {
    struct CPUUnresolvedRendererOptions {
        bool simulate_stellar_aberration = false;
        bool simulate_light_time_correction = false;
    };

    template <IsSpectral TSpectral>
    struct ProjectedPoint {
        Pixel point;
        TSpectral received_power;
    };

    template <IsSpectral TSpectral>
    struct UnresolvedPasses {
        vira::images::Image<TSpectral> unresolved_power;
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class CPUUnresolvedRenderer {
    public:
        CPUUnresolvedRenderer(CPUUnresolvedRendererOptions options = CPUUnresolvedRendererOptions{});

        void render(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& scene);

        CPUUnresolvedRendererOptions options;

        UnresolvedPasses<TSpectral> renderPasses{};

    private:
        std::vector<ProjectedPoint<TSpectral>> findPointsInRegion(const vira::images::ROI& roi, const vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, const std::vector<vira::vec3<TFloat>>& vectors, const std::vector<TSpectral>& irradiances, float minimumPower);
        void processRegion(const vira::images::ROI& roi, vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, const std::vector<vira::vec3<TFloat>>& vectors, const std::vector<TSpectral>& irradiances, float minimumPower);

    };
}

#include "implementation/rendering/cpu_unresolved_renderer.ipp"

#endif