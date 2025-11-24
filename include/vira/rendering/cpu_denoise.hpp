#ifndef VIRA_RENDERING_DENOISE_HPP
#define VIRA_RENDERING_DENOISE_HPP

#include "vira/vec.hpp"
#include "vira/images/image.hpp"
#include "vira/spectral_data.hpp"

namespace vira::rendering {
    struct EATWTOptions {
        int MAX_LEVELS_DIRECT = 5;      // Reduced for direct lighting
        int MAX_LEVELS_INDIRECT = 5;    // Full levels for indirect
        float DEPTH_THRESHOLD = 0.2f;   // More permissive
        float NORMAL_THRESHOLD = 0.5f;  // More permissive
        float MIN_NORMAL_WEIGHT = 0.3f; // Ensure minimum filtering
        size_t TILE_SIZE = 64;          // Cache-friendly tile size

        float EPSILON = 1e-6f;
    };

    // Edge-Avoiding A-Trous Wavelet Transform (EATWT) denoising:
    template<typename TSpectral>
    void denoiseSpectralRadianceEATWT(
        vira::images::Image<TSpectral>& direct,
        vira::images::Image<TSpectral>& indirect,
        const vira::images::Image<TSpectral>& albedo,
        const vira::images::Image<float>& depth,
        const vira::images::Image<vec3<float>>& normal,
        EATWTOptions options = EATWTOptions{}
    );
}

#include "implementation/rendering/cpu_denoise.ipp"

#endif