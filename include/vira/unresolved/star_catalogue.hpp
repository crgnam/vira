#ifndef VIRA_UNRESOLVED_STAR_CATALOGUE_HPP
#define VIRA_UNRESOLVED_STAR_CATALOGUE_HPP

#include <vector>

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/unresolved/star.hpp"
#include "vira/unresolved/star_light.hpp"
#include "vira/unresolved/unresolved_object.hpp"

namespace vira::unresolved {
    class StarCatalogue {
    public:
        StarCatalogue() = default;
        StarCatalogue(std::vector<Star> stars);
        StarCatalogue(size_t N);

        inline void sortByMagnitude();
        inline void append(StarCatalogue newList);

        template <IsSpectral TSpectral>
        std::vector<TSpectral> getIrradiances() const;

        template <IsFloat TFloat>
        std::vector<vec3<TFloat>> getUnitVectors(double et) const;

        template <IsSpectral TSpectral, IsFloat TFloat>
        std::vector<StarLight<TSpectral, TFloat>> makeStarLight(double et) const;

        const Star& operator[] (size_t idx) const;
        Star& operator[] (size_t idx) { return const_cast<Star&>(const_cast<const StarCatalogue*>(this)->operator[](idx)); }
        size_t size() { return stars.size(); }

        const std::vector<Star>& getVector() const { return stars; }
        std::vector<Star>& getVector() { return const_cast<std::vector<Star>&>(const_cast<const StarCatalogue*>(this)->getVector()); }

    private:
        std::vector<Star> stars;
    };
};

#include "implementation/unresolved/star_catalogue.ipp"

#endif