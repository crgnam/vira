#ifndef VIRA_IMAGES_COMPOSITING_HPP
#define VIRA_IMAGES_COMPOSITING_HPP

#include "vira/vec.hpp"
#include "vira/images/image_pixel.hpp"
#include "vira/images/image.hpp"

namespace vira::images {
    struct AlphaOverOptions {
        float scale = 1.f;
        Pixel position{ 0,0 }; // Pixel coordinates in the bottom image
        bool useCenter = false;
    };

    template <IsPixel T>
    Image<T> alphaOver(const Image<T>& bottom, const Image<T>& top, AlphaOverOptions options = AlphaOverOptions{});
};

#include "implementation/images/compositing.ipp"

#endif