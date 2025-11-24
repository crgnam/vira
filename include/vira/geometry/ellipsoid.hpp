#ifndef VIRA_GEOMETRY_ELLIPSOID_HPP
#define VIRA_GEOMETRY_ELLIPSOID_HPP

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/dems/georeference_image.hpp"
#include "vira/rotation.hpp"

namespace vira::geometry {
    template <IsFloat TFloat>
    class Ellipsoid {
    public:
        Ellipsoid(TFloat a, TFloat b, vira::dems::GeoreferenceImage<float> heightMap = vira::dems::GeoreferenceImage<float>{});

        TFloat getAltitude(TFloat lat, TFloat lon, bool isDegree = false);

        vec3<TFloat> computePoint(TFloat lat, TFloat lon, bool isDegree = false);
        vec3<TFloat> computePoint(TFloat lat, TFloat lon, TFloat alt, bool isDegree = false);

        Rotation<TFloat> computeEastUpNorth(TFloat lat, TFloat lon, bool isDegree = false);
        Rotation<TFloat> computeEastUpNorth(TFloat lat, TFloat lon, TFloat alt, bool isDegree = false);

        void initialize();
        void free();

    private:
        TFloat a;
        TFloat b;
        TFloat e2;

        vira::dems::GeoreferenceImage<float> heightMap;
    };
};


#include "implementation/geometry/ellipsoid.ipp"

#endif