#ifndef VIRA_CAMERAS_APERTURE_SHAPES_CIRCULAR_APERTURE_HPP
#define VIRA_CAMERAS_APERTURE_SHAPES_CIRCULAR_APERTURE_HPP

#include <random>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/cameras/apertures/aperture.hpp"
#include "vira/images/image.hpp"

namespace vira::cameras {
    /**
     * @brief Circular aperture implementation for cameras
     * @tparam TSpectral Spectral data type for radiance calculations
     * @tparam TFloat Floating point precision type
     *
     * Implements a circular camera aperture that controls light collection area,
     * depth of field ray sampling through uniform disk sampling, and produces
     * circular bokeh effects when rendering UnresolvedObject instances.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class CircularAperture : public Aperture<TSpectral, TFloat> {
    public:
        CircularAperture() = default;

        vira::vec2<float> samplePoint(std::mt19937& rng, std::uniform_real_distribution<float>& dist) const override;
        void applyPointSourceBokeh(vira::images::Image<TSpectral>& image, const Pixel& center_position, const TSpectral& intensity) const override;
    };
};

#include "implementation/cameras/apertures/circular_aperture.ipp"

#endif