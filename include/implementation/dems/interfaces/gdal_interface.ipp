#include <string>
#include <cstdint>
#include <stdexcept>
#include <array>
#include <filesystem>

#include <iostream>

#include "gdal_priv.h"
#include "gdal_utils.h"
#include "cpl_error.h"

#include "vira/dems/georeference_image.hpp"
#include "vira/images/image.hpp"
#include "vira/dems/set_proj_data.hpp"

namespace fs = std::filesystem;

namespace vira::dems {

    // Simple GDAL error handler
    class SimpleGDALErrorHandler {
    public:
        SimpleGDALErrorHandler() {
            CPLSetErrorHandler(collectErrorHandler);
            CPLErrorReset();
        }

        ~SimpleGDALErrorHandler() {
            CPLSetErrorHandler(CPLDefaultErrorHandler);
        }

        void checkAndThrow(const std::string& context = "") {
            // Since we're now silently ignoring all GDAL messages,
            // this method doesn't need to do anything
            (void)context;
        }

    private:
        static std::string* errorMessage;

        static void CPL_STDCALL collectErrorHandler(CPLErr eErrClass, int err_no, const char* msg) noexcept {
            (void)err_no;  // Suppress unused parameter warning
            (void)eErrClass; // Suppress unused parameter warning
            (void)msg; // Suppress unused parameter warning
            // Silently ignore all GDAL messages (errors, warnings, info, etc.)
        }
    };

    // Static member definition
    std::string* SimpleGDALErrorHandler::errorMessage = nullptr;

    /**
     * @brief Loads a georeferenced image from file using GDAL
     * @param filepath Path to the image file
     * @param options GDAL loading options (default: empty)
     * @return GeoreferenceImage containing pixel data and spatial reference information
     * @throws std::runtime_error if file cannot be loaded or lacks georeferencing data
     */
    GeoreferenceImage<float> GDALInterface::load(const fs::path& filepath, GDALOptions options)
    {
        SimpleGDALErrorHandler errorHandler;

        // Open the file:
        setProjDataSearchPaths();
        GDALAllRegister();
        GDALDatasetUniquePtr poDataset(static_cast<GDALDataset*>(GDALOpen(filepath.string().c_str(), GA_ReadOnly)));
        if (!poDataset) {
            errorHandler.checkAndThrow("Could not open: " + filepath.string());
            throw std::runtime_error("Could not open: " + filepath.string());
        }
        errorHandler.checkAndThrow("Could not open: " + filepath.string());

        DEMProjection projection;
        projection.isOGR = true;
        projection.resolution = vira::images::Resolution{ poDataset->GetRasterXSize(), poDataset->GetRasterYSize() };
        projection.resolution.x = std::min(options.x_width, projection.resolution.x);
        projection.resolution.y = std::min(options.y_width, projection.resolution.y);
        projection.xoff = static_cast<double>(options.x_start);
        projection.yoff = static_cast<double>(options.y_start);

        // Read the projection information:
        double adfGeoTransform[6];
        CPLErr gdalError = poDataset->GetGeoTransform(adfGeoTransform);
        if (gdalError != CE_None) {
            errorHandler.checkAndThrow("Could not read the GeoTransform from " + filepath.string());
            throw std::runtime_error("Could not read the GeoTransform from " + filepath.string());
        }
        errorHandler.checkAndThrow("Could not read the GeoTransform from " + filepath.string());

        projection.modelTiePoint[0] = adfGeoTransform[0];
        projection.pixelScale[0] = adfGeoTransform[1];
        projection.rowColRotation[0] = adfGeoTransform[2];

        projection.modelTiePoint[1] = adfGeoTransform[3];
        projection.rowColRotation[1] = adfGeoTransform[4];
        projection.pixelScale[1] = adfGeoTransform[5];

        // Read projection data:
        projection.projRef = poDataset->GetProjectionRef();
        if (projection.projRef.empty()) {
            errorHandler.checkAndThrow("Could not read ProjectionRef from " + filepath.string());
            throw std::runtime_error("Could not read ProjectionRef from " + filepath.string());
        }
        errorHandler.checkAndThrow("Could not read ProjectionRef from " + filepath.string());

        // TODO Modify this to support multiple bands.
        // TODO Currently, it just reads the first band:
        //size_t numChannels = poDataset->GetRasterCount();
        int RASTER_BAND = 1; // Recall, the raster bands are numbered 1 through GDALRasterBand::GetRasterCount())

        // Fetch the current raster band:
        GDALRasterBand* poBand;
        poBand = poDataset->GetRasterBand(RASTER_BAND);
        if (!poBand) {
            errorHandler.checkAndThrow("Could not get raster band");
            throw std::runtime_error("Could not get raster band");
        }
        errorHandler.checkAndThrow("Could not get raster band");

        // Initialize image raster:
        vira::images::Image<float> data(projection.resolution);

        // Read the specified raster data:
        double x0 = static_cast<double>(options.x_start);
        double y0 = static_cast<double>(options.y_start);
        double xW = static_cast<double>(projection.resolution.x);
        double yW = static_cast<double>(projection.resolution.y);
        double dfScale = poBand->GetScale();
        gdalError = poBand->ReadRaster(data.getVector(), x0, y0, xW, yW);
        if (gdalError != CE_None) {
            errorHandler.checkAndThrow("Could not read RasterBand " + std::to_string(RASTER_BAND) + " from " + filepath.string());
            throw std::runtime_error("Could not read RasterBand " + std::to_string(RASTER_BAND) + " from " + filepath.string());
        }
        errorHandler.checkAndThrow("Could not read RasterBand " + std::to_string(RASTER_BAND) + " from " + filepath.string());

        // Apply scale to the loaded image data:
        float scale = static_cast<float>(dfScale) * options.scale;
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = scale * data[i];
        }

        if (!options.allow_negative) {
            for (size_t i = 0; i < data.size(); ++i) {
                if (data[i] < 0) {
                    data[i] = std::numeric_limits<float>::infinity();
                }
            }
        }

        poDataset->Close();

        return GeoreferenceImage<float>(std::move(data), projection);
    };

    /**
     * @brief Gets image resolution without loading the full image data
     * @param filepath Path to the image file
     * @return Image resolution (width x height) in pixels
     * @throws std::runtime_error if file cannot be read or is not a valid image
     */
    vira::images::Resolution GDALInterface::getResolution(const fs::path& filepath)
    {
        SimpleGDALErrorHandler errorHandler;

        // Open the file:
        setProjDataSearchPaths();
        GDALAllRegister();
        const GDALAccess eAccess = GA_ReadOnly;
        GDALDatasetH  hDataset = GDALOpen(filepath.string().c_str(), eAccess);
        if (hDataset == nullptr) {
            errorHandler.checkAndThrow("Could not open: " + filepath.string());
            throw std::runtime_error("Could not open: " + filepath.string());
        }
        errorHandler.checkAndThrow("Could not open: " + filepath.string());

        GDALDatasetUniquePtr poDataset = GDALDatasetUniquePtr(GDALDataset::FromHandle(hDataset));

        vira::images::Resolution resolution{ poDataset->GetRasterXSize(), poDataset->GetRasterYSize() };

        poDataset->Close();

        return resolution;
    };
};