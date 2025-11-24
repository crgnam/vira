#include <random>

#include "vira/constraints.hpp"
#include "vira/sampling.hpp"
#include "vira/images/image.hpp"

namespace vira::cameras {
    /**
     * @brief Samples a random point within the circular aperture
     * @param rng Random number generator for sampling
     * @param dist Uniform real distribution for generating random values
     * @return A 2D point within the circular aperture, scaled by the aperture radius
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    vec2<float> CircularAperture<TSpectral, TFloat>::samplePoint(std::mt19937& rng, std::uniform_real_distribution<float>& dist) const
    {
        return this->radius_ * UniformDiskSample(rng, dist);
    }

    /**
     * @brief Applies circular bokeh effect for a projected point light source
     * @param image The image to render the circular bokeh onto
     * @param center_position The image coordinates where the point source projects to (bokeh center)
     * @param intensity The spectral intensity of the point light source
     * @note Creates a circular bokeh pattern centered at the specified position
     * @todo This method is currently not implemented
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void CircularAperture<TSpectral, TFloat>::applyPointSourceBokeh(vira::images::Image<TSpectral>& image, const Pixel& center_position, const TSpectral& intensity) const
    {
        // TODO Implement circular bokeh rendering for point source
        (void)image;
        (void)center_position;
        (void)intensity;
    }
}