#ifndef VIRA_DEMS_DEM_HPP
#define VIRA_DEMS_DEM_HPP

#include <vector>
#include <cstdint>

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/reference_frame.hpp"
#include "vira/images/image.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/dems/georeference_image.hpp"
#include "vira/dems/dem_projection.hpp"

// Forward Declare:
namespace vira::geometry {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Mesh;
};

namespace vira::quipu {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class DEMQuipu;
};

namespace vira::dems {
    enum AlbedoType : uint8_t {
        CONSTANT_ALBEDO = 0,
        FLOAT_IMAGE_ALBEDO = 1,
        COLOR_IMAGE_ALBEDO = 2
    };

    /**
    * @brief Digital Elevation Model with integrated material properties
    * @tparam TSpectral Spectral data type for wavelength-dependent albedo modeling
    * @tparam TFloat Floating point precision for geometric calculations
    * @tparam TMeshFloat Floating point precision for mesh data (must be >= TFloat)
    *
    * @details Represents terrain elevation data combined with surface reflectance properties
    *          for realistic 3D terrain modeling and rendering. Supports multiple albedo
    *          representations (constant, grayscale, or full spectral) to accommodate
    *          different material modeling requirements. Provides comprehensive spatial
    *          processing capabilities including tiling, level-of-detail generation,
    *          mesh creation, and coordinate system transformations. Designed for both
    *          terrestrial and planetary surface modeling applications with full
    *          georeferencing support through integrated projection systems.
    */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class DEM {
    public:
        DEM() = default;

        DEM(GeoreferenceImage<float> height_map, TSpectral albedo = TSpectral{ 1 });
        DEM(GeoreferenceImage<float> height_map, vira::images::Image<float> albedo);
        DEM(GeoreferenceImage<float> height_map, vira::images::Image<TSpectral> albedo);

        DEM(vira::images::Image<float> height_image, TSpectral albedo, DEMProjection projection);
        DEM(vira::images::Image<float> height_image, vira::images::Image<float> albedo, DEMProjection projection);
        DEM(vira::images::Image<float> height_image, vira::images::Image<TSpectral> albedo, DEMProjection projection);

        // Mesh Generators
        std::unique_ptr<vira::geometry::Mesh<TSpectral, TFloat, TMeshFloat>> makeMesh(vec3<double> offset = vec3<double>{ 0,0,0 }) const;
        vira::geometry::IndexBuffer makeIndexBuffer() const;
        vira::geometry::VertexBuffer<TSpectral, TMeshFloat> makeVertexBuffer(vec3<double> offset = vec3<double>{ 0,0,0 }) const;
        vec3<double> computeOrigin() const;

        std::vector<DEM<TSpectral, TFloat, TMeshFloat>> makePyramid(vira::images::Resolution min_resolution = vira::images::Resolution{ 8,8 }, bool fill_missing = false) const;

        void scaleHeights(float scale);
        void fillMissing();
        void resize(float scale);
        void resize(vira::images::Resolution new_resolution);

        std::vector<DEM<TSpectral, TFloat, TMeshFloat>> tile(int max_vertices = 1000000, int overlap = 0);
        std::vector<DEM<TSpectral, TFloat, TMeshFloat>> manualTile(int row_slices, int col_slices, int overlap = 0);

        template <IsTIFFData T>
        void removeOverlap(const GeoreferenceImage<T>& target);


        // Getters:
        vira::images::Resolution resolution() const { return heights_.resolution(); } ///< Returns the resolution
        const vira::images::Image<float>& getHeights() const { return heights_; } ///< Returns the read-only height data
        vira::images::Image<float>& getHeights() { return heights_; } ///< Returns mutable reference to height data

        AlbedoType getAlbedoType() const { return albedo_type_; } ///< Returns the type of albedo data (constant, grayscale, or spectral)
        TSpectral getConstantAlbedo() const { return constant_albedo_; } ///< Returns the constant albedo value when using uniform albedo
        const vira::images::Image<float>& getAlbedos_f() const { return albedos_f_; } ///< Returns read-only reference to grayscale albedo image
        vira::images::Image<float>& getAlbedos_f() { return albedos_f_; } ///< Returns mutable reference to grayscale albedo image
        const vira::images::Image<TSpectral>& getAlbedos() const { return albedos_; } ///< Returns read-only reference to spectral albedo image
        vira::images::Image<TSpectral>& getAlbedos() { return albedos_; } ///< Returns mutable reference to spectral albedo image
        
        bool getIsOGR() const { return projection_.isOGR; } ///< Return if DEM is defined using OGR
        std::array<double, 2> getGSD() const;

        DEMProjection getProjection() const { return projection_; } ///< Return a copy of the DEMProject struct

        void clear();

    private:
        vira::images::Image<float> heights_;

        TSpectral constant_albedo_;
        vira::images::Image<TSpectral> albedos_;
        vira::images::Image<float> albedos_f_;
        AlbedoType albedo_type_ = CONSTANT_ALBEDO;
        
        // Projection:
        mutable DEMProjection projection_;

        friend class vira::quipu::DEMQuipu<TSpectral, TFloat, TMeshFloat>;
    };
};

#include "implementation/dems/dem.ipp"

#endif