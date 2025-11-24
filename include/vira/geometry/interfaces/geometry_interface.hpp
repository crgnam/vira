#ifndef VIRA_GEOMETRY_INTERFACES_UNIFIED_GEOMETRY_INTERFACE_HPP
#define VIRA_GEOMETRY_INTERFACES_UNIFIED_GEOMETRY_INTERFACE_HPP

#include <filesystem>
#include <string>
#include <functional>

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"
#include "vira/geometry/interfaces/load_result.hpp"

namespace fs = std::filesystem;

// Forward Declare:
namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;

    namespace scene {
        template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
        class Group;
    }
}

namespace vira::geometry {
    /**
     * @brief Interface for loading and saving 3D geometry data with spectral rendering support.
     *
     * @tparam TSpectral The spectral type used for color representation (must satisfy IsSpectral concept)
     * @tparam TFloat The floating-point type used for general calculations (must satisfy IsFloat concept)
     * @tparam TMeshFloat The floating-point type used for mesh data (must satisfy IsFloat concept and have greater precision than TFloat)
     *
     * GeometryInterface provides a unified API for loading 3D geometry from various file formats
     * and integrating them into a spectral rendering scene. It supports automatic format detection,
     * multiple file formats through different backend loaders (Assimp, DSK), and configurable
     * color space conversions between RGB and spectral representations.
     *
     * The class is templated to support different spectral types and floating-point precisions,
     * with the constraint that mesh data uses higher precision floating-point than general
     * calculations to maintain geometric accuracy.
     *
     * Key features:
     * - Automatic file format detection based on extension
     * - Support for multiple 3D file formats via Assimp integration
     * - Native DSK (Digital Shape Kernel) format support
     * - Configurable RGB-to-spectral color conversion functions
     * - Group-based geometry saving functionality
     * - DSK-specific albedo configuration for material properties
     *
     * @note The LesserFloat<TFloat, TMeshFloat> constraint ensures that mesh data maintains
     *       higher numerical precision than general scene calculations.
     *
     * @see vira::Scene
     * @see vira::scene::Group
     * @see LoadedMeshes
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class GeometryInterface {
    public:
        GeometryInterface() = default;
        ~GeometryInterface() = default;

        // Load geometry from file
        LoadedMeshes<TFloat> load(vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, const fs::path& filepath, std::string format = "AUTO");

        void saveGroup(vira::scene::Group<TSpectral, TFloat, TMeshFloat>& group, const fs::path& filepath, std::string format = "AUTO");

        // Configuration options
        void setRgbToSpectralFunction(std::function<TSpectral(ColorRGB)> func);

        // DSK-specific options
        void setDSKAlbedo(const TSpectral& albedo);

    private:
        // Format detection
        std::string detectFormat(const fs::path& filepath, const std::string& requested_format);

        // Format-specific loaders
        LoadedMeshes<TFloat> loadWithAssimp(vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, const fs::path& filepath);
        LoadedMeshes<TFloat> loadDSK(vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, const fs::path& filepath);

        // Configuration
        std::function<TSpectral(ColorRGB)> rgbToSpectral_function = rgbToSpectral_val<TSpectral>;
        std::function<ColorRGB(TSpectral)> spectralToRGB_function = spectralToRGB_val<TSpectral>;

        // DSK-specific settings
        TSpectral dsk_albedo = TSpectral{ 0.03 };
    };
}

#include "implementation/geometry/interfaces/geometry_interface.ipp"

#endif