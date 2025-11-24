#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

#include "vira/vec.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_pixel.hpp"

namespace vira::rendering {
    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, T setValue, const Pixel& p0, const Pixel& p1, float width)
    {
        vira::images::Image<float> depth(vira::images::Resolution{ 0,0 });
        drawLine(image, depth, setValue, p0, p1, 0, 0, width);
    };

    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, vira::images::Image<float>& depth, T setValue, const Pixel& p0, const Pixel& p1, float p0Depth, float p1Depth, float width)
    {
        bool useDepth = true;
        if (depth.resolution() != image.resolution()) {
            useDepth = false;
        }
        float slope = (p1Depth - p0Depth) / length(p1 - p0);

        // Bresenham algorithm to draw anti-aliased thick lines between pixels p0 and p1
        int x0 = static_cast<int>(p0.x);
        int y0 = static_cast<int>(p0.y);
        int x1 = static_cast<int>(p1.x);
        int y1 = static_cast<int>(p1.y);

        int dx = std::abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        int e2;

        int rx = static_cast<int>(image.resolution().x);
        int ry = static_cast<int>(image.resolution().y);

        float halfWidth = width / 2.0f;
        int halfWidthCeil = static_cast<int>(std::ceil(halfWidth));

        float currentDepth = 0;
        while (true) {
            if (useDepth) {
                Pixel currentPixel{ x0, y0 };
                currentDepth = p0Depth + length(currentPixel - p0) * slope;
            }

            for (int wy = -halfWidthCeil; wy <= halfWidthCeil; ++wy) {
                for (int wx = -halfWidthCeil; wx <= halfWidthCeil; ++wx) {
                    int px = x0 + wx;
                    int py = y0 + wy;

                    if (px >= 0 && px < rx && py >= 0 && py < ry) {

                        // Check if valid depth:
                        if (useDepth) {
                            if (currentDepth >= depth(px, py)) {
                                continue;
                            }
                        }

                        float distance = static_cast<float>(std::sqrt(wx * wx + wy * wy));
                        if (distance <= halfWidth) {
                            float alpha = 1.0f - (distance / halfWidth);
                            image(px, py) = alpha * setValue + (1.f - alpha) * image(px, py);
                        }
                    }
                }
            }

            if (x0 == x1 && y0 == y1) {
                break;
            }

            e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }



    template <vira::images::IsPixel T>
    void drawBox(vira::images::Image<T>& image, T setValue, const Pixel& topLeft, const Pixel& bottomRight, float width)
    {
        Pixel bottomLeft{ topLeft.x, bottomRight.y };
        Pixel topRight{ bottomRight.x, topLeft.y };

        drawLine<T>(image, setValue, topLeft, bottomLeft, width);
        drawLine<T>(image, setValue, bottomLeft, bottomRight, width);
        drawLine<T>(image, setValue, bottomRight, topRight, width);
        drawLine<T>(image, setValue, topRight, topLeft, width);
    };



    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, T setValue, const std::vector<Pixel>& pixels, float width)
    {
        vira::images::Image<float> depth(vira::images::Resolution{ 0,0 });
        std::vector<float> pixelDepths(pixels.size(), 0);
        drawLine(image, depth, setValue, pixels, pixelDepths, width);
    };

    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, vira::images::Image<float>& depth, T setValue, const std::vector<Pixel>& pixels, const std::vector<float>& pixelDepths, float width)
    {
        for (size_t i = 0; i < (pixels.size() - 1); ++i) {
            drawLine<T>(image, depth, setValue, pixels[i], pixels[i + 1], pixelDepths[i], pixelDepths[i + 1], width);
        }
    };



    template <vira::images::IsPixel T>
    void drawPolygon(vira::images::Image<T>& image, T setValue, const std::vector<Pixel>& pixels, float width)
    {
        vira::images::Image<float> depth(vira::images::Resolution{ 0,0 });
        std::vector<float> pixelDepths(pixels.size(), 0);
        drawPolygon(image, depth, setValue, pixels, pixelDepths, width);
    };

    template <vira::images::IsPixel T>
    void drawPolygon(vira::images::Image<T>& image, vira::images::Image<float>& depth, T setValue, const std::vector<Pixel>& pixels, const std::vector<float>& pixelDepths, float width)
    {
        size_t END = pixels.size() - 1;
        drawLine<T>(image, depth, setValue, pixels, pixelDepths, width);
        drawLine<T>(image, depth, setValue, pixels[END], pixels[0], pixelDepths[END], pixelDepths[0], width);
    };
};