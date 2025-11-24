#include <string>

#include "gdal_priv.h"

#include "vira/vec.hpp"
#include "vira/images/image.hpp"
#include "vira/utils/utils.hpp"
#include "vira/dems/set_proj_data.hpp"


namespace vira::dems {
    std::string DEMProjection::getTileString()
    {
        std::string tileString = std::to_string(tileRow + 1) + "-" + std::to_string(tileCol + 1) + "_";
        tileString += std::to_string(tileDefinitions.y) + "-" + std::to_string(tileDefinitions.x);
        return tileString;
    };

    void DEMProjection::resize(vira::images::Resolution newResolution)
    {
        // Calculate initial scaling:
        double x_size = this->dx * (static_cast<double>(resolution.x) - 1.);
        double y_size = this->dy * (static_cast<double>(resolution.y) - 1.);

        // Calculate the new pixel scale:
        this->dx = x_size / (static_cast<double>(newResolution.x) - 1.);
        this->dy = y_size / (static_cast<double>(newResolution.y) - 1.);

        // Update the stored resolution:
        this->resolution = newResolution;
    };

    void DEMProjection::initToMap(DEMProjection target)
    {

        if (isOGR) {
            setProjDataSearchPaths();

            srcSRS = OGRSpatialReference(projRef.c_str());
            dstSRS = OGRSpatialReference(target.projRef.c_str());
            poTransform = OGRCreateCoordinateTransformation(&srcSRS, &dstSRS);

            initializedOGR = true;
        }
        else {
            throw std::runtime_error("Both source and target DEMProjection must be from an OGR in order to initialize a transform");
        }
    };

    void DEMProjection::initToWorld()
    {
        if (isOGR) {
            if (!initializedOGR) {
                setProjDataSearchPaths();

                srcSRS = OGRSpatialReference(projRef.c_str());
                a = srcSRS.GetSemiMajor();
                b = srcSRS.GetSemiMinor();
                dstSRS = SRStoLonLatProj(srcSRS);
                poTransform = OGRCreateCoordinateTransformation(&srcSRS, &dstSRS);

                initializedOGR = true;
            }
        }
        else {
            tileSizeX = (static_cast<double>(resolution.x) - 1.) / 2.;
            tileSizeY = (static_cast<double>(resolution.y) - 1.) / 2.;
            startX = -tileSizeX;
            startY = -tileSizeY;
            scale_x = (tileSizeX + 0.5) / static_cast<double>(resolution.x);
            scale_y = (tileSizeY + 0.5) / static_cast<double>(resolution.y);
        }
    };

    void DEMProjection::initFromWorld()
    {
        if (isOGR) {
            if (!initializedOGR) {
                setProjDataSearchPaths();

                dstSRS = OGRSpatialReference(projRef.c_str());
                a = dstSRS.GetSemiMajor();
                b = dstSRS.GetSemiMinor();
                srcSRS = SRStoLonLatProj(dstSRS);
                poTransform = OGRCreateCoordinateTransformation(&srcSRS, &dstSRS);

                initializedOGR = true;
            }
        }
    };

    void DEMProjection::free()
    {
        if (isOGR && initializedOGR) {
            OGRCoordinateTransformation::DestroyCT(poTransform);
            srcSRS.Clear();
            dstSRS.Clear();
            initializedOGR = false;
        }
    };

    vec3<double> DEMProjection::transformToWorld(Pixel pixel, float z)
    {
        vec3<double> position{ std::numeric_limits<double>::infinity() };
        if (isOGR) {
            if (initializedOGR) {
                // Compute pixel origin:
                double X_pixel = static_cast<double>(pixel[0]);
                double Y_pixel = static_cast<double>(pixel[1]);

                // Convert from pixel to raster space:
                double Ix = (dx * X_pixel) + xoff + 0.5;
                double Iy = (dy * Y_pixel) + yoff + 0.5;

                // Convert from raster space to model space:
                double X_geo_i = modelTiePoint[0] + (Ix * pixelScale[0]) + (Iy * rowColRotation[0]);
                double Y_geo_i = modelTiePoint[1] + (Ix * rowColRotation[1]) + (Iy * pixelScale[1]);

                // Perform transformation:
                double X_geo = X_geo_i;
                double Y_geo = Y_geo_i;
                int succeed = poTransform->Transform(1, &X_geo, &Y_geo);
                if (succeed) {
                    position = utils::ellipsoidToCartesian<double>(X_geo, Y_geo, static_cast<double>(z), a, b);
                }
            }
            else {
                throw std::runtime_error("transformToWorld was not initialized");
            }
        }
        else {
            double Ix = static_cast<double>(pixel[0]) + (xoff / dx);
            double Iy = static_cast<double>(pixel[1]) + (yoff / dy);

            double X_step = Ix * scale_x;
            double Y_step = -Iy * scale_y;

            position.x = dx * pixelScale[0] * (startX + (2. * X_step));
            position.y = dy * pixelScale[1] * (-startY + (2. * Y_step));
            position.z = static_cast<double>(z);
        }

        return position;
    };

    Pixel DEMProjection::transformFromWorld(double latitude, double longitude)
    {
        if (isOGR) {
            if (initializedOGR) {
                double X_geo = longitude;
                double Y_geo = latitude;
                poTransform->Transform(1, &X_geo, &Y_geo);

                std::array<double, 2>& P = pixelScale;
                std::array<double, 2>& r = rowColRotation;
                std::array<double, 2>& M = modelTiePoint;

                // Enforce wrapping:
                double X_m = (((Y_geo - M[1]) * r[0]) - (X_geo - M[0]) * P[1]);
                double Ix_proj = X_m / (r[0] * r[1] - P[0] * P[1]);

                double Y_m = ((Y_geo - M[1]) - (Ix_proj * r[1]));
                double Iy_proj = Y_m / P[1];

                // Convert from traget raster space to target pixel space:
                double x = ((Ix_proj - xoff - 0.5) / dx);
                double y = ((Iy_proj - yoff - 0.5) / dy);

                return Pixel{ x,y };
            }
            else {
                throw std::runtime_error("transformFromWorld was not initialized");
            }
        }
        else {
            throw std::runtime_error("Not OGR.  transformFromWorld was not initialized");
        }
    };

    Pixel DEMProjection::transformToMap(Pixel pixel, DEMProjection& target)
    {
        Pixel defaultPixel{ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
        if (isOGR && initializedOGR) {
            // Compute pixel origin:
            double X_pixel = static_cast<double>(pixel[0]);
            double Y_pixel = static_cast<double>(pixel[1]);

            // Convert from pixel to raster space:
            double Ix = (dx * X_pixel) + xoff + 0.5;
            double Iy = (dy * Y_pixel) + yoff + 0.5;

            // Convert from raster space to model space:
            double X_geo = modelTiePoint[0] + (Ix * pixelScale[0]) + (Iy * rowColRotation[0]);
            double Y_geo = modelTiePoint[1] + (Ix * rowColRotation[1]) + (Iy * pixelScale[1]);

            // Perform transformation:
            int succeed = poTransform->Transform(1, &X_geo, &Y_geo);
            if (succeed) {
                std::array<double, 2>& P = target.pixelScale;
                std::array<double, 2>& r = target.rowColRotation;
                std::array<double, 2>& M = target.modelTiePoint;

                // Enforce wrapping:
                double X_m = (((Y_geo - M[1]) * r[0]) - (X_geo - M[0]) * P[1]);
                double Ix_proj = X_m / (r[0] * r[1] - P[0] * P[1]);

                double Y_m = ((Y_geo - M[1]) - (Ix_proj * r[1]));
                double Iy_proj = Y_m / P[1];

                // Convert from traget raster space to target pixel space:
                double x = ((Ix_proj - target.xoff - 0.5) / target.dx);
                double y = ((Iy_proj - target.yoff - 0.5) / target.dy);

                return Pixel{ x,y };
            }
        }

        return defaultPixel;
    };


    OGRSpatialReference DEMProjection::SRStoLonLatProj(OGRSpatialReference& srs)
    {
        std::string latlonProjStr = "+proj=longlat";
        latlonProjStr += " +a=" + std::to_string(srs.GetSemiMajor());
        latlonProjStr += " +b=" + std::to_string(srs.GetSemiMinor());
        OGRSpatialReference lonlatSRS;
        OGRErr ogrErr = lonlatSRS.importFromProj4(latlonProjStr.c_str());
        if (ogrErr != 0) {
            throw std::runtime_error("importFromProj4 fails with OGRErr = " + std::to_string(ogrErr));
        }

        return lonlatSRS;
    };

    OGRSpatialReference DEMProjection::SRStoLonLatWKT(OGRSpatialReference& srs)
    {
        double SemiMajor = srs.GetSemiMajor();
        double InvFlattening = srs.GetInvFlattening();

        std::string GEO = "GEOGCS";
        std::string NAME = srs.GetName();
        const char* c_DATUM = srs.GetAttrValue("DATUM");
        std::string DATUM = "UNKNOWN";
        if (c_DATUM != nullptr) {
            DATUM = std::string(c_DATUM);
        }

        const char* c_SPHEROID = srs.GetAttrValue("SPHEROID");
        std::string SPHEROID = "UNKNOWN";
        if (c_SPHEROID != nullptr) {
            SPHEROID = std::string(c_SPHEROID);
        }

        const char* c_PRIMEM = srs.GetAttrValue("PRIMEM");
        std::string PRIMEM = "UNKNOWN";
        if (c_PRIMEM != nullptr) {
            PRIMEM = std::string(c_PRIMEM);
        }

        double prime_angle = srs.GetPrimeMeridian();
        std::string ANGLEUNIT = "Degree";
        double angle = srs.GetAngularUnits();

        // #include <format> not supported by GCC yet!
        //std::string lonlatWKTString = std::format("{}[\"{}\", DATUM[\"{}\", SPHEROID[\"{}\", {}, {}]], PRIMEM[\"{}\", {}], UNIT[\"{}\", {}], AXIS[\"Lon\", EAST], AXIS[\"Lat\", NORTH]]", 
        //    GEO, NAME, DATUM, SPHEROID, SemiMajor, InvFlattening, PRIMEM, prime_angle, ANGLEUNIT, angle);

        std::string lonlatWKTString = GEO + "[\"" + NAME + "\", DATUM[\"" + DATUM + "\", SPHEROID[\"" + SPHEROID + "\", " +
            std::to_string(SemiMajor) + ", " + std::to_string(InvFlattening) + "]], PRIMEM[\"" + PRIMEM + "\", " + std::to_string(prime_angle) + "], " +
            "UNIT[\"" + ANGLEUNIT + "\", " + std::to_string(angle) + "], AXIS[\"Lon\", EAST], AXIS[\"Lat\", NORTH]]";

        OGRSpatialReference lonlatSRS;
        OGRErr ogrErr = lonlatSRS.importFromWkt(lonlatWKTString.c_str());
        if (ogrErr != 0) {
            throw std::runtime_error("importFromWkt fails with OGRErr = " + std::to_string(ogrErr));
        }

        return lonlatSRS;
    };
};