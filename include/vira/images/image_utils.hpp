#ifndef VIRA_IMAGES_IMAGE_UTILS_HPP
#define VIRA_IMAGES_IMAGE_UTILS_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <functional>

#include "vira/vec.hpp"
#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/images/color_map.hpp"

namespace vira::images {

    // ============================ //
    // === Colorspace Modifiers === //
    // ============================ //
    inline ColorRGB linearToSRGB_val(ColorRGB linearRGB);
    inline Image<ColorRGB> linearToSRGB(Image<ColorRGB> linearImage);

    inline ColorRGB sRGBToLinear_val(ColorRGB sRGB);
    inline Image<ColorRGB> sRGBToLinear(Image<ColorRGB> sRGBImage);

    inline Image<ColorRGB> monoToRGB(const Image<float>& image);

    inline Image<ColorRGB> idToRGB(const Image<size_t>& image);

    inline Image<ColorRGB> velocityToRGB(const Image<vec3<float>>& image);

    inline Image<ColorRGB> triangleSizeToRGB(const Image<float>& image, float triangleSize);

    inline Image<ColorRGB> colorMap(Image<float> image, std::vector<ColorRGB> colormap = colormaps::viridis());

    inline Image<ColorRGB> formatNormals(const Image<vec3<float>>& image);


    // ========================= //
    // === Channel Modifiers === //
    // ========================= //
    template <typename TSpectral, typename... Channels>
    concept ValidChannels = (sizeof...(Channels) == TSpectral::size() || sizeof...(Channels) == 1);

    template <typename T>
    concept IsFloatImage = std::same_as<T, Image<float>>;

    template <IsSpectral TSpectral>
    std::vector<Image<float>> channelSplit(const Image<TSpectral>& image);

    template <IsSpectral TSpectral>
    Image<TSpectral> channelMerge(const std::vector<Image<float>>& channels);

    template <IsSpectral TSpectral, IsFloatImage... Channels> requires ValidChannels<TSpectral, Channels...>
    Image<TSpectral> channelMerge(Channels... channels);

    template <IsSpectral TSpectral>
    Image<float> spectralToMono(const Image<TSpectral>& image);


    template <IsSpectral TSpectral>
    Image<ColorRGB> spectralToRGB(const Image<TSpectral>& image, std::function<ColorRGB(const TSpectral&)>colorConvert = spectralToRGB_val<TSpectral>);

    template <IsSpectral TSpectral>
    Image<ColorRGB> defaultSpectralToRGB(const Image<TSpectral>& image) { return spectralToRGB(image); }


    template <IsSpectral TSpectral>
    Image<TSpectral> rgbToSpectral(const Image<TSpectral>& image, std::function<TSpectral(const ColorRGB&)>colorConvert = rgbToSpectral_val<TSpectral>);

    template <IsSpectral TSpectral>
    Image<TSpectral> defaultRGBtoSpectral(const Image<ColorRGB>& image) { return rgbToSpectral(image); }


    template <IsSpectral TSpectral1, IsSpectral TSpectral2>
    Image<TSpectral2> spectralConvert(const Image<TSpectral1>& image, std::function<TSpectral2(const TSpectral1&)>colorConvert = spectralConvert_val<TSpectral1, TSpectral2>);

    template <IsSpectral TSpectral1, IsSpectral TSpectral2>
    Image<TSpectral2> defaultSpectralConvert(const Image<TSpectral1>& image) { return spectralConvert(image); }


    // ========================= //
    // === Generic Utilities === //
    // ========================= //
    template <IsPixel T>
    vira::geometry::IndexBuffer imageToIndexBuffer(const Image<T>& image);

    template <IsPixel T>
    std::vector<vira::vec2<size_t>> getValidPerimeter(const Image<T>& image);

    template <IsInteger T>
    Image<T> floatToFixed(const Image<float>& image, std::array<float, 2> minMax, bool clamp = false);

    template <IsInteger T>
    Image<float> fixedToFloat(const Image<T>& image, std::array<float, 2> minMax);

    static inline std::vector<std::array<int, 4>> computeChunks(vira::images::Resolution resolution, int64_t maxAllowedPixels);
};

#include "implementation/images/image_utils.ipp"

#endif