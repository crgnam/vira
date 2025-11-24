#ifndef VIRA_DEMS_INTERFACES_GDAL_INTERFACE_HPP
#define VIRA_DEMS_INTERFACES_GDAL_INTERFACE_HPP

#include <filesystem>

#include "vira/dems/georeference_image.hpp"

namespace fs = std::filesystem;

namespace vira::dems {
    /**
     * @brief Configuration options for GDAL raster loading operations
     * @details Provides fine-grained control over how georeferenced raster data is loaded
     *          from files. Supports spatial subsetting, value scaling, and validation
     *          constraints to handle diverse geospatial datasets efficiently.
     */
    struct GDALOptions {
        float scale = 1.f;                             ///< Scale to abe applied to the loaded raster values
        bool allow_negative = true;                    ///< Allow negative raster values (Should be False for albedo)
        int x_start = 0;                               ///< Starting raster x-coordinate
        int y_start = 0;                               ///< Starting raster y-coordinate
        int x_width = std::numeric_limits<int>::max(); ///< Ending raster x-coordinate (defaults to numeric_limits::max, which will read the whole image)
        int y_width = std::numeric_limits<int>::max(); ///< Ending raster y-coordinate (defaults to numeric_limits::max, which will read the whole image)
    };

    /**
    * @brief Interface for loading georeferenced raster data using GDAL
    * @details Provides static methods for reading geospatial raster files with full
    *          spatial reference information. Supports all GDAL-compatible formats including
    *          GeoTIFF. Automatically handles coordinate system transformations and
    *          metadata extraction for seamless integration with spatial analysis workflows.
    */
    class GDALInterface {
    public:
        static GeoreferenceImage<float> load(const fs::path& filepath, GDALOptions options = GDALOptions{});

        static vira::images::Resolution getResolution(const fs::path& filepath);
    };
};

#include "implementation/dems/interfaces/gdal_interface.ipp"

#endif