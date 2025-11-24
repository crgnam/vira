#include "vira/math.hpp"
#include "vira/utils/valid_value.hpp"

namespace vira::cameras {
    /**
     * @brief Sets the aperture radius and updates diameter and area accordingly
     * @param new_radius The new radius value (must be positive)
     * @throws std::invalid_argument if new_radius is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void Aperture<TSpectral, TFloat>::setRadius(float new_radius)
    {
        vira::utils::validatePositiveDefinite(new_radius, "Aperture Radius");
        this->radius_ = new_radius;
        this->diameter_ = 2 * new_radius;
        this->area_ = PI<float>() * this->radius_ * this->radius_;
    }

    /**
     * @brief Sets the aperture diameter and updates radius and area accordingly
     * @param new_diameter The new diameter value (must be positive)
     * @throws std::invalid_argument if new_diameter is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void Aperture<TSpectral, TFloat>::setDiameter(float new_diameter)
    {
        vira::utils::validatePositiveDefinite(new_diameter, "Aperture Diameter");
        this->radius_ = new_diameter / 2.f;
        this->diameter_ = new_diameter;
        this->area_ = PI<float>() * this->radius_ * this->radius_;
    }

    /**
     * @brief Sets the aperture area and updates radius and diameter accordingly
     * @param new_area The new area value (must be positive)
     * @throws std::invalid_argument if new_area is not positive definite
     */
    template <IsSpectral TSpectral, IsFloat TFloat>
    void Aperture<TSpectral, TFloat>::setArea(float new_area)
    {
        vira::utils::validatePositiveDefinite(new_area, "Aperture Area");
        this->radius_ = std::sqrt(new_area / PI<float>());
        this->diameter_ = 2 * this->radius_;
        this->area_ = new_area;
    }
}