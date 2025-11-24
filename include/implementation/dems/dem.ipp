#include <memory>
#include <vector>
#include <cstddef>
#include <string>
#include <limits>
#include <stdexcept>

#include "tbb/parallel_for.h"
#include "tbb/blocked_range2d.h"

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/reference_frame.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/dems/dem_projection.hpp"

namespace vira::dems {
    // ==================== //
    // === Constructors === //
    // ==================== //
    /**
     * @brief Constructs a DEM with georeferenced height data and constant albedo
     * @param height_map Georeferenced elevation data with spatial projection
     * @param albedo Uniform spectral albedo value for all pixels
     * @details Creates a DEM where all surface points have the same reflectance properties.
     *          Suitable for uniform terrain or when albedo variation is not important.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat>::DEM(GeoreferenceImage<float> height_map, TSpectral albedo) :
        heights_{ height_map.data }, constant_albedo_{ albedo }, projection_{ height_map.projection }
    {
        albedo_type_ = CONSTANT_ALBEDO;
    };

    /**
     * @brief Constructs a DEM with georeferenced height data and single-channel albedo map
     * @param height_map Georeferenced elevation data with spatial projection
     * @param albedo Single-channel albedo image (single value per pixel)
     * @throws std::runtime_error if height and albedo resolutions don't match
     * @details Creates a DEM with spatially-varying grayscale reflectance properties.
     *          Height and albedo images must have identical dimensions.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat>::DEM(GeoreferenceImage<float> height_map, vira::images::Image<float> albedo) :
        heights_{ height_map.data }, albedos_f_{ albedo }, projection_{ height_map.projection }
    {
        albedo_type_ = FLOAT_IMAGE_ALBEDO;

        if (heights_.resolution() != albedos_f_.resolution()) {
            throw std::runtime_error("The provided heights_ and albedos_ do not have the same resolution");
        }
    };

    /**
     * @brief Constructs a DEM with georeferenced height data and spectral albedo map
     * @param height_map Georeferenced elevation data with spatial projection
     * @param albedo Spectral albedo image (full spectrum per pixel)
     * @throws std::runtime_error if height and albedo resolutions don't match
     * @details Creates a DEM with spatially-varying spectral reflectance properties.
     *          Enables wavelength-dependent surface modeling for realistic rendering.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat>::DEM(GeoreferenceImage<float> height_map, vira::images::Image<TSpectral> albedo) :
        heights_{ height_map.data }, albedos_{ albedo }, projection_{ height_map.projection }
    {
        albedo_type_ = COLOR_IMAGE_ALBEDO;

        if (heights_.resolution() != albedos_.resolution()) {
            throw std::runtime_error("The provided heights_ and albedos_ do not have the same resolution");
        }
    };

    /**
     * @brief Constructs a DEM with height data and constant albedo given a DEMProjection georeference definition
     * @param height_image Elevation data with spatial projection
     * @param albedo Uniform spectral albedo value for all pixels
     * @param projection A DEMProjection to serve as the georeference for the height_map
     * @details Creates a DEM where all surface points have the same reflectance properties.
     *          Suitable for uniform terrain or when albedo variation is not important.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat>::DEM(vira::images::Image<float> height_image, TSpectral albedo, DEMProjection projection) :
        heights_{ height_image }, constant_albedo_{ albedo }, projection_{ projection }
    {
        albedo_type_ = CONSTANT_ALBEDO;
    };

    /**
     * @brief Constructs a DEM with height data and single-channel albedo map given a DEMProjection georeference definition
     * @param height_image Elevation data with spatial projection
     * @param albedo Single-channel albedo image (single value per pixel)
     * @param projection A DEMProjection to serve as the georeference for the height_map
     * @details Creates a DEM where all surface points have the same reflectance properties.
     *          Suitable for uniform terrain or when albedo variation is not important.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat>::DEM(vira::images::Image<float> height_image, vira::images::Image<float> albedo, DEMProjection projection) :
        heights_{ height_image }, albedos_f_{ albedo }, projection_{ projection }
    {
        albedo_type_ = FLOAT_IMAGE_ALBEDO;

        if (heights_.resolution() != albedos_f_.resolution()) {
            throw std::runtime_error("The provided heights_ and albedos_ do not have the same resolution");
        }
    };

    /**
     * @brief Constructs a DEM with height data and spectral albedo map given a DEMProjection georeference definition
     * @param height_image Elevation data with spatial projection
     * @param albedo Spectral albedo image (full spectrum per pixel)
     * @param projection A DEMProjection to serve as the georeference for the height_map
     * @details Creates a DEM where all surface points have the same reflectance properties.
     *          Suitable for uniform terrain or when albedo variation is not important.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    DEM<TSpectral, TFloat, TMeshFloat>::DEM(vira::images::Image<float> height_image, vira::images::Image<TSpectral> albedo, DEMProjection projection) :
        heights_{ height_image }, albedos_{ albedo }, projection_{ projection }
    {
        albedo_type_ = COLOR_IMAGE_ALBEDO;

        if (heights_.resolution() != albedos_.resolution()) {
            throw std::runtime_error("The provided heights_ and albedos_ do not have the same resolution");
        }
    };


    // ======================= //
    // === Mesh Generators === //
    // ======================= //
    /**
     * @brief Generates a 3D mesh representation of the DEM
     * @param offset Vector to translate the mesh vertices
     * @return Unique pointer to a Mesh object representing the DEM surface
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::unique_ptr<vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>> DEM<TSpectral, TFloat, TMeshFloat>::makeMesh(vec3<double> offset) const
    {
        return std::make_unique<vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>>(this->makeVertexBuffer(offset), this->makeIndexBuffer());
    };

    /**
    * @brief Generates triangle index buffer for mesh triangulation
    * @return Index buffer defining triangle connectivity for the height grid
    * @details Creates triangle indices that connect the DEM grid points into a
    *          triangulated mesh surface. Each quad in the grid is split into two
    *          triangles, with proper winding order for consistent surface normals.
    *          Used in conjunction with vertex buffer to create complete mesh geometry.
    */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::geometry::IndexBuffer DEM<TSpectral, TFloat, TMeshFloat>::makeIndexBuffer() const
    {
        return vira::images::imageToIndexBuffer(heights_);
    };

    /**
     * @brief Generates vertex buffer with 3D positions and albedo data
     * @param offset World coordinate offset to apply to all vertices
     * @return Vertex buffer containing position and material data for each grid point
     * @details Transforms height map pixels to 3D world coordinates using the projection.
     *          Invalid height values (NaN/inf) result in infinite vertex positions for culling.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vira::geometry::VertexBuffer<TSpectral, TMeshFloat> DEM<TSpectral, TFloat, TMeshFloat>::makeVertexBuffer(vec3<double> offset) const
    {
        vira::images::Resolution resolution = this->heights_.resolution();
        size_t numVertices = static_cast<size_t>(resolution.x) * static_cast<size_t>(resolution.y);
        vira::geometry::VertexBuffer<TSpectral, TMeshFloat> vertexBuffer(numVertices);

        projection_.initToWorld();

        for (int j = 0; j < resolution.y; ++j) {
            for (int i = 0; i < resolution.x; ++i) {
                vira::geometry::Vertex<TSpectral, TMeshFloat> vertex;

                float z = this->heights_(i, j);
                if (std::isinf(z) || std::isnan(z)) {
                    vertex.position = vec3<TMeshFloat>{ std::numeric_limits<TMeshFloat>::infinity() };
                }
                else {
                    // Calculate the vertex position:
                    vec3<double> position = projection_.transformToWorld(Pixel{ i,j }, z);
                    if (projection_.isOGR) {
                        vertex.position = (position - offset);
                    }
                    else {
                        vertex.position = position;
                    }

                    // Assign albedo value:
                    if (albedo_type_ == CONSTANT_ALBEDO) {
                        vertex.albedo = constant_albedo_;
                    }
                    else if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
                        vertex.albedo = TSpectral{ this->albedos_f_(i,j) };
                    }
                    else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
                        vertex.albedo = this->albedos_(i, j);
                    }
                }

                size_t idx = (static_cast<size_t>(j) * static_cast<size_t>(resolution.x)) + static_cast<size_t>(i);
                vertexBuffer[idx] = vertex;
            }
        }

        projection_.free();

        return vertexBuffer;
    };

    /**
     * @brief Computes the origin point for mesh positioning
     * @return World coordinate origin point for the DEM
     * @details For georeferenced DEMs, returns the center point in world coordinates.
     *          For OGR DEMs, returns (0,0,0). Used for mesh centering
     *          and coordinate system alignment.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    vec3<double> DEM<TSpectral, TFloat, TMeshFloat>::computeOrigin() const
    {
        if (projection_.isOGR) {
            // Compute reference height:
            float z = 0;
            for (size_t i = 0; i < heights_.size(); ++i) {
                if (vira::utils::IS_VALID(heights_[i])) {
                    z = heights_[i];
                    break;
                }
            }

            // Compute raster-space center:
            vira::images::Resolution res = heights_.resolution();
            double i = static_cast<double>(res.x) / 2.;
            double j = static_cast<double>(res.y) / 2.;

            // Create Proj context and transformer:
            projection_.initToWorld();
            vec3<double> position = projection_.transformToWorld(Pixel{ i,j }, z);
            projection_.free();

            return position;
        }
        else {
            return vec3<double>{0, 0, 0};
        }
    };

    /**
     * @brief Creates a pyramid of progressively lower resolution DEMs
     * @param minResolution Minimum resolution for the coarsest level
     * @param fillMissing Whether to fill missing data before downsampling
     * @return Vector of DEMs from full resolution to minimum resolution
     * @details Generates Level-of-Detail (LOD) hierarchy by successive 2x downsampling.
     *          Useful for adaptive mesh rendering and distant terrain representation.
     *          Missing data filling prevents artifacts during downsampling.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<DEM<TSpectral, TFloat, TMeshFloat>> DEM<TSpectral, TFloat, TMeshFloat>::makePyramid(vira::images::Resolution minResolution, bool fillMissing) const
    {
        DEM<TSpectral, TFloat, TMeshFloat> dem = *this;

        // Calculate the number of Levels-of-Detail:
        float X_ = static_cast<float>(dem.resolution().x);
        float Y_ = static_cast<float>(dem.resolution().y);
        float mX_ = static_cast<float>(minResolution.x);
        float mY_ = static_cast<float>(minResolution.y);
        float maxXhalvings = std::round(std::log2(X_ / mX_));
        float maxYhalvings = std::round(std::log2(Y_ / mY_));
        size_t numberOfLoDs = static_cast<size_t>(std::max(std::min(maxXhalvings, maxYhalvings), 0.f)) + 1;

        // Pre-allocate:
        std::vector<DEM<TSpectral, TFloat, TMeshFloat>> LoDs(numberOfLoDs);

        for (size_t i = 0; i < numberOfLoDs; ++i) {
            if (i != 0) {
                // Compute proper resolution:
                vira::images::Resolution new_resolution;
                new_resolution.x = dem.resolution().x / 2;
                new_resolution.y = dem.resolution().y / 2;

                // Fill missing data:
                if (i == 1 && fillMissing) {
                    dem.fillMissing();
                }

                // Perform resizing and store:
                dem.resize(new_resolution);
            }

            LoDs[i] = dem;
        }

        return LoDs;
    };

    // ======================== //
    // === Modifier Methods === //
    // ======================== //
    /**
     * @brief Scales all elevation values by a constant factor
     * @param scale Multiplicative scaling factor
     * @details Applies uniform vertical scaling to the entire DEM. Useful for
     *          unit conversions or vertical exaggeration for visualization.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEM<TSpectral, TFloat, TMeshFloat>::scaleHeights(float scale)
    {
        for (size_t i = 0; i < heights_.size(); ++i) {
            heights_[i] = scale * heights_[i];
        }
    };

    /**
     * @brief Fills missing data using interpolation
     * @details Interpolates missing (NaN/inf) values in both height and albedo data
     *          using surrounding valid pixels. Essential for creating continuous
     *          surfaces from sparse or incomplete datasets.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEM<TSpectral, TFloat, TMeshFloat>::fillMissing()
    {
        this->heights_.fillMissing();

        if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
            this->albedos_f_.fillMissing();
        }
        else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
            this->albedos_.fillMissing();
        }
    };

    /**
     * @brief Resizes the DEM to a new resolution
     * @param scale Target scaling of the new resolution
     * @details Resamples height and albedo data to the specified resolution using
     *          appropriate filtering. Updates projection parameters to maintain
     *          correct spatial referencing.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEM<TSpectral, TFloat, TMeshFloat>::resize(float scale)
    {
        return this->resize(scale * this->resolution());
    };

    /**
     * @brief Resizes the DEM to a new resolution
     * @param new_resolution Target resolution for resampling
     * @details Resamples height and albedo data to the specified resolution using
     *          appropriate filtering. Updates projection parameters to maintain
     *          correct spatial referencing.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEM<TSpectral, TFloat, TMeshFloat>::resize(vira::images::Resolution new_resolution)
    {
        if (new_resolution == this->resolution()) {
            return;
        }

        // Calculate new pixel scale:
        projection_.resize(new_resolution);

        // Subsample the images:
        this->heights_.resize(new_resolution);

        if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
            this->albedos_f_.resize(new_resolution);
        }
        else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
            this->albedos_.resize(new_resolution);
        }
    };



    // ====================== //
    // === Tiling Methods === //
    // ====================== //
    /**
     * @brief Automatically tiles the DEM based on vertex count constraints
     * @param maxVertices Maximum number of vertices per tile
     * @param overlap Pixel overlap between adjacent tiles
     * @return Vector of DEM tiles with appropriate dimensions
     * @details Automatically determines optimal tiling configuration to meet
     *          vertex constraints while maintaining reasonable aspect ratios.
     *          Overlap prevents seams between tiles during rendering.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<DEM<TSpectral, TFloat, TMeshFloat>> DEM<TSpectral, TFloat, TMeshFloat>::tile(int maxVertices, int overlap)
    {
        // TODO Consolidate this interface with GeoreferenceImage!
        vira::images::Resolution resolution = this->resolution();
        double w = static_cast<double>(resolution.x);
        double h = static_cast<double>(resolution.y);
        double aspect = w / static_cast<double>(resolution.y);
        double n = std::ceil(w / std::sqrt(static_cast<double>(maxVertices)));

        int rowSlices = static_cast<int>(std::round(n / aspect));
        int colSlices = static_cast<int>(n);

        double nC = static_cast<double>(colSlices);
        double nR = static_cast<double>(rowSlices);
        double ov = static_cast<double>(overlap);
        double tw = std::floor((w + (nC - 1) * ov) / nC);
        double th = std::floor((h + (nR - 1) * ov) / nR);

        if (static_cast<int>(tw * th) > maxVertices) {
            rowSlices++;
            colSlices++;
        }

        return this->manualTile(rowSlices, colSlices, overlap);
    };

    /**
     * @brief Manually tiles the DEM with specified grid dimensions
     * @param rowSlices Number of tiles in the row direction
     * @param colSlices Number of tiles in the column direction
     * @param overlap Pixel overlap between adjacent tiles
     * @return Vector of DEM tiles arranged in the specified grid
     * @details Creates a regular grid of tiles with user-specified dimensions.
     *          Tiles with all invalid data are automatically excluded from results.
     *          Each tile maintains proper spatial referencing for its subset.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::vector<DEM<TSpectral, TFloat, TMeshFloat>> DEM<TSpectral, TFloat, TMeshFloat>::manualTile(int rowSlices, int colSlices, int overlap)
    {
        // Calculate tile dimensions:
        vira::images::Resolution resolution = this->resolution();
        double w = static_cast<double>(resolution.x);
        double h = static_cast<double>(resolution.y);
        double nC = static_cast<double>(colSlices);
        double nR = static_cast<double>(rowSlices);

        vira::images::Resolution tileResolution;
        tileResolution.x = static_cast<int>(std::floor((w + (nC - 1) * static_cast<double>(overlap)) / nC));
        tileResolution.y = static_cast<int>(std::floor((h + (nR - 1) * static_cast<double>(overlap)) / nR));

        // Delete any existing tile definition:
        size_t numTiles = static_cast<size_t>(colSlices) * static_cast<size_t>(rowSlices);
        std::vector<DEM<TSpectral, TFloat, TMeshFloat>> tiles(numTiles);

        // Generate tile definitions:
        size_t idx = 0;
        for (int col = 0; col < colSlices; col++) {
            for (int row = 0; row < rowSlices; row++) {

                vira::images::Resolution useResolution = tileResolution;

                // Calculate starting point of the tile:
                int xoff = col * (tileResolution.x - overlap - 1);
                int yoff = row * (tileResolution.y - overlap - 1);

                // Ensure we cover the entire DEM:
                if (col == colSlices - 1) {
                    useResolution.x = resolution.x - xoff;
                }
                if (row == rowSlices - 1) {
                    useResolution.y = resolution.y - yoff;
                }

                // Extract the height data:
                bool allInvalid = true;
                vira::images::Image<float> useHeights(useResolution);
                for (int i = 0; i < useResolution.x; i++) {
                    for (int j = 0; j < useResolution.y; j++) {
                        auto val = this->heights_(i + xoff, j + yoff);
                        useHeights(i, j) = val;
                        if (!std::isinf(val)) {
                            allInvalid = false;
                        }
                    }
                }

                if (allInvalid) {
                    continue;
                }

                // Extract the albedo data:
                vira::images::Image<TSpectral> useAlbedos;
                vira::images::Image<float> useAlbedos_f;
                if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
                    useAlbedos_f = vira::images::Image<float>(useResolution);

                    for (int i = 0; i < useResolution.x; i++) {
                        for (int j = 0; j < useResolution.y; j++) {
                            useAlbedos_f(i, j) = this->albedos_f_(i + xoff, j + yoff);
                        }
                    }
                }
                else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
                    useAlbedos = vira::images::Image<TSpectral>(useResolution);

                    for (int i = 0; i < useResolution.x; i++) {
                        for (int j = 0; j < useResolution.y; j++) {
                            useAlbedos(i, j) = this->albedos_(i + xoff, j + yoff);
                        }
                    }
                }

                // Create the projection_:
                DEMProjection projection = projection_;
                projection.resolution = useResolution;
                projection.tileDefinitions = vira::images::Resolution{ colSlices, rowSlices };
                projection.tileRow = static_cast<uint32_t>(row);
                projection.tileCol = static_cast<uint32_t>(col);

                if (projection_.isOGR) {
                    projection.xoff = projection_.xoff + projection_.dx * static_cast<double>(xoff);
                    projection.yoff = projection_.yoff + projection_.dy * static_cast<double>(yoff);

                    if (albedo_type_ == CONSTANT_ALBEDO) {
                        tiles[idx] = DEM<TSpectral, TFloat, TMeshFloat>(useHeights, constant_albedo_, projection);
                    }
                    else if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
                        tiles[idx] = DEM<TSpectral, TFloat, TMeshFloat>(useHeights, useAlbedos_f, projection);
                    }
                    else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
                        tiles[idx] = DEM<TSpectral, TFloat, TMeshFloat>(useHeights, useAlbedos, projection);
                    }

                }
                else {
                    // Adjustment to normal frame:
                    double tileSizeX = (static_cast<double>(resolution.x) - 1.) / 2.;
                    double tileSizeY = (static_cast<double>(resolution.y) - 1.) / 2.;
                    double xadj = tileSizeX - (static_cast<double>(useResolution.x) - 1.) / 2.;
                    double yadj = tileSizeY - (static_cast<double>(useResolution.y) - 1.) / 2.;

                    projection.xoff = projection_.xoff + projection_.dx * (static_cast<double>(xoff) - xadj);
                    projection.yoff = projection_.yoff + projection_.dy * (static_cast<double>(yoff) - yadj);

                    if (albedo_type_ == CONSTANT_ALBEDO) {
                        tiles[idx] = DEM<TSpectral, TFloat, TMeshFloat>(useHeights, constant_albedo_, projection);
                    }
                    else if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
                        tiles[idx] = DEM<TSpectral, TFloat, TMeshFloat>(useHeights, useAlbedos_f, projection);
                    }
                    else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
                        tiles[idx] = DEM<TSpectral, TFloat, TMeshFloat>(useHeights, useAlbedos, projection);
                    }
                }

                idx++;
            }
        }

        // Remove unused tiles (unused due to invalid data):
        tiles.erase(tiles.begin() + static_cast<std::ptrdiff_t>(idx), tiles.end());

        return tiles;
    };

    /**
     * @brief Removes overlapping regions with another georeferenced dataset
     * @tparam T Data type of the reference image
     * @param map Reference georeferenced image defining overlap regions
     * @details Identifies pixels that overlap with valid data in the reference image
     *          and marks them as invalid. Used for preventing duplicate data when
     *          combining multiple elevation datasets with overlapping coverage.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    template <IsTIFFData T>
    void DEM<TSpectral, TFloat, TMeshFloat>::removeOverlap(const GeoreferenceImage<T>& map)
    {
        DEMProjection target = map.projection;
        vira::images::Resolution targetRes = map.resolution();

        projection_.initToMap(target);
        for (int i = 0; i < resolution().x; ++i) {
            for (int j = 0; j < resolution().y; ++j) {
                // If point is already invalid, no need to remove:
                if (vira::utils::IS_INVALID<T>(this->heights_(i, j))) {
                    continue;
                }

                Pixel projectedPixel = projection_.transformToMap(Pixel{ i,j }, target);

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
                    this->heights_(i, j) = vira::utils::INVALID_VALUE<float>();
                    if (albedo_type_ == FLOAT_IMAGE_ALBEDO) {
                        this->albedos_f_(i, j) = vira::utils::INVALID_VALUE<float>();
                    }
                    else if (albedo_type_ == COLOR_IMAGE_ALBEDO) {
                        this->albedos_(i, j) = TSpectral{ vira::utils::INVALID_VALUE<float>() };
                    }
                }
            }
        }
        projection_.free();
    };

    /**
     * @brief Computes Ground Sample Distance in both spatial directions
     * @return Array containing GSD in X and Y directions (meters per pixel)
     * @details Calculates the spatial resolution of the DEM in world units.
     *          Essential for understanding data quality and appropriate usage scales.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::array<double, 2> DEM<TSpectral, TFloat, TMeshFloat>::getGSD() const
    {
        std::array<double, 2> gsd;
        gsd[0] = projection_.dx * projection_.pixelScale[0];
        gsd[1] = projection_.dy * projection_.pixelScale[1];
        return gsd;
    };

    /**
     * @brief Clears all DEM data and resets to empty state
     * @details Releases all image data and resets projection to default state.
     *          Used for memory cleanup and object reuse.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void DEM<TSpectral, TFloat, TMeshFloat>::clear()
    {
        heights_.clear();
        albedos_.clear();
        albedos_f_.clear();
        projection_ = DEMProjection{};
    };
};