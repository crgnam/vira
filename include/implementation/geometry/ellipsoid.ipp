#include <cmath>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/dems/georeference_image.hpp"
#include "vira/rotation.hpp"

namespace vira::geometry {
    template <IsFloat TFloat>
    Ellipsoid<TFloat>::Ellipsoid(TFloat semiMajor, TFloat semiMinor, vira::dems::GeoreferenceImage<float> newHeightMap) :
        a{ semiMajor }, b{ semiMinor }, heightMap{ newHeightMap }
    {
        this->e2 = 1 - (b * b) / (a * a);
    }

    template <IsFloat TFloat>
    TFloat Ellipsoid<TFloat>::getAltitude(TFloat lat, TFloat lon, bool isDegree)
    {
        if (!isDegree) {
            lat = 180 * lat / PI<TFloat>();
            lon = 180 * lon / PI<TFloat>();
        }

        TFloat alt;
        if (heightMap.size() != 0) {
            alt = heightMap.getLatLon(lat, lon);
        }
        else {
            alt = 0;
        }
        return alt;
    }

    template <IsFloat TFloat>
    vec3<TFloat> Ellipsoid<TFloat>::computePoint(TFloat lat, TFloat lon, bool isDegree)
    {
        TFloat alt = this->getAltitude(lat, lon, isDegree);
        return this->computePoint(lat, lon, alt, isDegree);
    }

    template <IsFloat TFloat>
    vec3<TFloat> Ellipsoid<TFloat>::computePoint(TFloat lat, TFloat lon, TFloat alt, bool isDegree)
    {
        if (isDegree) {
            lat = PI<TFloat>() * lat / 180;
            lon = PI<TFloat>() * lon / 180;
        }

        TFloat slon = std::sin(lon);
        TFloat clon = std::cos(lon);
        TFloat slat = std::sin(lat);
        TFloat clat = std::cos(lat);

        TFloat N = a / (std::sqrt(1 - (e2 * slat * slat)));
        vec3<TFloat> point{
            (N + alt) * clat * clon,
            (N + alt) * clat * slon,
            (N * (1 - e2) + alt) * slat
        };

        return point;
    }

    template <IsFloat TFloat>
    Rotation<TFloat> Ellipsoid<TFloat>::computeEastUpNorth(TFloat lat, TFloat lon, bool isDegree)
    {
        TFloat alt = this->getAltitude(lat, lon, isDegree);
        return this->computeEastUpNorth(lat, lon, alt, isDegree);
    }

    template <IsFloat TFloat>
    Rotation<TFloat> Ellipsoid<TFloat>::computeEastUpNorth(TFloat lat, TFloat lon, TFloat alt, bool isDegree)
    {
        if (isDegree) {
            lat = PI<TFloat>() * lat / 180;
            lon = PI<TFloat>() * lon / 180;
        }

        TFloat slon = std::sin(lon);
        TFloat clon = std::cos(lon);
        TFloat slat = std::sin(lat);
        TFloat clat = std::cos(lat);

        // Compute East:
        vec3<TFloat> east{ -slon, clon, 0 };
        east = normalize(east);

        // Compute Up:
        TFloat N = a / (std::sqrt(1 - (e2 * slat * slat)));
        vec3<TFloat> up{
            (N + alt) * clat * clon,
            (N + alt) * clat * slon,
            (N * (1 - e2) + alt) * slat
        };
        up = normalize(up);

        // Compute North:
        vec3<TFloat> north = cross(up, east);
        north = normalize(north);

        Rotation<TFloat> rotation(east, north, up);
        return rotation.inverse();
    }

    template <IsFloat TFloat>
    void Ellipsoid<TFloat>::initialize()
    {
        this->heightMap.initializeFromWorld();
    }

    template <IsFloat TFloat>
    void Ellipsoid<TFloat>::free()
    {
        this->heightMap.freeFromWorld();
    }
};