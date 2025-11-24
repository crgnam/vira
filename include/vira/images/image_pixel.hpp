#ifndef VIRA_IMAGES_IMAGE_PIXEL_HPP
#define VIRA_IMAGES_IMAGE_PIXEL_HPP

#include <type_traits>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::images {
    template <typename T>
    concept IsNonSpectralPixel = IsFloatingVec<T> || IsNumeric<T> || !IsSpectral<T>;

    template <typename T>
    concept IsColorRGB = std::is_same_v<T, vira::ColorRGB>;

    template <typename T>
    concept IsColorPixel = IsFloat<T> || IsColorRGB<T>;

    template <typename T>
    concept IsPixel = IsNonSpectralPixel<T> || IsSpectral<T>;

    enum class BufferDataType {
        UINT8,      // 8-bit unsigned integer
        UINT16,     // 16-bit unsigned integer  
        UINT32,     // 32-bit unsigned integer
        UINT64,     // 64-bit unsigned integer
        FLOAT32,    // 32-bit IEEE float
        FLOAT64     // 64-bit IEEE double
    };
};

#endif