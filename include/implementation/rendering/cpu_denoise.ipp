#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"
#include "tbb/cache_aligned_allocator.h"

#include "vira/vec.hpp"
#include "vira/images/image.hpp"
#include "vira/spectral_data.hpp"

namespace vira::rendering {
    template<typename TSpectral>
    void denoiseSpectralRadianceEATWT(
        vira::images::Image<TSpectral>& direct,
        vira::images::Image<TSpectral>& indirect,
        const vira::images::Image<TSpectral>& albedo,
        const vira::images::Image<float>& depth,
        const vira::images::Image<vec3<float>>& normal,
        EATWTOptions options
    ) {
        // Separable 1D kernel (derived from 5-tap binomial)
        static constexpr float KERNEL_1D[5] = { 1.0f / 16.0f, 4.0f / 16.0f, 6.0f / 16.0f, 4.0f / 16.0f, 1.0f / 16.0f };

        int width = direct.resolution().x;
        int height = direct.resolution().y;

        // Separate albedo from lighting:
        vira::images::Image<TSpectral> direct_illum(width, height);
        vira::images::Image<TSpectral> indirect_illum(width, height);

        tbb::parallel_for(tbb::blocked_range2d<int>(0, height, options.TILE_SIZE, 0, width, options.TILE_SIZE),
            [&](const tbb::blocked_range2d<int>& range) {
                for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
                    for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
                        // SIMD-friendly spectral division
                        const TSpectral& direct_val = direct(y, x);
                        const TSpectral& indirect_val = indirect(y, x);
                        const TSpectral& albedo_val = albedo(y, x);

                        TSpectral& direct_illum_val = direct_illum(y, x);
                        TSpectral& indirect_illum_val = indirect_illum(y, x);

                        for (size_t c = 0; c < direct_val.size(); ++c) {
                            direct_illum_val[c] = direct_val[c] / std::max(albedo_val[c], options.EPSILON);
                            indirect_illum_val[c] = indirect_val[c] / std::max(albedo_val[c], options.EPSILON);
                        }
                    }
                }
            });

        // Denoising with separable filters and early termination:
        auto denoiseIlluminationOptimized = [&](vira::images::Image<TSpectral>& illum, int max_levels) {
            vira::images::Image<TSpectral> temp(width, height);
            vira::images::Image<TSpectral>* current = &illum;
            vira::images::Image<TSpectral>* next = &temp;

            // Helper lambda function for separable filtering
            auto filterPixelSeparable = [&](
                const vira::images::Image<TSpectral>& input,
                int center_x, int center_y, int step, bool horizontal,
                vira::images::Image<TSpectral>& output
                ) {
                    auto sum = input(center_y, center_x);
                    for (size_t c = 0; c < sum.size(); ++c) {
                        sum[c] = 0.0f;
                    }
                    float weight_sum = 0.0f;

                    float center_depth = depth(center_y, center_x);
                    vec3<float> center_normal = normal(center_y, center_x);

                    // Apply 1D kernel in specified direction
                    for (int k = -2; k <= 2; ++k) {
                        int sample_x = horizontal ? center_x + k * step : center_x;
                        int sample_y = horizontal ? center_y : center_y + k * step;

                        // Boundary check with clamping
                        sample_x = std::clamp(sample_x, 0, width - 1);
                        sample_y = std::clamp(sample_y, 0, height - 1);

                        float kernel_weight = KERNEL_1D[k + 2];

                        // Compute optimized edge weights
                        float sample_depth = depth(sample_y, sample_x);
                        vec3<float> sample_normal = normal(sample_y, sample_x);

                        // Fast depth weight with relative threshold
                        float depth_ratio = std::max(center_depth, sample_depth) / (std::min(center_depth, sample_depth) + options.EPSILON);
                        float depth_weight = depth_ratio < (1.0f + options.DEPTH_THRESHOLD) ? 1.0f :
                            std::exp(-(depth_ratio - 1.0f) / options.DEPTH_THRESHOLD);

                        // Smooth normal weight with faster computation
                        float normal_dot = center_normal.x * sample_normal.x +
                            center_normal.y * sample_normal.y +
                            center_normal.z * sample_normal.z;

                        // Smooth falloff instead of hard threshold
                        float normal_similarity = std::max(0.0f, normal_dot);
                        float normal_weight = std::max(options.MIN_NORMAL_WEIGHT,
                            std::exp(-std::pow(1.0f - normal_similarity, 2.0f) / options.NORMAL_THRESHOLD));

                        float total_weight = kernel_weight * depth_weight * normal_weight;

                        // Vectorized spectral accumulation
                        auto sample_value = input(sample_y, sample_x);
                        for (size_t c = 0; c < sum.size(); ++c) {
                            sum[c] += sample_value[c] * total_weight;
                        }
                        weight_sum += total_weight;
                    }

                    // Normalize with fallback
                    if (weight_sum > options.EPSILON) {
                        for (size_t c = 0; c < sum.size(); ++c) {
                            output(center_y, center_x)[c] = sum[c] / weight_sum;
                        }
                    }
                    else {
                        output(center_y, center_x) = input(center_y, center_x);
                    }
                };

            for (int level = 0; level < max_levels; ++level) {
                int step = 1 << level;

                // Two-pass separable filtering for better cache efficiency
                
                // Horizontal Pass:
                tbb::parallel_for(tbb::blocked_range<int>(0, height, options.TILE_SIZE / 4),
                    [&](const tbb::blocked_range<int>& range) {
                        for (int y = range.begin(); y != range.end(); ++y) {
                            for (int x = 0; x < width; ++x) {
                                filterPixelSeparable(*current, x, y, step, true, *next);
                            }
                        }
                    });

                // Vertical Pass:
                std::swap(current, next);
                tbb::parallel_for(tbb::blocked_range<int>(0, height, options.TILE_SIZE / 4),
                    [&](const tbb::blocked_range<int>& range) {
                        for (int y = range.begin(); y != range.end(); ++y) {
                            for (int x = 0; x < width; ++x) {
                                filterPixelSeparable(*current, x, y, step, false, *next);
                            }
                        }
                    });

                std::swap(current, next);
            }

            // Ensure result is in the original illum buffer
            if (current != &illum) {
                illum = std::move(*current);
            }
            };

        // Process direct and indirect in parallel using parallel_for
        tbb::parallel_for(tbb::blocked_range<int>(0, 2),
            [&](const tbb::blocked_range<int>& range) {
                for (int i = range.begin(); i != range.end(); ++i) {
                    if (i == 0) {
                        denoiseIlluminationOptimized(direct_illum, options.MAX_LEVELS_DIRECT);
                    }
                    else {
                        denoiseIlluminationOptimized(indirect_illum, options.MAX_LEVELS_INDIRECT);
                    }
                }
            });

        // Recombine with albedo (optimized with larger tiles)
        tbb::parallel_for(tbb::blocked_range2d<int>(0, height, options.TILE_SIZE, 0, width, options.TILE_SIZE),
            [&](const tbb::blocked_range2d<int>& range) {
                for (int y = range.rows().begin(); y != range.rows().end(); ++y) {
                    for (int x = range.cols().begin(); x != range.cols().end(); ++x) {
                        const TSpectral& direct_illum_val = direct_illum(y, x);
                        const TSpectral& indirect_illum_val = indirect_illum(y, x);
                        const TSpectral& albedo_val = albedo(y, x);

                        TSpectral& direct_val = direct(y, x);
                        TSpectral& indirect_val = indirect(y, x);

                        for (size_t c = 0; c < direct_val.size(); ++c) {
                            direct_val[c] = direct_illum_val[c] * albedo_val[c];
                            indirect_val[c] = indirect_illum_val[c] * albedo_val[c];
                        }
                    }
                }
            });
    }
}