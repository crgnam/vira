#ifndef VIRA_DEMS_GEOREFERENCE_IMAGE_HPP
#define VIRA_DEMS_GEOREFERENCE_IMAGE_HPP

#include <cstddef>
#include <concepts>

#include "vira/images/image.hpp"
#include "vira/dems/dem_projection.hpp"

namespace vira::dems {
    /**
    * @brief Concept defining valid data types for TIFF-based geospatial images
    * @details Restricts template types to spectral data or float values, ensuring
    *          compatibility with standard geospatial file formats and operations.
    */
    template <typename T>
    concept IsTIFFData = IsSpectral<T> || std::same_as<T, float>;

    /**
    * @brief Georeferenced image combining raster data with spatial reference information
    * @tparam T Data type for pixel values (must satisfy IsTIFFData concept)
    *
    * @details Represents a raster image with complete spatial referencing capabilities,
    *          combining pixel data with coordinate system transformations. Provides
    *          comprehensive geospatial operations including reprojection, resampling,
    *          tiling, and coordinate-based queries. Essential for working with satellite
    *          imagery, elevation data, and other georeferenced datasets where spatial
    *          accuracy and coordinate system handling are critical.
    *
    *          Supports both spectral (multi-band) and single-band float data types,
    *          making it suitable for diverse applications from remote sensing to
    *          digital elevation modeling. The integrated projection system handles
    *          coordinate transformations between different spatial reference systems,
    *          enabling seamless integration of datasets from multiple sources.
    */
    template <IsTIFFData T>
    class GeoreferenceImage {
    public:
        GeoreferenceImage() = default;

        GeoreferenceImage(const vira::images::Image<T>& set_data, DEMProjection set_projection);

        GeoreferenceImage(vira::images::Image<T>&& set_data, DEMProjection set_projection);

        vira::images::Image<T> data{};
        DEMProjection projection{};

        const float& operator[] (size_t idx) const { return this->data[idx]; } ///< Obtain read-only pixel access via linear index
        float& operator[] (size_t idx) { return const_cast<float&>(const_cast<const GeoreferenceImage*>(this)->operator[](idx)); } ///< Obtain mutable pixel reference via linear index

        const float& operator() (int i, int j) const { return this->data(i, j); } ///< Obtain read-only pixel access via pixel coordinates
        float& operator() (int i, int j) { return const_cast<float&>(const_cast<const GeoreferenceImage*>(this)->operator()(i, j)); } ///< Obtain mutable pixel reference via pixel coordinates

        T interpolatePixel(const Pixel& pixel) const { return this->data.interpolatePixel(pixel); } ///< Bilinear interpolation at sub-pixel coordinates

        vira::images::Resolution resolution() const { return data.resolution(); } ///< Returns image dimensions in pixels
        size_t size() const { return data.size(); } ///< Returns total number of pixels
        void appendRight(GeoreferenceImage<T> target_map, int num_columns = 1);
        void appendBelow(GeoreferenceImage<T> target_map, int num_rows = 1);
        void padRight(int num_columns = 1);
        void padBelow(int num_rows = 1);
        void resize(float scale);
        void resize(vira::images::Resolution new_resolution);
        void fillMissing(T value);
        void fillMissing();
        void clear();

        std::vector<GeoreferenceImage<T>> tile(int max_vertices = 1000000, int overlap = 0);
        std::vector<GeoreferenceImage<T>> manualTile(int row_slices, int col_slices, int overlap = 0);

        template <IsTIFFData T2>
        GeoreferenceImage<T2> newBlank();

        void removeOverlap(const GeoreferenceImage<T>& map);
        void sampleFrom(const GeoreferenceImage<T>& map);

        float getLatLon(float latitude, float longitude);

        void initializeFromWorld();
        void freeFromWorld();

    private:
        void warpTo(const GeoreferenceImage<T>& map, bool delete_overlap);

        bool initialized_from_world = false;
    };
};

#include "implementation/dems/georeference_image.ipp"

#endif