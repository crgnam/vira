#ifndef VIRA_RENDERING_BRESENHAM_HPP
#define VIRA_RENDERING_BRESENHAM_HPP

#include <vector>

#include "vira/vec.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_pixel.hpp"

namespace vira::rendering {
    // Reference: https://zingl.github.io/bresenham.html

    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, vira::images::Image<float>& depth, T setValue, const Pixel& p0, const Pixel& p1, float p0Depth, float p1Depth, float width = 1.f);



    template <vira::images::IsPixel T>
    void drawBox(vira::images::Image<T>& image, T setValue, const Pixel& topLeft, const Pixel& bottomRight, float width = 1.f);



    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, T setValue, const std::vector<Pixel>& pixels, float width = 1.f);

    template <vira::images::IsPixel T>
    void drawLine(vira::images::Image<T>& image, vira::images::Image<float>& depth, T setValue, const std::vector<Pixel>& pixels, const std::vector<float>& pixelDepths, float width = 1.f);



    template <vira::images::IsPixel T>
    void drawPolygon(vira::images::Image<T>& image, T setValue, const std::vector<Pixel>& pixels, float width = 1.f);

    template <vira::images::IsPixel T>
    void drawPolygon(vira::images::Image<T>& image, vira::images::Image<float>& depth, T setValue, const std::vector<Pixel>& pixels, const std::vector<float>& pixelDepths, float width = 1.f);
};

#include "implementation/rendering/bresenham.ipp"

#endif