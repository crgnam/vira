#include <vector>
#include <cstddef>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/unresolved/star_light.hpp"
#include "vira/unresolved/unresolved_object.hpp"

namespace vira::unresolved {
    StarCatalogue::StarCatalogue(std::vector<Star> newStars) :
        stars{ newStars }
    {

    };

    StarCatalogue::StarCatalogue(size_t N)
    {
        this->stars = std::vector<Star>(N);
    };

    void StarCatalogue::sortByMagnitude()
    {
        if (stars.size() > 0) {
            std::sort(stars.begin(), stars.end(), stars[0].compareByMag);
        }
    };

    void StarCatalogue::append(StarCatalogue newList)
    {
        stars.insert(stars.end(), newList.stars.begin(), newList.stars.end());
    };

    template <IsSpectral TSpectral>
    std::vector<TSpectral> StarCatalogue::getIrradiances() const
    {
        std::vector<TSpectral> irradiances(stars.size());

        vira::debug::tbb_debug(); // Only has effect in Debug mode (switches to single threaded)
        tbb::parallel_for(tbb::blocked_range<size_t>(0, stars.size()),
            [&](tbb::blocked_range<size_t> r)
            {
                for (size_t i = r.begin(); i < r.end(); ++i) {
                    irradiances[i] = stars[i].getIrradiance<TSpectral>();
                }
            });
        return irradiances;
    };

    template <IsFloat TFloat>
    std::vector<vec3<TFloat>> StarCatalogue::getUnitVectors(double et) const
    {
        std::vector<vec3<TFloat>> unitVectors(stars.size());

        for (size_t i = 0; i < stars.size(); ++i) {
            unitVectors[i] = stars[i].getUnitVector<TFloat>(et);
        }

        return unitVectors;
    };

    template <IsSpectral TSpectral, IsFloat TFloat>
    std::vector<StarLight<TSpectral, TFloat>> StarCatalogue::makeStarLight(double et) const
    {
        std::vector<vec3<TFloat>> unitVectors = this->getUnitVectors<TFloat>(et);
        std::vector<TSpectral> irradiances = this->getIrradiances<TSpectral>();

        std::vector<StarLight<TSpectral, TFloat>> starlight;
        starlight.reserve(unitVectors.size());
        for (size_t i = 0; i < unitVectors.size(); ++i) {
            starlight.emplace_back(irradiances[i], unitVectors[i]);
        }

        return starlight;
    };

    

    const Star& StarCatalogue::operator[] (size_t idx) const
    {
        if (idx >= stars.size()) {
            throw std::runtime_error("Attempted out-of-bounds linear index (" + std::to_string(idx) +
                "), on StarCatalogue with only " + std::to_string(stars.size()) + " stars");
        }
        return stars[idx];
    };

    
};