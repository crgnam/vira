#ifndef VIRA_DEMS_DEM_PROJECTION_HPP
#define VIRA_DEMS_DEM_PROJECTION_HPP

#include <string>
#include <array>
#include <cstdint>

// TODO REMOVE THIRD PARTY HEADERS:
#include "vira/platform/windows_compat.hpp" // MUST CALL BEFORE FITSIO.H
#include "gdal_priv.h"

#include "vira/vec.hpp"
#include "vira/images/resolution.hpp"

namespace vira::dems {
    struct DEMProjection {
        DEMProjection() = default;

        vira::images::Resolution resolution;
        std::array<double, 2> pixelScale{ 0, 0 };        

        // Tiling data:
        vira::images::Resolution tileDefinitions{ 0,0 };
        uint32_t tileRow = 0;
        uint32_t tileCol = 0;
        double xoff = 0;
        double yoff = 0;
        double dx = 1;
        double dy = 1;

        // OGR Specific data:
        bool isOGR = false;
        std::string projRef = ""; // ESRI WKT String
        std::array<double, 2> rowColRotation{ 0 ,0 };
        std::array<double, 2> modelTiePoint{ 0, 0 };

        inline std::string getTileString();

        // Modifier methods:
        inline void resize(vira::images::Resolution newResolution);

        // Initialization methods:
        inline void initToWorld();
        inline void initFromWorld();
        inline void initToMap(DEMProjection target);
        inline void free();

        // Transformation methods:
        inline vec3<double> transformToWorld(Pixel pixel, float z);
        inline Pixel transformFromWorld(double latitude, double longitude);
        inline Pixel transformToMap(Pixel pixel, DEMProjection& target);

    private:
        // Proj heap allocations:
        OGRSpatialReference srcSRS;
        OGRSpatialReference dstSRS;
        OGRCoordinateTransformation* poTransform;
        double a;
        double b;
        bool initializedOGR = false;

        // Functions for generating lonlat coordinate system:
        inline OGRSpatialReference SRStoLonLatProj(OGRSpatialReference& srs);
        inline OGRSpatialReference SRStoLonLatWKT(OGRSpatialReference& srs);

        // SPC Maplet information:
        double tileSizeX;
        double tileSizeY;
        double startX;
        double startY;
        double scale_x;
        double scale_y;
    };
};

#include "implementation/dems/dem_projection.ipp"

#endif