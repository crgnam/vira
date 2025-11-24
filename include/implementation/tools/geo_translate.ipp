#include <stdexcept>
#include <vector>
#include <string>
#include <filesystem>

#include "gdal.h"
#include "gdal_utils.h"

#include "vira/utils/utils.hpp"

namespace fs = std::filesystem;

static void replaceBoundedSubstring(std::string& str, std::string newSubStr, std::string start_delim, std::string end_delim) {
    size_t start_pos = str.find(start_delim);
    if (start_pos == std::string::npos) {
        str.erase(str.find_last_not_of(" \t\n\r\f\v") + 1);
        str += " " + newSubStr;
        return;
    }
    start_pos = start_pos + start_delim.length();

    size_t end_pos = str.find(end_delim, start_pos);
    if (end_pos == std::string::npos) {
        str.erase(start_pos);
    }
    else {
        str.erase(start_pos, end_pos - start_pos);
    }
    str.insert(start_pos, newSubStr);
}

namespace vira::tools {
    void geoTranslate(const fs::path& inputFile, const fs::path& outputFile)
    {
        GDALAllRegister();

        // Read the current projection:
        vira::dems::setProjDataSearchPaths();
        GDALAllRegister();
        const GDALAccess eAccess = GA_ReadOnly;
        GDALDatasetUniquePtr poDataset = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(inputFile.string().c_str(), eAccess)));

        const OGRSpatialReference* srs = poDataset->GetSpatialRef();
        char* c_projStr = nullptr;
        srs->exportToProj4(&c_projStr);
        std::string projStr(c_projStr);

        // Create transformation:
        std::string latlonProjStr = "+proj=longlat";
        latlonProjStr += " +a=" + std::to_string(srs->GetSemiMajor());
        latlonProjStr += " +b=" + std::to_string(srs->GetSemiMinor());
        latlonProjStr += " +lon_0=" + std::to_string(srs->GetProjParm("CENTRAL_MERIDIAN"));
        OGRSpatialReference lonlatSRS;
        lonlatSRS.importFromProj4(latlonProjStr.c_str());
        OGRCoordinateTransformation* poTransform = OGRCreateCoordinateTransformation(&lonlatSRS, srs);

        // Compute current easting:
        double lon = 0;
        double lat = 0;
        poTransform->Transform(1, &lon, &lat);
        double currentEasting = lon;

        // Compute target easting:
        lon = 180;
        lat = 0;
        poTransform->Transform(1, &lon, &lat);
        double targetEasting = lon;

        // Compute new false easting:
        double lon_0 = 180; // Central Meridian of 180 corresponds to wrapping -180 to +180
        double x_0 = targetEasting - currentEasting; // False easting to offset the current origin to correspond to new Central Meridian

        // Clean-up:
        OGRCoordinateTransformation::DestroyCT(poTransform);
        lonlatSRS.Clear();
        poDataset->Close();

        // Update proj string and pass to gdal_translate options:
        replaceBoundedSubstring(projStr, std::to_string(lon_0), "+lon_0=", " +");
        replaceBoundedSubstring(projStr, std::to_string(x_0), "+x_0=", " +");
        std::vector<std::string> options = { "-a_srs", projStr };
        

        // Convert options to char**:
        std::vector<char*> args;
        for (const auto& arg : options) {
            args.push_back(const_cast<char*>(arg.data()));
        }
        args.push_back(nullptr);

        GDALTranslateOptions* psOptions = GDALTranslateOptionsNew(args.data(), nullptr);

        GDALDatasetH pdsData = GDALOpen(inputFile.string().c_str(), GA_ReadOnly);
        if (pdsData == nullptr) {
            throw std::runtime_error("GDAL Could not open the provided PDS-dataset");
        }

        // Generate a tmp filepath:
        std::string outputUse;
        if (outputFile == inputFile) {
            outputUse = "tmp";
        }
        else {
            outputUse = outputFile.string();
        }

        GDALDatasetH geotiffData = GDALTranslate(outputUse.c_str(), pdsData, psOptions, nullptr);
        if (!geotiffData) {
            throw std::runtime_error("GDALTranslate failed");
        }

        GDALReleaseDataset(geotiffData);
        GDALReleaseDataset(pdsData);
        GDALTranslateOptionsFree(psOptions);

        // If we are overwriting the output file, replace it:
        if (outputFile == inputFile) {
            if (fs::exists(outputUse)) {
                fs::remove(inputFile);
                fs::rename(outputUse, inputFile);
            }
            else {
                throw std::runtime_error("Failed to create temporary file");
            }
        }
    };

    // In-memory version that modifies the GeoreferenceImage directly
    void geoTranslateInMemory(vira::dems::GeoreferenceImage<float>& geoImage)
    {
        // Get the current projection from the GeoreferenceImage
        auto& projection = geoImage.projection;

        if (!projection.isOGR) {
            throw std::runtime_error("GeoreferenceImage must have OGR projection data");
        }

        // Parse the current projection string to extract parameters
        std::string projStr = projection.projRef;

        // Create temporary OGR objects to work with the projection
        vira::dems::setProjDataSearchPaths();
        GDALAllRegister();

        OGRSpatialReference srs;
        OGRErr err = srs.importFromWkt(projStr.c_str());
        if (err != OGRERR_NONE) {
            throw std::runtime_error("Failed to import projection from WKT string");
        }

        // Create transformation to lon/lat
        std::string latlonProjStr = "+proj=longlat";
        latlonProjStr += " +a=" + std::to_string(srs.GetSemiMajor());
        latlonProjStr += " +b=" + std::to_string(srs.GetSemiMinor());
        latlonProjStr += " +lon_0=" + std::to_string(srs.GetProjParm("central_meridian"));

        OGRSpatialReference lonlatSRS;
        err = lonlatSRS.importFromProj4(latlonProjStr.c_str());
        if (err != OGRERR_NONE) {
            throw std::runtime_error("Failed to create lon/lat coordinate system");
        }

        OGRCoordinateTransformation* poTransform = OGRCreateCoordinateTransformation(&lonlatSRS, &srs);
        if (!poTransform) {
            throw std::runtime_error("Failed to create coordinate transformation");
        }

        // Compute current easting at origin
        double lon = 0;
        double lat = 0;
        if (!poTransform->Transform(1, &lon, &lat)) {
            OGRCoordinateTransformation::DestroyCT(poTransform);
            throw std::runtime_error("Failed to transform coordinates");
        }
        double currentEasting = lon;

        // Compute target easting at 180 degrees
        lon = 180;
        lat = 0;
        if (!poTransform->Transform(1, &lon, &lat)) {
            OGRCoordinateTransformation::DestroyCT(poTransform);
            throw std::runtime_error("Failed to transform coordinates");
        }
        double targetEasting = lon;

        // Clean up transformation
        OGRCoordinateTransformation::DestroyCT(poTransform);

        // Compute new parameters
        double lon_0 = 180; // Central Meridian of 180 corresponds to wrapping -180 to +180
        double x_0 = targetEasting - currentEasting; // False easting to offset

        // Update the projection parameters in the SRS
        err = srs.SetProjParm("central_meridian", lon_0);
        if (err != OGRERR_NONE) {
            throw std::runtime_error("Failed to set central meridian");
        }

        err = srs.SetProjParm("false_easting", x_0);
        if (err != OGRERR_NONE) {
            throw std::runtime_error("Failed to set false easting");
        }

        // Export the modified projection back to WKT
        char* newWKT = nullptr;
        err = srs.exportToWkt(&newWKT);
        if (err != OGRERR_NONE || !newWKT) {
            throw std::runtime_error("Failed to export modified projection to WKT");
        }

        // Update the GeoreferenceImage's projection
        projection.projRef = std::string(newWKT);

        // Clean up
        CPLFree(newWKT);
        srs.Clear();
    }

    // Convenience function that loads, translates, and returns a new GeoreferenceImage
    vira::dems::GeoreferenceImage<float> geoTranslateFromFile(const fs::path& inputFile, vira::dems::GDALOptions options)
    {
        // Load the image using your existing interface
        auto geoImage = vira::dems::GDALInterface::load(inputFile, options);

        // Apply the translation in-memory
        geoTranslateInMemory(geoImage);

        return geoImage;
    }
};
