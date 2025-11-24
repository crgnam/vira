#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_utils.hpp"

namespace vira::images {
    template <IsPixel T>
    Image<T> alphaOver(const Image<T>& bottom, const Image<T>& top, AlphaOverOptions options)
    {
        Image<T> outImage = bottom;
        Image<T> topImage = top; // This copy can be modified

        // Scale image:
        if (options.scale != 1.f) {
            topImage.resize(options.scale);
        }

        // Get the alpha channel:
        Image<float> alpha;
        if (top.hasAlpha()) {
            alpha = Image<float>(topImage.resolution(), topImage.getAlpha());
        }
        else {
            alpha = Image<float>(topImage.resolution(), 1.f);
        }

        size_t top_resx = static_cast<size_t>(topImage.resolution().x);
        size_t top_resy = static_cast<size_t>(topImage.resolution().y);
        size_t bottom_resx = static_cast<size_t>(bottom.resolution().x);
        size_t bottom_resy = static_cast<size_t>(bottom.resolution().y);

        // Apply alpha screening:
        size_t xoff = static_cast<size_t>(options.position.x);
        size_t yoff = static_cast<size_t>(options.position.y);
        if (options.useCenter) {
            xoff = xoff - top_resx / 2;
            yoff = yoff - top_resy / 2;
        }

        for (size_t i = 0; i < top_resx; ++i) {
            for (size_t j = 0; j < top_resy; ++j) {
                size_t x = i + xoff;
                size_t y = j + yoff;

                if (x > 0 && y > 0 && x < bottom_resx && y < bottom_resy) {
                    T bmix = (1.f - alpha(i, j)) * bottom(x, y);
                    T tmix = alpha(i, j) * topImage(i, j);
                    outImage(x, y) = bmix + tmix;
                }
            }
        }

        return outImage;
    };
};