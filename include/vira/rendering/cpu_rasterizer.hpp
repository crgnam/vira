#ifndef VIRA_RENDERING_CPU_RASTERIZER_HPP
#define VIRA_RENDERING_CPU_RASTERIZER_HPP

#include <vector>
#include <cstdint>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/camera.hpp"
#include "vira/rendering/passes.hpp"

namespace vira::scene {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;
};

namespace vira::rendering {
    // Rasterizer options:
    struct CPURasterizerOptions {
        
    };

    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class CPURasterizer {
    public:
        CPURasterizer(CPURasterizerOptions options = CPURasterizerOptions{});

        void render(vira::cameras::Camera<TSpectral, TFloat, TMeshFloat>& camera, vira::Scene<TSpectral, TFloat, TMeshFloat>& Scene);

        CPURasterizerOptions options;

        // Render output settings:
        vira::rendering::RenderPasses<TSpectral, TFloat> renderPasses{};

    private:
        bool inFrame(Pixel& point, vira::images::Resolution& resolution);
    };
};

#include "implementation/rendering/cpu_rasterizer.ipp"

#endif
