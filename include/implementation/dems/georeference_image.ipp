#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <vector>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"
#include "gdalwarper.h"

#include "vira/images/image.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/dems/dem_projection.hpp"
#include "vira/utils/valid_value.hpp"

namespace vira::dems {

    /**
     * @brief Constructs georeferenced image from existing image data
     * @param set_data Image data to be georeferenced
     * @param set_projection Spatial projection defining coordinate system
     */
    template <IsTIFFData T>
    GeoreferenceImage<T>::GeoreferenceImage(const vira::images::Image<T>& set_data, DEMProjection set_projection) :
        data{ set_data }, projection{ set_projection }
    {

    }

    /**
     * @brief Constructs georeferenced image with move semantics
     * @param set_data Image data to be georeferenced (moved)
     * @param set_projection Spatial projection defining coordinate system
     * @note Uses move semantics to avoid copying large image data
     */
    template <IsTIFFData T>
    GeoreferenceImage<T>::GeoreferenceImage(vira::images::Image<T>&& set_data, DEMProjection set_projection) :
        data{ std::move(set_data) }, projection{ set_projection }
    {

    }

    /**
     * @brief Extends the image to the right by padding with edge values
     * @param numColumns Number of columns to pad on the right side
     * @details Creates a padding strip by replicating the rightmost column values,
     *          then appends it to the right side of the current image
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::padRight(int numColumns)
    {
        GeoreferenceImage<T> pad(vira::images::Image<T>(vira::images::Resolution{ numColumns, this->resolution().y }), this->projection);
        for (int i = 0; i < numColumns; ++i) {
            for (int j = 0; j < this->resolution().y; ++j) {
                pad(i, j) = this->data(this->resolution().x - 1, j);
            }
        }

        this->appendRight(pad);
    };

    /**
     * @brief Appends another georeferenced image to the right of the current image
     * @param target_map The georeferenced image to append
     * @param numColumns Number of columns to take from the target map (default: all columns)
     * @throws std::runtime_error If the images have different heights
     * @details Creates a new larger image buffer and copies data from both images.
     *          If numColumns is less than target image width, crops the target first.
     * @todo Optimize to modify existing buffer without duplicate allocation
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::appendRight(GeoreferenceImage<T> target_map, int numColumns)
    {
        vira::images::Resolution res = this->data.resolution();
        vira::images::Resolution targetRes = target_map.data.resolution();

        if (res.y != targetRes.y) {
            throw std::runtime_error("Image appended to the right must have a matching height");
        }
        numColumns = std::min(numColumns, targetRes.x);
        if (numColumns == 0) {
            return;
        }

        vira::images::Image<T> strip = target_map.data;
        if (strip.resolution().x > numColumns) {
            vira::images::ROI roi(0, 0, numColumns, targetRes.y, vira::images::ROI_CORNER_DIM);
            strip.crop(roi);
        }
        else {
            numColumns = strip.resolution().x;
        }


        // TODO modify the existing buffer, without making a duplicate allocation:
        // Allocate new data image:
        vira::images::Resolution newRes{ res.x + numColumns, res.y };
        vira::images::Image<T> newData(newRes);

        // Copy old data image into the new data image:
        for (int i = 0; i < res.x; i++) {
            for (int j = 0; j < res.y; j++) {
                newData(i, j) = data(i, j);
            }
        }

        // Copy the newly cropped data to insert into the new data image:
        for (int i = 0; i < numColumns; i++) {
            for (int j = 0; j < res.y; j++) {
                newData(i + res.x, j) = strip(i, j);
            }
        }

        this->data = newData;
    };

    /**
     * @brief Extends the image downward by padding with edge values
     * @param num_rows Number of rows to pad below the current image
     * @details Creates a padding strip by replicating the bottom row values,
     *          then appends it below the current image
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::padBelow(int num_rows)
    {
        GeoreferenceImage<T> pad(Image<T>(vira::images::Resolution{ this->resolution().x, num_rows }), this->projection);
        for (int i = 0; i < this->resolution().x; ++i) {
            for (int j = 0; j < num_rows; ++j) {
                pad(i, j) = this->data(i, this->resolution().y - 1);
            }
        }

        this->appendBelow(pad);
    };

    /**
     * @brief Appends another georeferenced image below the current image
     * @param target_map The georeferenced image to append below
     * @param num_rows Number of rows to take from the target map (default: all rows)
     * @throws std::runtime_error If the images have different widths
     * @details Creates a new larger image buffer and copies data from both images.
     *          If num_rows is less than target image height, crops the target first.
     * @todo Optimize to modify existing buffer without duplicate allocation
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::appendBelow(GeoreferenceImage<T> target_map, int num_rows)
    {
        vira::images::Resolution res = data.resolution();
        vira::images::Resolution targetRes = target_map.data.resolution();

        if (res.x != targetRes.x) {
            throw std::runtime_error("Image appended below must have a matching width");
        }
        num_rows = std::min(num_rows, targetRes.y);
        if (num_rows == 0) {
            return;
        }

        vira::images::Image<T> strip = target_map.data;
        if (strip.resolution().y > num_rows) {
            vira::images::ROI roi(0, 0, targetRes.x, num_rows, vira::images::ROI_CORNER_DIM);
            strip.crop(roi);
        }
        else {
            num_rows = strip.resolution().y;
        }

        // TODO modify the existing buffer, without making a duplicate allocation:
        // Allocate new data image:
        vira::images::Resolution newRes{ res.x, res.y + num_rows };
        vira::images::Image<T> newData(newRes);

        // Copy old data image into the new data image:
        for (int i = 0; i < res.x; i++) {
            for (int j = 0; j < res.y; j++) {
                newData(i, j) = data(i, j);
            }
        }

        // Copy the newly cropped data to insert into the new data image:
        for (int i = 0; i < res.x; i++) {
            for (int j = 0; j < num_rows; j++) {
                newData(i, j + res.y) = strip(i, j);
            }
        }

        this->data = newData;
    };

    /**
     * @brief Resizes the georeferenced image by a scale factor
     * @param scale Scaling factor to apply (e.g., 0.5 for half size, 2.0 for double size)
     * @details Calculates new resolution based on scale factor and calls resize(Resolution)
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::resize(float scale)
    {
        vira::images::Resolution new_resolution = scale * this->resolution();
        this->resize(new_resolution);
    };

    /**
     * @brief Resizes the georeferenced image to a specific resolution
     * @param new_resolution Target resolution for the resized image
     * @details Resamples both the image data and updates the spatial projection.
     *          Returns early if the target resolution matches current resolution.
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::resize(vira::images::Resolution new_resolution)
    {
        if (new_resolution == this->resolution()) {
            return;
        }

        // Subsample the data:
        this->data.resize(new_resolution);

        // Subsample projection:
        this->projection.resize(new_resolution);
    };

    /**
     * @brief Fills missing/invalid pixels with a specified value
     * @param value The value to use for filling missing pixels
     * @details Delegates to the underlying image's fillMissing method
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::fillMissing(T value)
    {
        this->data.fillMissing(value);
    };

    /**
     * @brief Fills missing/invalid pixels using default fill strategy
     * @details Uses the underlying image's default fill method (typically interpolation)
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::fillMissing()
    {
        this->data.fillMissing();
    };

    /**
     * @brief Clears the georeferenced image and resets projection
     * @details Clears image data and reinitializes projection with OGR flag set to true
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::clear()
    {
        data.clear();
        projection = DEMProjection{};
        projection.isOGR = true;
    };

    /**
     * @brief Automatically tiles the image based on maximum vertex constraint
     * @param max_vertices Maximum number of pixels allowed per tile
     * @param overlap Number of pixels to overlap between adjacent tiles
     * @return Vector of tiled georeferenced images
     * @details Calculates optimal tiling pattern based on aspect ratio and vertex limit.
     *          Uses ceiling division to ensure complete coverage of the original image.
     */
    template <IsTIFFData T>
    std::vector<GeoreferenceImage<T>> GeoreferenceImage<T>::tile(int max_vertices, int overlap)
    {
        // TODO Consolidate this interface with GeoreferenceImage!
        vira::images::Resolution resolution = this->resolution();
        double w = static_cast<double>(resolution.x);
        double h = static_cast<double>(resolution.y);
        double aspect = w / static_cast<double>(resolution.y);
        double n = std::ceil(w / std::sqrt(static_cast<double>(max_vertices)));

        int row_slices = static_cast<int>(std::round(n / aspect));
        int col_slices = static_cast<int>(n);

        double nC = static_cast<double>(col_slices);
        double nR = static_cast<double>(row_slices);
        double ov = static_cast<double>(overlap);
        double tw = std::floor((w + (nC - 1) * ov) / nC);
        double th = std::floor((h + (nR - 1) * ov) / nR);

        if (static_cast<int>(tw * th) > max_vertices) {
            row_slices++;
            col_slices++;
        }

        return this->manualTile(row_slices, col_slices, overlap);
    };

    /**
     * @brief Manually tiles the image with specified grid dimensions
     * @param row_slices Number of rows in the tiling grid
     * @param col_slices Number of columns in the tiling grid
     * @param overlap Number of pixels to overlap between adjacent tiles
     * @return Vector of tiled georeferenced images (excluding tiles with all invalid data)
     * @details Creates a regular grid of tiles with specified overlap. Each tile maintains
     *          its own projection with updated offsets and tile metadata. Tiles containing
     *          only invalid data are excluded from the result.
     */
    template <IsTIFFData T>
    std::vector<GeoreferenceImage<T>> GeoreferenceImage<T>::manualTile(int row_slices, int col_slices, int overlap)
    {
        // Calculate tile dimensions:
        vira::images::Resolution resolution = this->resolution();
        double w = static_cast<double>(resolution.x);
        double h = static_cast<double>(resolution.y);
        double nC = static_cast<double>(col_slices);
        double nR = static_cast<double>(row_slices);

        vira::images::Resolution tileResolution;
        tileResolution.x = static_cast<int>(std::floor((w + (nC - 1) * static_cast<double>(overlap)) / nC));
        tileResolution.y = static_cast<int>(std::floor((h + (nR - 1) * static_cast<double>(overlap)) / nR));

        // Delete any existing tile definition:
        size_t numTiles = static_cast<size_t>(col_slices) * static_cast<size_t>(row_slices);
        std::vector<GeoreferenceImage<T>> tiles(numTiles);

        // Generate tile definitions:
        size_t idx = 0;
        for (int col = 0; col < col_slices; col++) {
            for (int row = 0; row < row_slices; row++) {

                vira::images::Resolution useResolution = tileResolution;

                // Calculate starting point of the tile:
                int xoff = col * (tileResolution.x - overlap - 1);
                int yoff = row * (tileResolution.y - overlap - 1);

                // Ensure we cover the entire DEM:
                if (col == col_slices - 1) {
                    useResolution.x = resolution.x - xoff;
                }
                if (row == row_slices - 1) {
                    useResolution.y = resolution.y - yoff;
                }

                // Create the projection:
                DEMProjection newProjection = projection;
                newProjection.resolution = useResolution;
                newProjection.tileDefinitions = vira::images::Resolution{ col_slices, row_slices };
                newProjection.tileRow = static_cast<uint32_t>(row);
                newProjection.tileCol = static_cast<uint32_t>(col);
                newProjection.xoff = projection.xoff + projection.dx * static_cast<double>(xoff);
                newProjection.yoff = projection.yoff + projection.dy * static_cast<double>(yoff);

                // Extract the height data:
                bool allInvalid = true;
                vira::images::Image<T> useData(useResolution);
                for (int i = 0; i < useResolution.x; i++) {
                    for (int j = 0; j < useResolution.y; j++) {
                        auto val = this->data(i + xoff, j + yoff);
                        useData(i, j) = val;
                        if (!std::isinf(val)) {
                            allInvalid = false;
                        }
                    }
                }

                if (allInvalid) {
                    continue;
                }

                tiles[idx] = GeoreferenceImage<T>(useData, newProjection);

                idx++;
            }
        }

        // Remove unused tiles (unused due to invalid data):
        tiles.erase(tiles.begin() + static_cast<std::ptrdiff_t>(idx), tiles.end());

        return tiles;
    };

    /**
     * @brief Creates a new blank georeferenced image with different data type
     * @tparam T2 Target image data type for the new blank image
     * @return New georeferenced image with same projection but empty/invalid data
     * @details Creates an appropriately initialized blank image based on the target type:
     *          - Float types: initialized with infinity values
     *          - Spectral types: initialized with infinity values
     *          - Other types: default initialization
     */
    template <IsTIFFData T>
    template <IsTIFFData T2>
    GeoreferenceImage<T2> GeoreferenceImage<T>::newBlank()
    {
        if (IsFloat<T2>) {
            vira::images::Image<T2> emptyData(this->resolution(), std::numeric_limits<T2>::infinity());
            return GeoreferenceImage<T2>(emptyData, projection);
        }
        else if (IsSpectral<T2>) {
            vira::images::Image<T2> emptyData(this->resolution(), T2{ std::numeric_limits<T2>::infinity() });
            return GeoreferenceImage<T2>(emptyData, projection);
        }
        else {
            vira::images::Image<T2> emptyData(this->resolution());
            return GeoreferenceImage<T2>(emptyData, projection);
        }
    };

    /**
     * @brief Removes overlapping regions with another georeferenced image
     * @param map Reference image to check for overlaps
     * @details Sets overlapping pixels to invalid values. Uses warpTo with delete flag.
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::removeOverlap(const GeoreferenceImage<T>& map)
    {
        this->warpTo(map, true);
    };

    /**
     * @brief Samples values from another georeferenced image
     * @param map Source image to sample from
     * @details Updates pixel values by sampling from the source image at corresponding
     *          geographic locations. Uses warpTo without delete flag.
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::sampleFrom(const GeoreferenceImage<T>& map)
    {
        this->warpTo(map, false);
    };

    /**
     * @brief Initializes projection for world coordinate transformations
     * @details Sets up the projection system for coordinate transformations.
     *          Only initializes once using the initialized_from_world flag.
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::initializeFromWorld()
    {
        if (!initialized_from_world) {
            projection.initFromWorld();
            initialized_from_world = true;
        }
    };

    /**
     * @brief Frees resources used for world coordinate transformations
     * @details Cleans up projection resources and resets the initialization flag.
     *          Only frees if previously initialized.
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::freeFromWorld()
    {
        if (initialized_from_world) {
            projection.free();
            initialized_from_world = false;
        }
    };

    /**
     * @brief Gets pixel value at specified latitude/longitude coordinates
     * @param latitude Latitude coordinate in degrees
     * @param longitude Longitude coordinate in degrees
     * @return Pixel value at the specified world coordinates
     * @details Transforms world coordinates to pixel coordinates and returns the value.
     *          Handles projection initialization/cleanup automatically if not persistent.
     */
    template <IsTIFFData T>
    float GeoreferenceImage<T>::getLatLon(float latitude, float longitude)
    {
        if (!initialized_from_world) {
            projection.initFromWorld();
        }

        Pixel pixelPoint = projection.transformFromWorld(latitude, longitude);

        if (!initialized_from_world) {
            projection.free();
        }

        return (*this)(static_cast<size_t>(pixelPoint.x), static_cast<size_t>(pixelPoint.y));
    };

    /**
     * @brief Warps current image to match coordinate system of target image
     * @param map Target georeferenced image for coordinate system reference
     * @param delete_overlap If true, sets overlapping pixels to invalid; if false, samples values
     * @details Core warping function that transforms pixels between coordinate systems.
     *          For each pixel in the current image:
     *          1. Projects pixel to target coordinate system
     *          2. Checks if projection is valid and within target bounds
     *          3. Either deletes overlap or samples value based on delete_overlap flag
     *          Uses interpolation when sampling values from the target image.
     */
    template <IsTIFFData T>
    void GeoreferenceImage<T>::warpTo(const GeoreferenceImage<T>& map, bool delete_overlap)
    {
        DEMProjection target = map.projection;
        vira::images::Resolution targetRes = map.resolution();

        projection.initToMap(target);
        for (int i = 0; i < resolution().x; ++i) {
            for (int j = 0; j < resolution().y; ++j) {
                // If point is already invalid, no need to remove:
                if (delete_overlap && vira::utils::IS_INVALID<T>((*this)(i, j))) {
                    continue;
                }

                Pixel projectedPixel = projection.transformToMap(Pixel{ i,j }, target);

                // Verify the overlap is valid:
                if (std::isnan(projectedPixel[0]) || std::isnan(projectedPixel[1])) {
                    continue;
                }
                if (projectedPixel[0] < 0 || projectedPixel[1] < 0) {
                    continue;
                }
                if (projectedPixel[0] >= static_cast<float>(targetRes.x) || projectedPixel[1] >= static_cast<float>(targetRes.y)) {
                    continue;
                }

                // Determine if point projected to is valid:
                int X = static_cast<int>(projectedPixel[0]);
                int Y = static_cast<int>(projectedPixel[1]);
                if (vira::utils::IS_VALID<T>(map(X, Y))) {

                    // Determine whether to delete or to sample:
                    if (delete_overlap) {
                        (*this)(i, j) = vira::utils::INVALID_VALUE<T>();
                    }
                    else {
                        (*this)(i, j) = map.interpolatePixel(projectedPixel);
                    }
                }
            }
        }
        projection.free();
    };
};