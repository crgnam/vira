#ifndef VIRA_CAMERAS_APERTURES_APERTURE_HPP
#define VIRA_CAMERAS_APERTURES_APERTURE_HPP

#include <random>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image.hpp"

namespace vira::cameras {
    /**
     * @brief Abstract base class for camera aperture implementations
     * @tparam TSpectral Spectral data type for radiance calculations
     * @tparam TFloat Floating point precision type
     *
     * Defines the interface for camera apertures that control the light
     * collection area (for converting received radiance into power), the
     * depth of field ray sampling, and bokeh effects for rendering 
     * UnresolvedObject instances.  Apertures can sample points within 
     * their geometry and render bokeh effects for out-of-focus point 
     * light sources.
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    class Aperture {
    public:
        virtual ~Aperture() = default;

        /**
         * @brief Samples a random point within the aperture shape
         * @param rng Random number generator for sampling
         * @param dist Uniform real distribution for generating random values
         * @return A 2D point within the aperture bounds
         */
        virtual vira::vec2<float> samplePoint(std::mt19937& rng, std::uniform_real_distribution<float>& dist) const = 0;
        
        /**
         * @brief Applies aperture-shaped bokeh effect for a projected point light source
         * @param image The image to render the bokeh effect onto
         * @param center_position The image coordinates where the point source projects to
         * @param intensity The spectral intensity/radiance of the point light source
         * @note The bokeh shape and size are determined by the aperture geometry and radius
         */
        virtual void applyPointSourceBokeh(vira::images::Image<TSpectral>& image, const Pixel& center_position, const TSpectral& intensity) const = 0;

        void setRadius(float new_radius);
        void setDiameter(float new_diameter);
        void setArea(float new_area);

        float getRadius() const { return radius_; }
        float getDiameter() const { return diameter_; }
        float getArea() const { return area_; }

    protected:
        float radius_ = 0;
        float diameter_ = 0;
        float area_ = 0;
    };
};

#include "implementation/cameras/apertures/aperture.ipp"

#endif